#include "lvr2/io/storage/ChunkStore.hpp"

#include "lvr2/io/storage.hpp"
#include "lvr2/io/MeshStores.hpp"
#include "lvr2/util/Hdf5Util.hpp"

#include <memory>

#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>

#include <functional>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace lvr2::io::storage
{
namespace
{

constexpr const char* kChunksName = "chunks";
constexpr const char* kAmountName = "amount";
constexpr const char* kChunkSizeName = "size";
constexpr const char* kBoundingBoxName = "bounding_box";

template<typename T>
std::shared_ptr<T[]> loadArray(const std::shared_ptr<HighFive::File>& file,
                                 const std::string& group,
                                 const std::string& name,
                                 std::vector<std::size_t>& dimensions)
{
    std::shared_ptr<T[]> result;
    if (!file || !file->isValid() || !hdf5util::exist(file, group))
    {
        return result;
    }

    auto hdf5Group = hdf5util::getGroup(file, group, false);
    if (!hdf5Group.exist(name))
    {
        return result;
    }

    auto dataset = hdf5Group.getDataSet(name);
    dimensions = dataset.getSpace().getDimensions();
    const std::size_t elementCount = std::accumulate(dimensions.begin(), dimensions.end(), std::size_t{1}, std::multiplies<std::size_t>());
    if (elementCount == 0)
    {
        return result;
    }

    result.reset(new T[elementCount]);
    dataset.read_raw(result.get());
    return result;
}

template<typename T>
void saveArray(const std::shared_ptr<HighFive::File>& file,
               const std::string& group,
               const std::string& name,
               std::vector<std::size_t> dimensions,
               std::shared_ptr<T[]> data)
{
    if (!file || !file->isValid())
    {
        throw std::runtime_error("chunk store is not open");
    }

    auto hdf5Group = hdf5util::getGroup(file, group, true);
    HighFive::DataSpace space(dimensions);
    HighFive::DataSetCreateProps properties;
    std::vector<hsize_t> chunks;
    chunks.reserve(dimensions.size());
    for (auto dimension : dimensions)
    {
        chunks.push_back(static_cast<hsize_t>(dimension));
    }
    if (!chunks.empty())
    {
        properties.add(HighFive::Chunking(chunks));
        properties.add(HighFive::Deflate(9));
    }

    auto dataset = hdf5util::createDataset<T>(hdf5Group, name, space, properties);
    dataset->write_raw(data.get());
    file->flush();
}

std::string chunkGroup(const std::string& layer, int x, int y, int z)
{
    return std::string(kChunksName) + "/" + layer + "/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z);
}

void requireStatus(const Status& status, const std::string& action)
{
    if (!status)
    {
        throw std::runtime_error(action + ": " + status.error().message);
    }
}

} // namespace

struct ChunkStore::State final
{
    std::string filename;
    std::shared_ptr<HighFive::File> file;
    std::unique_ptr<StorageBackend> backend;
    lvr2::io::mesh::Hdf5MeshStore meshStore;
};

ChunkStore::ChunkStore()
    : state_(new State)
{
}

ChunkStore::ChunkStore(const std::string& filename)
    : ChunkStore()
{
    open(filename);
}

ChunkStore::~ChunkStore() = default;
ChunkStore::ChunkStore(ChunkStore&&) noexcept = default;
ChunkStore& ChunkStore::operator=(ChunkStore&&) noexcept = default;

void ChunkStore::open(const std::string& filename)
{
    state_->filename = filename;
    auto registry = make_default_registry();
    OpenRequest request;
    request.uri = filename;
    request.kind = StorageKind::hdf5();
    auto opened = registry.open(request);
    if (!opened)
    {
        throw std::runtime_error("chunk store could not open HDF5 backend through StorageRegistry: "
                                 + opened.error().message);
    }
    state_->backend = std::move(opened.value());

    state_->file = hdf5util::open(filename);
    if (!state_->file || !state_->file->isValid())
    {
        throw std::runtime_error("chunk store HDF5 file is not valid");
    }
    state_->meshStore.open(filename);
}

void ChunkStore::saveAmount(lvr2::BaseVector<std::size_t> amount)
{
    std::shared_ptr<std::size_t[]> values(new std::size_t[3]{amount.x, amount.y, amount.z});
    saveArray(state_->file, kChunksName, kAmountName, {3, 1}, values);
}

void ChunkStore::saveChunkSize(float chunkSize)
{
    const float values[1]{chunkSize};
    requireStatus(state_->backend->writeFloatArray(
                      {kChunksName, kChunkSizeName},
                      {std::span<const float>(values), {1, 1}}),
                  "saving chunk size");
}

void ChunkStore::saveBoundingBox(lvr2::BoundingBox<lvr2::BaseVector<float>> boundingBox)
{
    const float values[6]{boundingBox.getMin()[0],
                          boundingBox.getMin()[1],
                          boundingBox.getMin()[2],
                          boundingBox.getMax()[0],
                          boundingBox.getMax()[1],
                          boundingBox.getMax()[2]};
    requireStatus(state_->backend->writeFloatArray(
                      {kChunksName, kBoundingBoxName},
                      {std::span<const float>(values), {2, 3}}),
                  "saving chunk bounding box");
}

void ChunkStore::save(lvr2::BaseVector<std::size_t> amount,
                      float chunkSize,
                      lvr2::BoundingBox<lvr2::BaseVector<float>> boundingBox)
{
    saveAmount(amount);
    saveChunkSize(chunkSize);
    saveBoundingBox(boundingBox);
}

lvr2::BaseVector<std::size_t> ChunkStore::loadAmount()
{
    std::vector<std::size_t> dimensions;
    auto values = loadArray<std::size_t>(state_->file, kChunksName, kAmountName, dimensions);
    if (!values || dimensions.empty() || dimensions[0] != 3)
    {
        throw std::runtime_error("chunk amount has invalid dimensions");
    }
    return {values[0], values[1], values[2]};
}

float ChunkStore::loadChunkSize()
{
    std::vector<std::size_t> dimensions;
    auto values = loadArray<float>(state_->file, kChunksName, kChunkSizeName, dimensions);
    if (!values)
    {
        throw std::runtime_error("chunk size is missing");
    }
    return values[0];
}

lvr2::BoundingBox<lvr2::BaseVector<float>> ChunkStore::loadBoundingBox()
{
    std::vector<std::size_t> dimensions;
    auto values = loadArray<float>(state_->file, kChunksName, kBoundingBoxName, dimensions);
    if (!values || dimensions.size() < 2 || dimensions[0] != 2 || dimensions[1] != 3)
    {
        throw std::runtime_error("chunk bounding box has invalid dimensions");
    }
    return {lvr2::BaseVector<float>(values[0], values[1], values[2]),
            lvr2::BaseVector<float>(values[3], values[4], values[5])};
}

void ChunkStore::saveMeshChunk(const lvr2::MeshBufferPtr& data, const std::string& layer, int x, int y, int z)
{
    auto group = hdf5util::getGroup(state_->file, chunkGroup(layer, x, y, z), true);
    state_->meshStore.save_mesh(group, data);
}

void ChunkStore::savePointChunk(const lvr2::PointBufferPtr& data, const std::string& layer, int x, int y, int z)
{
    requireStatus(state_->backend->writePointBuffer({chunkGroup(layer, x, y, z), "points"}, data),
                  "saving point chunk");}

lvr2::MeshBufferPtr ChunkStore::loadMeshChunk(const std::string& layer, int x, int y, int z)
{
    const auto groupName = chunkGroup(layer, x, y, z);
    if (!hdf5util::exist(state_->file, groupName))
    {
        return nullptr;
    }
    auto group = hdf5util::getGroup(state_->file, groupName, false);
    return state_->meshStore.load_mesh(group);
}

lvr2::PointBufferPtr ChunkStore::loadPointChunk(const std::string& layer, int x, int y, int z)
{
    const auto groupName = chunkGroup(layer, x, y, z);
    if (!hdf5util::exist(state_->file, groupName))
    {
        return nullptr;
    }
    auto loaded = state_->backend->readPointBuffer({groupName, "points"});
    if (!loaded)
    {
        if (loaded.error().code == ErrorCode::NotFound)
        {
            return nullptr;
        }
        throw std::runtime_error("loading point chunk: " + loaded.error().message);
    }
    return loaded.value();}

} // namespace lvr2::io::storage
