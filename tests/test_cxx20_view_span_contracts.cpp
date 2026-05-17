#include <lvr2/types/BaseBuffer.hpp>
#include <lvr2/types/Channel.hpp>
#include <lvr2/attrmaps/HashMap.hpp>

#include <concepts>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
int failIf(bool condition, int code)
{
    return condition ? code : 0;
}
} // namespace

int main()
{
    using lvr2::BaseBuffer;
    using lvr2::Channel;

    static_assert(std::same_as<decltype(std::declval<Channel<float>&>().values()), std::span<float>>);
    static_assert(std::same_as<decltype(std::declval<const Channel<float>&>().values()), std::span<const float>>);
    static_assert(std::same_as<Channel<float>::DataPtr, std::shared_ptr<float[]>>);

    std::vector<float> source{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};
    Channel<float> channel(2, 3, std::span<const float>(source.data(), source.size()));
    source[0] = 99.0F;
    if (int failure = failIf(channel.values()[0] != 1.0F, 1)) return failure;

    std::span<float> channelValues = channel.values();
    channelValues[1] = 8.0F;
    if (int failure = failIf(channel.dataPtr()[1] != 8.0F, 2)) return failure;

    auto shared = std::shared_ptr<float[]>(new float[3]{7.0F, 8.0F, 9.0F});
    Channel<float> sharedChannel(1, 3, shared);
    shared[0] = 10.0F;
    if (int failure = failIf(sharedChannel.values()[0] != 10.0F, 12)) return failure;

    bool rejectedWrongSize = false;
    try
    {
        Channel<float> invalid(2, 3, std::span<const float>(source.data(), source.size() - 1));
        (void)invalid;
    }
    catch(const std::invalid_argument&)
    {
        rejectedWrongSize = true;
    }
    if (int failure = failIf(!rejectedWrongSize, 3)) return failure;

    BaseBuffer buffer;
    std::string pointsName = "points";
    buffer.addFloatChannel(std::span<const float>(source.data(), source.size()),
                           std::string_view(pointsName),
                           2,
                           3);
    pointsName.assign("mutated");
    if (int failure = failIf(!buffer.hasFloatChannel("points"), 4)) return failure;
    if (int failure = failIf(buffer.hasFloatChannel("mutated"), 5)) return failure;

    std::size_t n = 0;
    std::size_t width = 0;
    auto points = buffer.getFloatArray(std::string_view("points"), n, width);
    if (int failure = failIf(!points, 6)) return failure;
    if (int failure = failIf(n != 2, 7)) return failure;
    if (int failure = failIf(width != 3, 8)) return failure;
    if (int failure = failIf(points[0] != 99.0F, 9)) return failure;

    std::vector<unsigned int> indices{0U, 1U, 2U};
    std::string indicesName = "indices";
    buffer.addIndexChannel(std::span<const unsigned int>(indices.data(), indices.size()),
                           std::string_view(indicesName),
                           1,
                           3);
    indicesName.clear();
    if (int failure = failIf(!buffer.hasIndexChannel("indices"), 10)) return failure;
    if (int failure = failIf(buffer.indexChannelWidth("indices") != 3, 11)) return failure;

    lvr2::HashMap<lvr2::VertexHandle, int> map;
    map.insert(lvr2::VertexHandle(0), 3);
    auto value = map.get(lvr2::VertexHandle(0));
    if (int failure = failIf(!value, 13)) return failure;
    value->get() = 4;
    auto updated = map.get(lvr2::VertexHandle(0));
    if (int failure = failIf(!updated || updated->get() != 4, 14)) return failure;

    return 0;
}
