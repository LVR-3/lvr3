#include <lvr2/io/scan.hpp>
#include <lvr2/io/storage.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <memory>
#include <span>
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

    lvr2::io::storage::Result<std::size_t> readBytes(const lvr2::io::storage::DataKey&,
                                                     std::span<std::byte> output) const override
    {
        return output.size();
    }

    lvr2::io::storage::Status writeBytes(const lvr2::io::storage::DataKey&,
                                         std::span<const std::byte>) override
    {
        return {};
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
                                               const lvr2::io::storage::FloatArrayView& array) override
    {
        (void)array;
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

struct StatefulFactory
{
    int state = 0;

    lvr2::io::storage::Result<std::unique_ptr<lvr2::io::storage::StorageBackend>> operator()(
        const lvr2::io::storage::OpenRequest&) const
    {
        return std::unique_ptr<lvr2::io::storage::StorageBackend>{};
    }
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
    static_assert(storage::TypedDatasetView<storage::FloatArrayView>,
                  "float array views must expose a typed span and owned dimensions");
    static_assert(storage::DatasetReader<HeaderOnlyBackend>,
                  "storage readers must accept writable byte spans");
    static_assert(storage::DatasetWriter<HeaderOnlyBackend>,
                  "storage writers must accept read-only byte spans and typed array spans");
    static_assert(storage::StorageBackendLike<HeaderOnlyBackend>,
                  "complete storage backends must satisfy the named backend concept");
    static_assert(!storage::StorageBackendLike<MissingBackendOperations>,
                  "incomplete storage backends must fail the named backend concept");
    static_assert(std::is_pointer<storage::StorageFactory>::value,
                  "registry storage factory alias must remain a function pointer");
    static_assert(storage::StorageFactoryLike<storage::StorageFactory>,
                  "registry storage factory alias must satisfy the named factory concept");
    static_assert(storage::RegistryStorageFactory<storage::StorageFactory>,
                  "registry storage factory alias must be directly storable");
    static_assert(storage::StorageFactoryLike<decltype(&makeBackend)>,
                  "free backend factory functions must satisfy the named factory concept");
    static_assert(storage::RegistryStorageFactory<decltype(&makeBackend)>,
                  "free backend factory functions must be accepted by the registry");
    auto capturelessFactory = +[](const storage::OpenRequest&) -> storage::Result<std::unique_ptr<storage::StorageBackend>> {
        return std::unique_ptr<storage::StorageBackend>{};
    };
    static_assert(storage::RegistryStorageFactory<decltype(capturelessFactory)>,
                  "captureless factories must be accepted by the registry as function pointers");
    static_assert(!storage::StorageFactoryLike<WrongFactoryReturn>,
                  "factories must return the Result-wrapped runtime backend pointer");
    static_assert(storage::StorageFactoryLike<StatefulFactory>,
                  "stateful callables may model the callable shape");
    static_assert(!storage::RegistryStorageFactory<StatefulFactory>,
                  "registry factories must not require stateful type-erased storage");
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
