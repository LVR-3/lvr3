#include <lvr2/io/scan.hpp>
#include <lvr2/io/storage.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <type_traits>
#include <vector>

namespace
{

struct HeaderOnlyBackend final : lvr2::io::storage::StorageBackend
{
    lvr2::io::storage::BackendInfo info() const override { return {}; }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::GroupKey&) const override
    {
        return false;
    }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::DataKey&) const override
    {
        return false;
    }

    lvr2::io::storage::Result<std::vector<std::string>> list(const lvr2::io::storage::GroupKey&) const override
    {
        return std::vector<std::string>{};
    }

    lvr2::io::storage::Result<lvr2::io::storage::MetaValue> readMeta(const lvr2::io::storage::MetaKey&) const override
    {
        return lvr2::io::storage::MetaValue{};
    }

    lvr2::io::storage::Status writeMeta(const lvr2::io::storage::MetaKey&,
                                         const lvr2::io::storage::MetaValue&) override
    {
        return {};
    }

    lvr2::io::storage::Result<lvr2::PointBufferPtr> readPointBuffer(const lvr2::io::storage::DataKey&) const override
    {
        return lvr2::PointBufferPtr{};
    }

    lvr2::io::storage::Status writePointBuffer(const lvr2::io::storage::DataKey&,
                                                const lvr2::PointBufferPtr&) override
    {
        return {};
    }

    lvr2::io::storage::Status writeFloatArray(const lvr2::io::storage::DataKey&,
                                               const lvr2::io::storage::FloatArrayView&) override
    {
        return {};
    }
};

struct MissingBackendOperations
{
    lvr2::io::storage::BackendInfo info() const { return {}; }
};

struct WrongFactoryReturn
{
    std::unique_ptr<lvr2::io::storage::StorageBackend> operator()(const lvr2::io::storage::OpenRequest&) const;
};

lvr2::io::storage::Result<std::unique_ptr<lvr2::io::storage::StorageBackend>> makeBackend(
    const lvr2::io::storage::OpenRequest&)
{
    return std::unique_ptr<lvr2::io::storage::StorageBackend>{};
}

} // namespace

int main()
{
    using namespace lvr2::io;

    static_assert(std::is_same<storage::Result<int>, tl::expected<int, storage::Error>>::value,
                  "storage result must use the approved expected backing");
    static_assert(std::is_same<storage::Status, tl::expected<void, storage::Error>>::value,
                  "storage status must use the approved expected backing");
    static_assert(storage::StorageBackendLike<HeaderOnlyBackend>,
                  "complete storage backends must satisfy the named backend concept");
    static_assert(!storage::StorageBackendLike<MissingBackendOperations>,
                  "incomplete storage backends must fail the named backend concept");
    static_assert(storage::StorageFactoryLike<storage::StorageFactory>,
                  "registry storage factory alias must satisfy the named factory concept");
    static_assert(storage::StorageFactoryLike<decltype(&makeBackend)>,
                  "free backend factory functions must satisfy the named factory concept");
    static_assert(!storage::StorageFactoryLike<WrongFactoryReturn>,
                  "factories must return the Result-wrapped runtime backend pointer");
    static_assert(!std::is_copy_constructible<storage::StorageContext>::value,
                  "storage context must keep unique backend ownership");
    static_assert(std::is_move_constructible<storage::StorageContext>::value,
                  "storage context must be movable");
    static_assert(!std::is_copy_constructible<scan::ProjectStore>::value,
                  "project store must keep unique opened storage ownership");
    static_assert(std::is_move_constructible<scan::ProjectStore>::value,
                  "project store must be movable");

    return 0;
}
