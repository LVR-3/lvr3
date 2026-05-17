#ifndef LVR2_IO_STORAGE_STORAGE_BACKEND_HPP
#define LVR2_IO_STORAGE_STORAGE_BACKEND_HPP

#include <tl/expected.hpp>

#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace lvr2
{

class PointBuffer;
using PointBufferPtr = std::shared_ptr<PointBuffer>;

} // namespace lvr2

namespace lvr2::io::storage
{

enum class ErrorCode
{
    None,
    InvalidArgument,
    EmptyPath,
    DuplicateKind,
    UnknownKind,
    OpenFailed,
    NotFound,
    ReadFailed,
    WriteFailed,
    Unsupported
};

struct Error
{
    ErrorCode code = ErrorCode::None;
    std::string message;
    std::string uri;
    std::string group;
    std::string name;
};

template<class T>
using Result = tl::expected<T, Error>;

using Status = tl::expected<void, Error>;

enum class LoadMode
{
    Lazy,
    Eager
};

class StorageKind final
{
public:
    StorageKind() = default;

    static StorageKind auto_detect();
    static StorageKind directory();
    static StorageKind hdf5();
    static StorageKind named(std::string name);

    const std::string& name() const noexcept { return name_; }
    bool is_auto() const noexcept { return name_.empty(); }

    friend bool operator==(const StorageKind& lhs, const StorageKind& rhs) noexcept
    {
        return lhs.name_ == rhs.name_;
    }

    friend bool operator!=(const StorageKind& lhs, const StorageKind& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const StorageKind& lhs, const StorageKind& rhs) noexcept
    {
        return lhs.name_ < rhs.name_;
    }

private:
    explicit StorageKind(std::string name) : name_(std::move(name)) {}

    std::string name_;
};

struct GroupKey
{
    std::string group;
};

struct DataKey
{
    std::string group;
    std::string name;
};

struct MetaKey
{
    std::string group;
    std::string name;
};

struct MetaValue
{
    std::string text;
};

template<class T>
struct TypedArrayView
{
    using value_type = T;

    std::span<const T> values;
    std::vector<std::size_t> dimensions;
};

using FloatArrayView = TypedArrayView<float>;

struct Hdf5OpenOptions
{
    unsigned int compressionLevel = 9;
};

struct OpenRequest
{
    std::string uri;
    StorageKind kind = StorageKind::auto_detect();
    LoadMode loadMode = LoadMode::Lazy;
    Hdf5OpenOptions hdf5;
};

struct BackendInfo
{
    StorageKind kind;
    std::string uri;
    std::string description;
};

class StorageBackend
{
public:
    virtual ~StorageBackend() = default;

    virtual BackendInfo info() const = 0;
    virtual Result<bool> exists(const GroupKey& key) const = 0;
    virtual Result<bool> exists(const DataKey& key) const = 0;
    virtual Result<std::vector<std::string>> list(const GroupKey& key) const = 0;
    virtual Result<std::size_t> readBytes(const DataKey& key, std::span<std::byte> output) const = 0;
    virtual Status writeBytes(const DataKey& key, std::span<const std::byte> bytes) = 0;
    virtual Result<MetaValue> readMeta(const MetaKey& key) const = 0;
    virtual Status writeMeta(const MetaKey& key, const MetaValue& value) = 0;
    virtual Result<lvr2::PointBufferPtr> readPointBuffer(const DataKey& key) const = 0;
    virtual Status writePointBuffer(const DataKey& key, const lvr2::PointBufferPtr& buffer) = 0;
    virtual Status writeFloatArray(const DataKey& key, const FloatArrayView& array) = 0;
};

using StorageFactory = Result<std::unique_ptr<StorageBackend>> (*)(const OpenRequest&);

template<class View>
concept TypedDatasetView = requires(const View& view)
{
    typename View::value_type;
    { view.values } -> std::convertible_to<std::span<const typename View::value_type>>;
    { view.dimensions } -> std::same_as<const std::vector<std::size_t>&>;
};

template<class Backend>
concept DatasetReader = requires(const Backend& constBackend,
                                 const DataKey& dataKey,
                                 std::span<std::byte> outputBytes)
{
    { constBackend.readBytes(dataKey, outputBytes) } -> std::same_as<Result<std::size_t>>;
};

template<class Backend>
concept DatasetWriter = requires(Backend& backend,
                                 const DataKey& dataKey,
                                 std::span<const std::byte> inputBytes,
                                 const FloatArrayView& floatArray)
{
    { backend.writeBytes(dataKey, inputBytes) } -> std::same_as<Status>;
    { backend.writeFloatArray(dataKey, floatArray) } -> std::same_as<Status>;
};

template<class Backend>
concept StorageBackendLike = DatasetReader<Backend> &&
                             DatasetWriter<Backend> &&
                             requires(Backend& backend,
                                      const Backend& constBackend,
                                      const GroupKey& groupKey,
                                      const DataKey& dataKey,
                                      const MetaKey& metaKey,
                                      const MetaValue& metaValue,
                                      const lvr2::PointBufferPtr& pointBuffer,
                                      const FloatArrayView& floatArray)
{
    { constBackend.info() } -> std::same_as<BackendInfo>;
    { constBackend.exists(groupKey) } -> std::same_as<Result<bool>>;
    { constBackend.exists(dataKey) } -> std::same_as<Result<bool>>;
    { constBackend.list(groupKey) } -> std::same_as<Result<std::vector<std::string>>>;
    { constBackend.readMeta(metaKey) } -> std::same_as<Result<MetaValue>>;
    { backend.writeMeta(metaKey, metaValue) } -> std::same_as<Status>;
    { constBackend.readPointBuffer(dataKey) } -> std::same_as<Result<lvr2::PointBufferPtr>>;
    { backend.writePointBuffer(dataKey, pointBuffer) } -> std::same_as<Status>;
    { backend.writeFloatArray(dataKey, floatArray) } -> std::same_as<Status>;
};

template<class Factory>
concept StorageFactoryLike = requires(Factory factory, const OpenRequest& request)
{
    { factory(request) } -> std::same_as<Result<std::unique_ptr<StorageBackend>>>;
};

template<class Factory>
concept RegistryStorageFactory = StorageFactoryLike<Factory> &&
                                 std::convertible_to<Factory, StorageFactory>;

static_assert(TypedDatasetView<FloatArrayView>,
              "storage typed array view must expose an element span plus owned dimensions");
static_assert(StorageBackendLike<StorageBackend>,
              "storage backend concept must match the runtime backend interface");
static_assert(StorageFactoryLike<StorageFactory>,
              "storage factory concept must match the runtime registry factory interface");
static_assert(RegistryStorageFactory<StorageFactory>,
              "registry factories must be stateless function-pointer-compatible callables");
static_assert(std::is_pointer_v<StorageFactory>,
              "storage registry stores simple function pointers, not type-erased factory wrappers");

class StorageRegistry final
{
public:
    template<RegistryStorageFactory Factory>
    Status add(StorageKind kind, Factory factory)
    {
        return addFactory(std::move(kind), static_cast<StorageFactory>(factory));
    }

    Result<std::unique_ptr<StorageBackend>> open(const OpenRequest& request) const;

private:
    Status addFactory(StorageKind kind, StorageFactory factory);

    std::map<StorageKind, StorageFactory> factories_;
};

struct StorageContext
{
    std::unique_ptr<StorageBackend> backend;
    LoadMode loadMode = LoadMode::Lazy;

    StorageContext() = default;
    StorageContext(std::unique_ptr<StorageBackend> backend,
                   LoadMode loadMode = LoadMode::Lazy);
    StorageContext(StorageContext&&) noexcept = default;
    StorageContext& operator=(StorageContext&&) noexcept = default;
    StorageContext(const StorageContext&) = delete;
    StorageContext& operator=(const StorageContext&) = delete;
};

inline tl::unexpected<Error> unexpected(Error error)
{
    return tl::unexpected<Error>(std::move(error));
}

Status register_default_backends(StorageRegistry& registry);
StorageRegistry make_default_registry();

static_assert(std::is_enum_v<ErrorCode>,
              "storage error code must remain an enum vocabulary");
static_assert(!std::is_convertible_v<ErrorCode, int>,
              "storage error code must not implicitly convert to integer values");
static_assert(std::is_enum_v<LoadMode>,
              "storage load mode must remain an enum vocabulary");
static_assert(!std::is_convertible_v<LoadMode, int>,
              "storage load mode must not implicitly convert to integer values");
static_assert(std::is_same_v<Result<lvr2::PointBufferPtr>,
                             tl::expected<lvr2::PointBufferPtr, Error>>,
              "storage result must stay backed by tl::expected");
static_assert(std::is_default_constructible_v<FloatArrayView>,
              "storage float-array views must remain simple non-owning value options");
static_assert(std::is_same_v<Status, tl::expected<void, Error>>,
              "storage status must stay backed by tl::expected");
static_assert(!std::is_copy_constructible_v<StorageContext>,
              "storage context owns its opened backend and must not be copyable");
static_assert(std::is_move_constructible_v<StorageContext>,
              "storage context must remain movable");

} // namespace lvr2::io::storage

#endif // LVR2_IO_STORAGE_STORAGE_BACKEND_HPP
