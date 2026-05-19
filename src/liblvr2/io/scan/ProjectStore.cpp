#include "lvr2/io/scan.hpp"

#include "lvr2/io/YAML.hpp"
#include "lvr2/io/schema/ScanProjectSchemaHDF5.hpp"
#include "lvr2/io/schema/ScanProjectSchemaRaw.hpp"
#include "lvr2/types/ScanTypes.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace lvr2::io::scan
{

struct ProjectStore::State final
{
    State(storage::StorageContext context, Schema publicSchema);

    storage::StorageContext context;
    Schema publicSchema;
    std::shared_ptr<lvr2::ScanProjectSchema> schema;
};

namespace
{

storage::Error makeError(storage::ErrorCode code,
                         std::string message,
                         std::string group = {},
                         std::string name = {})
{
    return {code, std::move(message), {}, std::move(group), std::move(name)};
}

template<class T>
storage::Result<T> forwardError(const storage::Error& error)
{
    return storage::unexpected(error);
}

storage::Status forwardStatus(const storage::Error& error)
{
    return storage::unexpected(error);
}

std::shared_ptr<lvr2::ScanProjectSchema> makeSchema(const Schema& schema, const std::string& uri)
{
    switch (schema.kind())
    {
    case SchemaKind::Raw:
        return std::make_shared<lvr2::ScanProjectSchemaRaw>(uri);
    case SchemaKind::RawPly:
        return std::make_shared<lvr2::ScanProjectSchemaRawPly>(uri);
    case SchemaKind::Hdf5:
        return std::make_shared<lvr2::ScanProjectSchemaHDF5>();
    }

    return std::make_shared<lvr2::ScanProjectSchemaRawPly>(uri);
}

storage::Result<storage::GroupKey> groupKey(const lvr2::Description& description)
{
    if (!description.dataRoot)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "description has no data group"));
    }
    return storage::GroupKey{*description.dataRoot};
}

storage::Result<storage::MetaKey> metaKey(const lvr2::Description& description)
{
    if (!description.metaRoot || !description.meta)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "description has no metadata key"));
    }
    return storage::MetaKey{*description.metaRoot, *description.meta};
}

storage::Result<storage::DataKey> dataKey(const lvr2::Description& description)
{
    if (!description.dataRoot || !description.data)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "description has no data key"));
    }
    return storage::DataKey{*description.dataRoot, *description.data};
}

storage::Result<YAML::Node> readMeta(const ProjectStore::State& state,
                                     const lvr2::Description& description)
{
    auto key = metaKey(description);
    if (!key)
    {
        return forwardError<YAML::Node>(key.error());
    }
    auto value = state.context.backend->readMeta(key.value());
    if (!value)
    {
        return forwardError<YAML::Node>(value.error());
    }

    try
    {
        return YAML::Load(value->text);
    }
    catch (const std::exception& e)
    {
        return storage::unexpected(makeError(storage::ErrorCode::ReadFailed, e.what()));
    }
}

storage::Status writeMeta(ProjectStore::State& state,
                          const lvr2::Description& description,
                          const YAML::Node& node)
{
    auto key = metaKey(description);
    if (!key)
    {
        return forwardStatus(key.error());
    }
    return state.context.backend->writeMeta(key.value(), storage::MetaValue{YAML::Dump(node)});
}

storage::Result<bool> existsGroup(const ProjectStore::State& state,
                                  const lvr2::Description& description)
{
    auto key = groupKey(description);
    if (!key)
    {
        return forwardError<bool>(key.error());
    }
    return state.context.backend->exists(key.value());
}

storage::Result<storage::DataKey> scanPointKey(const ProjectStore::State& state,
                                               std::size_t positionIndex,
                                               std::size_t lidarIndex,
                                               std::size_t scanIndex)
{
    if (state.publicSchema.kind() == SchemaKind::Raw)
    {
        return storage::unexpected(makeError(storage::ErrorCode::Unsupported,
                                            "raw directory point-channel storage is not implemented by this service path"));
    }

    auto direct = dataKey(state.schema->scan(positionIndex, lidarIndex, scanIndex));
    if (direct)
    {
        return direct;
    }

    auto channel = dataKey(state.schema->scanChannel(positionIndex, lidarIndex, scanIndex, "points"));
    if (channel)
    {
        return channel;
    }

    return storage::unexpected(makeError(storage::ErrorCode::Unsupported,
                                        "scan point data has no storage key"));
}

storage::Result<lvr2::PointBufferPtr> loadPoints(const std::shared_ptr<ProjectStore::State>& state,
                                                 const storage::DataKey& key)
{
    if (!state || !state->context.backend)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return state->context.backend->readPointBuffer(key);
}

storage::Status ensureSupportedPosition(const lvr2::ScanPositionPtr& position)
{
    if (!position)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "scan position pointer must not be null"));
    }

    if (!position->cameras.empty() || !position->hyperspectral_cameras.empty())
    {
        return storage::unexpected(makeError(
            storage::ErrorCode::Unsupported,
            "ProjectStore cannot save camera or hyperspectral payloads yet"));
    }

    return {};
}

storage::Status saveScan(ProjectStore::State& state,
                         std::size_t positionIndex,
                         std::size_t lidarIndex,
                         std::size_t scanIndex,
                         const lvr2::ScanPtr& scan)
{
    if (!scan)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "scan pointer must not be null"));
    }

    if (!scan->loaded() && scan->loadable())
    {
        scan->load();
    }

    if (scan->points)
    {
        auto key = scanPointKey(state, positionIndex, lidarIndex, scanIndex);
        if (!key)
        {
            return forwardStatus(key.error());
        }
        auto wrotePoints = state.context.backend->writePointBuffer(key.value(), scan->points);
        if (!wrotePoints)
        {
            return wrotePoints;
        }
    }

    auto description = state.schema->scan(positionIndex, lidarIndex, scanIndex);
    if (description.meta)
    {
        YAML::Node node = YAML::convert<lvr2::Scan>::encode(*scan);
        auto wroteMeta = writeMeta(state, description, node);
        if (!wroteMeta)
        {
            return wroteMeta;
        }
    }

    return {};
}

storage::Result<lvr2::ScanPtr> loadScan(const std::shared_ptr<ProjectStore::State>& state,
                                        std::size_t positionIndex,
                                        std::size_t lidarIndex,
                                        std::size_t scanIndex)
{
    auto description = state->schema->scan(positionIndex, lidarIndex, scanIndex);
    auto exists = existsGroup(*state, description);
    if (!exists)
    {
        return forwardError<lvr2::ScanPtr>(exists.error());
    }
    if (!exists.value())
    {
        return lvr2::ScanPtr{};
    }

    lvr2::ScanPtr scan;
    if (description.meta)
    {
        auto meta = readMeta(*state, description);
        if (!meta)
        {
            return forwardError<lvr2::ScanPtr>(meta.error());
        }
        try
        {
            scan = std::make_shared<lvr2::Scan>(meta.value().as<lvr2::Scan>());
        }
        catch (const std::exception& e)
        {
            return storage::unexpected(makeError(storage::ErrorCode::ReadFailed, e.what()));
        }
    }
    else
    {
        scan = std::make_shared<lvr2::Scan>();
    }

    auto key = scanPointKey(*state, positionIndex, lidarIndex, scanIndex);
    if (!key)
    {
        return forwardError<lvr2::ScanPtr>(key.error());
    }

    auto hasPoints = state->context.backend->exists(key.value());
    if (!hasPoints)
    {
        return forwardError<lvr2::ScanPtr>(hasPoints.error());
    }
    if (hasPoints.value())
    {
        const storage::DataKey pointKey = key.value();
        scan->points_loader = [state, pointKey]() -> lvr2::PointBufferPtr {
            auto loaded = loadPoints(state, pointKey);
            if (!loaded)
            {
                return {};
            }
            return loaded.value();
        };
        scan->points_loader_reduced = [loader = scan->points_loader](lvr2::ReductionAlgorithmPtr reduction) -> lvr2::PointBufferPtr {
            auto points = loader ? loader() : lvr2::PointBufferPtr{};
            if (points && reduction)
            {
                reduction->setPointBuffer(points);
                return reduction->getReducedPoints();
            }
            return points;
        };
        scan->points_saver = [state, positionIndex, lidarIndex, scanIndex](lvr2::ScanPtr savedScan) {
            auto status = saveScan(*state, positionIndex, lidarIndex, scanIndex, savedScan);
            (void)status;
        };

        if (state->context.loadMode == storage::LoadMode::Eager)
        {
            scan->points = scan->points_loader();
        }
    }

    return scan;
}

storage::Status saveLidar(ProjectStore::State& state,
                          std::size_t positionIndex,
                          std::size_t lidarIndex,
                          const lvr2::LIDARPtr& lidar)
{
    if (!lidar)
    {
        return storage::unexpected(makeError(storage::ErrorCode::InvalidArgument,
                                            "lidar pointer must not be null"));
    }

    for (std::size_t scanIndex = 0; scanIndex < lidar->scans.size(); ++scanIndex)
    {
        auto saved = saveScan(state, positionIndex, lidarIndex, scanIndex, lidar->scans[scanIndex]);
        if (!saved)
        {
            return saved;
        }
    }

    auto description = state.schema->lidar(positionIndex, lidarIndex);
    if (description.meta)
    {
        YAML::Node node = YAML::convert<lvr2::LIDAR>::encode(*lidar);
        auto wrote = writeMeta(state, description, node);
        if (!wrote)
        {
            return wrote;
        }
    }

    return {};
}

storage::Result<lvr2::LIDARPtr> loadLidar(const std::shared_ptr<ProjectStore::State>& state,
                                          std::size_t positionIndex,
                                          std::size_t lidarIndex)
{
    auto description = state->schema->lidar(positionIndex, lidarIndex);
    auto exists = existsGroup(*state, description);
    if (!exists)
    {
        return forwardError<lvr2::LIDARPtr>(exists.error());
    }
    if (!exists.value())
    {
        return lvr2::LIDARPtr{};
    }

    lvr2::LIDARPtr lidar;
    if (description.meta)
    {
        auto meta = readMeta(*state, description);
        if (!meta)
        {
            return forwardError<lvr2::LIDARPtr>(meta.error());
        }
        try
        {
            lidar = std::make_shared<lvr2::LIDAR>(meta.value().as<lvr2::LIDAR>());
        }
        catch (const std::exception& e)
        {
            return storage::unexpected(makeError(storage::ErrorCode::ReadFailed, e.what()));
        }
    }
    else
    {
        lidar = std::make_shared<lvr2::LIDAR>();
    }

    for (std::size_t scanIndex = 0;; ++scanIndex)
    {
        auto scan = loadScan(state, positionIndex, lidarIndex, scanIndex);
        if (!scan)
        {
            return forwardError<lvr2::LIDARPtr>(scan.error());
        }
        if (!scan.value())
        {
            break;
        }
        lidar->scans.push_back(scan.value());
    }

    return lidar;
}

storage::Status savePosition(ProjectStore::State& state,
                             std::size_t positionIndex,
                             const lvr2::ScanPositionPtr& position)
{
    auto supported = ensureSupportedPosition(position);
    if (!supported)
    {
        return supported;
    }

    for (std::size_t lidarIndex = 0; lidarIndex < position->lidars.size(); ++lidarIndex)
    {
        auto saved = saveLidar(state, positionIndex, lidarIndex, position->lidars[lidarIndex]);
        if (!saved)
        {
            return saved;
        }
    }

    auto description = state.schema->position(positionIndex);
    if (description.meta)
    {
        YAML::Node node = YAML::convert<lvr2::ScanPosition>::encode(*position);
        auto wrote = writeMeta(state, description, node);
        if (!wrote)
        {
            return wrote;
        }
    }

    return {};
}

storage::Result<lvr2::ScanPositionPtr> loadPosition(const std::shared_ptr<ProjectStore::State>& state,
                                                    std::size_t positionIndex)
{
    auto description = state->schema->position(positionIndex);
    auto exists = existsGroup(*state, description);
    if (!exists)
    {
        return forwardError<lvr2::ScanPositionPtr>(exists.error());
    }
    if (!exists.value())
    {
        return lvr2::ScanPositionPtr{};
    }

    lvr2::ScanPositionPtr position;
    if (description.meta)
    {
        auto meta = readMeta(*state, description);
        if (!meta)
        {
            return forwardError<lvr2::ScanPositionPtr>(meta.error());
        }
        try
        {
            position = std::make_shared<lvr2::ScanPosition>(meta.value().as<lvr2::ScanPosition>());
        }
        catch (const std::exception& e)
        {
            return storage::unexpected(makeError(storage::ErrorCode::ReadFailed, e.what()));
        }
    }
    else
    {
        position = std::make_shared<lvr2::ScanPosition>();
    }

    for (std::size_t lidarIndex = 0;; ++lidarIndex)
    {
        auto lidar = loadLidar(state, positionIndex, lidarIndex);
        if (!lidar)
        {
            return forwardError<lvr2::ScanPositionPtr>(lidar.error());
        }
        if (!lidar.value())
        {
            break;
        }
        position->lidars.push_back(lidar.value());
    }

    return position;
}

LoadOptions loadOptionsFor(storage::StorageKind kind, Schema schema, storage::LoadMode mode)
{
    LoadOptions options;
    options.kind = std::move(kind);
    options.schema = schema;
    options.loadMode = mode;
    return options;
}

SaveOptions saveOptionsFor(storage::StorageKind kind, Schema schema, storage::LoadMode mode)
{
    SaveOptions options;
    options.kind = std::move(kind);
    options.schema = schema;
    options.loadMode = mode;
    return options;
}

LoadOptions loadOptionsFor(const SaveOptions& options)
{
    LoadOptions openOptions;
    openOptions.kind = options.kind;
    openOptions.schema = options.schema;
    openOptions.loadMode = options.loadMode;
    openOptions.hdf5Options = options.hdf5Options;
    return openOptions;
}

} // namespace

ProjectStore::State::State(storage::StorageContext context, Schema publicSchema)
    : context(std::move(context))
    , publicSchema(publicSchema)
    , schema(makeSchema(publicSchema, this->context.backend ? this->context.backend->info().uri : std::string{}))
{
}

Schema Schema::raw()
{
    return Schema{SchemaKind::Raw};
}

Schema Schema::raw_ply()
{
    return Schema{SchemaKind::RawPly};
}

Schema Schema::hdf5()
{
    return Schema{SchemaKind::Hdf5};
}

LoadOptions LoadOptions::directory_raw()
{
    return loadOptionsFor(storage::StorageKind::directory(), Schema::raw(), storage::LoadMode::Lazy);
}

LoadOptions LoadOptions::directory_raw_ply()
{
    return loadOptionsFor(storage::StorageKind::directory(), Schema::raw_ply(), storage::LoadMode::Lazy);
}

LoadOptions LoadOptions::hdf5()
{
    return loadOptionsFor(storage::StorageKind::hdf5(), Schema::hdf5(), storage::LoadMode::Lazy);
}

SaveOptions SaveOptions::directory_raw()
{
    return saveOptionsFor(storage::StorageKind::directory(), Schema::raw(), storage::LoadMode::Lazy);
}

SaveOptions SaveOptions::directory_raw_ply()
{
    return saveOptionsFor(storage::StorageKind::directory(), Schema::raw_ply(), storage::LoadMode::Lazy);
}

SaveOptions SaveOptions::hdf5()
{
    return saveOptionsFor(storage::StorageKind::hdf5(), Schema::hdf5(), storage::LoadMode::Lazy);
}

ProjectStore::ProjectStore(storage::StorageContext context, Schema schema)
    : state_(std::make_shared<State>(std::move(context), schema))
{
}

ProjectStore::~ProjectStore() = default;

storage::Result<lvr2::ScanProjectPtr> ProjectStore::load() const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }

    const auto description = state_->schema->scanProject();
    auto exists = existsGroup(*state_, description);
    if (!exists)
    {
        return forwardError<lvr2::ScanProjectPtr>(exists.error());
    }
    if (!exists.value())
    {
        return storage::unexpected(makeError(storage::ErrorCode::NotFound,
                                            "scan project data group was not found"));
    }

    lvr2::ScanProjectPtr project;
    if (description.meta)
    {
        auto meta = readMeta(*state_, description);
        if (!meta)
        {
            return forwardError<lvr2::ScanProjectPtr>(meta.error());
        }
        try
        {
            project = std::make_shared<lvr2::ScanProject>(meta.value().as<lvr2::ScanProject>());
        }
        catch (const std::exception& e)
        {
            return storage::unexpected(makeError(storage::ErrorCode::ReadFailed, e.what()));
        }
    }
    else
    {
        project = std::make_shared<lvr2::ScanProject>();
    }

    for (std::size_t positionIndex = 0;; ++positionIndex)
    {
        auto position = loadPosition(state_, positionIndex);
        if (!position)
        {
            return forwardError<lvr2::ScanProjectPtr>(position.error());
        }
        if (position.value())
        {
            project->positions.push_back(position.value());
        }
        else if (positionIndex > 1)
        {
            break;
        }
    }

    return project;
}

storage::Status ProjectStore::save(const lvr2::ScanProject& project) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }

    for (std::size_t i = 0; i < project.positions.size(); ++i)
    {
        auto saved = savePosition(*state_, i + 1, project.positions[i]);
        if (!saved)
        {
            return saved;
        }
    }

    const auto description = state_->schema->scanProject();
    if (description.meta)
    {
        YAML::Node node = YAML::convert<lvr2::ScanProject>::encode(project);
        auto wrote = writeMeta(*state_, description, node);
        if (!wrote)
        {
            return wrote;
        }
    }

    return {};
}

storage::Result<lvr2::ScanPositionPtr> ProjectStore::load_position(std::size_t positionIndex) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return loadPosition(state_, positionIndex);
}

storage::Status ProjectStore::save_position(std::size_t positionIndex,
                                            const lvr2::ScanPositionPtr& position) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return savePosition(*state_, positionIndex, position);
}

storage::Result<lvr2::LIDARPtr> ProjectStore::load_lidar(std::size_t positionIndex,
                                                         std::size_t lidarIndex) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return loadLidar(state_, positionIndex, lidarIndex);
}

storage::Status ProjectStore::save_lidar(std::size_t positionIndex,
                                         std::size_t lidarIndex,
                                         const lvr2::LIDARPtr& lidar) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return saveLidar(*state_, positionIndex, lidarIndex, lidar);
}

storage::Result<lvr2::ScanPtr> ProjectStore::load_scan(std::size_t positionIndex,
                                                       std::size_t lidarIndex,
                                                       std::size_t scanIndex) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return loadScan(state_, positionIndex, lidarIndex, scanIndex);
}

storage::Status ProjectStore::save_scan(std::size_t positionIndex,
                                        std::size_t lidarIndex,
                                        std::size_t scanIndex,
                                        const lvr2::ScanPtr& scan) const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }
    return saveScan(*state_, positionIndex, lidarIndex, scanIndex, scan);
}

storage::Result<storage::MetaValue> ProjectStore::load_meta() const
{
    if (!state_ || !state_->context.backend || !state_->schema)
    {
        return storage::unexpected(makeError(storage::ErrorCode::OpenFailed,
                                            "project store is not open"));
    }

    auto key = metaKey(state_->schema->scanProject());
    if (!key)
    {
        return forwardError<storage::MetaValue>(key.error());
    }
    return state_->context.backend->readMeta(key.value());
}

storage::Result<ProjectStore> open_project(const std::string& uri,
                                           const LoadOptions& options,
                                           const storage::StorageRegistry& registry)
{
    storage::OpenRequest request;
    request.uri = uri;
    request.kind = options.kind;
    request.loadMode = options.loadMode;
    request.hdf5 = options.hdf5Options;

    auto backend = registry.open(request);
    if (!backend)
    {
        return forwardError<ProjectStore>(backend.error());
    }

    storage::StorageContext context(std::move(backend.value()), options.loadMode);
    return ProjectStore(std::move(context), options.schema);
}

storage::Result<ProjectStore> open_project(const std::string& uri,
                                           const LoadOptions& options)
{
    auto registry = storage::make_default_registry();
    return open_project(uri, options, registry);
}

storage::Result<ProjectStore> open_directory(const std::string& root,
                                             Schema schema,
                                             storage::LoadMode loadMode)
{
    return open_project(root, loadOptionsFor(storage::StorageKind::directory(), schema, loadMode));
}

storage::Result<ProjectStore> open_hdf5(const std::string& file,
                                        Schema schema,
                                        storage::LoadMode loadMode)
{
    return open_project(file, loadOptionsFor(storage::StorageKind::hdf5(), schema, loadMode));
}

storage::Result<lvr2::ScanProjectPtr> load_project(const std::string& uri,
                                                   const LoadOptions& options,
                                                   const storage::StorageRegistry& registry)
{
    auto store = open_project(uri, options, registry);
    if (!store)
    {
        return forwardError<lvr2::ScanProjectPtr>(store.error());
    }
    return store.value().load();
}

storage::Result<lvr2::ScanProjectPtr> load_project(const std::string& uri,
                                                   const LoadOptions& options)
{
    auto store = open_project(uri, options);
    if (!store)
    {
        return forwardError<lvr2::ScanProjectPtr>(store.error());
    }
    return store.value().load();
}

storage::Status save_project(const std::string& uri,
                             const lvr2::ScanProject& project,
                             const SaveOptions& options,
                             const storage::StorageRegistry& registry)
{
    auto store = open_project(uri, loadOptionsFor(options), registry);
    if (!store)
    {
        return forwardStatus(store.error());
    }
    return store.value().save(project);
}

storage::Status save_project(const std::string& uri,
                             const lvr2::ScanProject& project,
                             const SaveOptions& options)
{
    auto store = open_project(uri, loadOptionsFor(options));
    if (!store)
    {
        return forwardStatus(store.error());
    }
    return store.value().save(project);
}

} // namespace lvr2::io::scan
