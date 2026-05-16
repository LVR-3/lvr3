#ifndef LVR2_IO_SCAN_PROJECT_STORE_HPP
#define LVR2_IO_SCAN_PROJECT_STORE_HPP

#include "lvr2/io/storage.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace lvr2
{

struct ScanProject;

} // namespace lvr2

namespace lvr2::io::scan
{

enum class SchemaKind
{
    Raw,
    RawPly,
    Hdf5
};

class Schema final
{
public:
    static Schema raw();
    static Schema raw_ply();
    static Schema hdf5();

    SchemaKind kind() const noexcept { return kind_; }

    friend bool operator==(const Schema& lhs, const Schema& rhs) noexcept
    {
        return lhs.kind_ == rhs.kind_;
    }

    friend bool operator!=(const Schema& lhs, const Schema& rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    explicit Schema(SchemaKind kind) : kind_(kind) {}

    SchemaKind kind_ = SchemaKind::RawPly;
};

struct LoadOptions
{
    storage::StorageKind kind = storage::StorageKind::directory();
    Schema schema = Schema::raw_ply();
    storage::LoadMode loadMode = storage::LoadMode::Lazy;

    static LoadOptions directory_raw();
    static LoadOptions directory_raw_ply();
    static LoadOptions hdf5();
};

struct SaveOptions
{
    storage::StorageKind kind = storage::StorageKind::directory();
    Schema schema = Schema::raw_ply();
    storage::LoadMode loadMode = storage::LoadMode::Lazy;

    static SaveOptions directory_raw();
    static SaveOptions directory_raw_ply();
    static SaveOptions hdf5();
};

class ProjectStore final
{
public:
    struct State;

    explicit ProjectStore(storage::StorageContext context,
                          Schema schema = Schema::raw_ply());
    ~ProjectStore();

    ProjectStore(ProjectStore&&) noexcept = default;
    ProjectStore& operator=(ProjectStore&&) noexcept = default;
    ProjectStore(const ProjectStore&) = delete;
    ProjectStore& operator=(const ProjectStore&) = delete;

    storage::Result<std::shared_ptr<lvr2::ScanProject>> load() const;
    storage::Status save(const lvr2::ScanProject& project) const;
    storage::Result<storage::MetaValue> load_meta() const;

private:
    std::shared_ptr<State> state_;
};

storage::Result<ProjectStore> open_project(const std::string& uri,
                                           const LoadOptions& options,
                                           const storage::StorageRegistry& registry);
storage::Result<ProjectStore> open_project(const std::string& uri,
                                           const LoadOptions& options = {});
storage::Result<ProjectStore> open_directory(const std::string& root,
                                             Schema schema = Schema::raw_ply(),
                                             storage::LoadMode loadMode = storage::LoadMode::Lazy);
storage::Result<ProjectStore> open_hdf5(const std::string& file,
                                        Schema schema = Schema::hdf5(),
                                        storage::LoadMode loadMode = storage::LoadMode::Lazy);

storage::Result<std::shared_ptr<lvr2::ScanProject>> load_project(const std::string& uri,
                                                                 const LoadOptions& options,
                                                                 const storage::StorageRegistry& registry);
storage::Result<std::shared_ptr<lvr2::ScanProject>> load_project(const std::string& uri,
                                                                 const LoadOptions& options = {});
storage::Status save_project(const std::string& uri,
                             const lvr2::ScanProject& project,
                             const SaveOptions& options,
                             const storage::StorageRegistry& registry);
storage::Status save_project(const std::string& uri,
                             const lvr2::ScanProject& project,
                             const SaveOptions& options = {});

static_assert(std::is_enum<SchemaKind>::value,
              "scan schema kind must remain an enum vocabulary");
static_assert(!std::is_convertible<SchemaKind, int>::value,
              "scan schema kind must not implicitly convert to integer values");
static_assert(std::is_move_constructible<ProjectStore>::value,
              "scan project store must remain movable");
static_assert(!std::is_copy_constructible<ProjectStore>::value,
              "scan project store owns an opened storage context and must not be copyable");
static_assert(std::is_default_constructible<LoadOptions>::value,
              "scan load options must remain default constructible");
static_assert(std::is_default_constructible<SaveOptions>::value,
              "scan save options must remain default constructible");

} // namespace lvr2::io::scan

#endif // LVR2_IO_SCAN_PROJECT_STORE_HPP
