#include <lvr2/io/storage.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{

using Key = std::pair<std::string, std::string>;

class SpanBackend final : public lvr2::io::storage::StorageBackend
{
public:
    explicit SpanBackend(std::string uri)
        : uri_(std::move(uri))
    {
    }

    lvr2::io::storage::BackendInfo info() const override
    {
        return {lvr2::io::storage::StorageKind::named("span-contract"), uri_, "span contract"};
    }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::GroupKey& key) const override
    {
        for (const auto& entry : bytes_)
        {
            if (entry.first.first == key.group)
            {
                return true;
            }
        }
        for (const auto& entry : floats_)
        {
            if (entry.first.first == key.group)
            {
                return true;
            }
        }
        return false;
    }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::DataKey& key) const override
    {
        const Key lookup{key.group, key.name};
        return bytes_.find(lookup) != bytes_.end() || floats_.find(lookup) != floats_.end();
    }

    lvr2::io::storage::Result<std::vector<std::string>> list(const lvr2::io::storage::GroupKey& key) const override
    {
        std::vector<std::string> names;
        for (const auto& entry : bytes_)
        {
            if (entry.first.first == key.group)
            {
                names.push_back(entry.first.second);
            }
        }
        for (const auto& entry : floats_)
        {
            if (entry.first.first == key.group)
            {
                names.push_back(entry.first.second);
            }
        }
        return names;
    }

    lvr2::io::storage::Result<std::size_t> readBytes(const lvr2::io::storage::DataKey& key,
                                                     std::span<std::byte> output) const override
    {
        const auto found = bytes_.find({key.group, key.name});
        if (found == bytes_.end())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::NotFound,
                                                  "byte dataset missing",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        if (output.size() < found->second.size())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::InvalidArgument,
                                                  "output byte span is too small",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        std::copy(found->second.begin(), found->second.end(), output.begin());
        return found->second.size();
    }

    lvr2::io::storage::Status writeBytes(const lvr2::io::storage::DataKey& key,
                                         std::span<const std::byte> bytes) override
    {
        bytes_[{key.group, key.name}] = std::vector<std::byte>(bytes.begin(), bytes.end());
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

    lvr2::io::storage::Status writeFloatArray(const lvr2::io::storage::DataKey& key,
                                              const lvr2::io::storage::FloatArrayView& array) override
    {
        const std::size_t expected = elementCount(array.dimensions);
        if (expected != array.values.size())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::InvalidArgument,
                                                  "float array span size does not match dimensions",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        floats_[{key.group, key.name}] = std::vector<float>(array.values.begin(), array.values.end());
        return {};
    }

    const std::vector<float>& storedFloats(const lvr2::io::storage::DataKey& key) const
    {
        return floats_.at({key.group, key.name});
    }

private:
    static std::size_t elementCount(const std::vector<std::size_t>& dimensions)
    {
        if (dimensions.empty())
        {
            return 0;
        }
        std::size_t count = 1;
        for (std::size_t dimension : dimensions)
        {
            count *= dimension;
        }
        return count;
    }

    std::string uri_;
    std::map<Key, std::vector<std::byte>> bytes_;
    std::map<Key, std::vector<float>> floats_;
};

lvr2::io::storage::Result<std::unique_ptr<lvr2::io::storage::StorageBackend>> openSpanBackend(
    const lvr2::io::storage::OpenRequest& request)
{
    return std::unique_ptr<lvr2::io::storage::StorageBackend>(new SpanBackend(request.uri));
}

bool check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

} // namespace

namespace lvr2::io::storage
{

Status register_default_backends(StorageRegistry&)
{
    return {};
}

} // namespace lvr2::io::storage

int main()
{
    using namespace lvr2::io::storage;

    static_assert(StorageBackendLike<SpanBackend>,
                  "span test backend must use the same runtime backend contract");
    static_assert(RegistryStorageFactory<decltype(&openSpanBackend)>,
                  "test backend factory must use the same registry factory contract");

    StorageRegistry registry;
    if (!check(static_cast<bool>(registry.add(StorageKind::named("span-contract"), openSpanBackend)),
               "span backend registration failed"))
    {
        return 1;
    }

    OpenRequest request;
    request.uri = "memory://span-contract";
    request.kind = StorageKind::named("span-contract");
    auto opened = registry.open(request);
    if (!check(static_cast<bool>(opened), "span backend open failed"))
    {
        return 1;
    }

    StorageBackend& backend = *opened.value();
    const DataKey byteKey{"group", "bytes"};
    std::array<std::byte, 4> input{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    if (!check(static_cast<bool>(backend.writeBytes(byteKey, std::span<const std::byte>(input.data(), input.size()))),
               "byte span write failed"))
    {
        return 1;
    }
    input[0] = std::byte{9};

    std::array<std::byte, 4> output{};
    auto read = backend.readBytes(byteKey, std::span<std::byte>(output.data(), output.size()));
    if (!check(static_cast<bool>(read), "byte span read failed") ||
        !check(read.value() == output.size(), "byte span read returned wrong size") ||
        !check(output[0] == std::byte{1} && output[3] == std::byte{4}, "byte span was not copied on write"))
    {
        return 1;
    }

    std::array<std::byte, 2> tooSmall{};
    auto undersized = backend.readBytes(byteKey, std::span<std::byte>(tooSmall.data(), tooSmall.size()));
    if (!check(!undersized, "undersized byte read unexpectedly succeeded") ||
        !check(undersized.error().code == ErrorCode::InvalidArgument,
               "undersized byte read used the wrong error code"))
    {
        return 1;
    }

    auto* spanBackend = dynamic_cast<SpanBackend*>(&backend);
    if (!check(spanBackend != nullptr, "opened backend did not use the registry-created span backend"))
    {
        return 1;
    }

    const DataKey floatKey{"group", "floats"};
    std::array<float, 3> floats{1.0f, 2.0f, 3.0f};
    FloatArrayView floatView{std::span<const float>(floats.data(), floats.size()), {3, 1}};
    if (!check(static_cast<bool>(backend.writeFloatArray(floatKey, floatView)),
               "float span write failed"))
    {
        return 1;
    }
    floats[0] = 42.0f;
    const auto& storedFloats = spanBackend->storedFloats(floatKey);
    if (!check(storedFloats.size() == 3, "float span write stored wrong size") ||
        !check(storedFloats[0] == 1.0f && storedFloats[2] == 3.0f,
               "float span was not copied on write"))
    {
        return 1;
    }

    FloatArrayView invalidFloatView{std::span<const float>(floats.data(), 2), {3, 1}};
    auto invalidFloatWrite = backend.writeFloatArray(floatKey, invalidFloatView);
    if (!check(!invalidFloatWrite, "mismatched float span unexpectedly succeeded") ||
        !check(invalidFloatWrite.error().code == ErrorCode::InvalidArgument,
               "mismatched float span used the wrong error code"))
    {
        return 1;
    }

    return 0;
}
