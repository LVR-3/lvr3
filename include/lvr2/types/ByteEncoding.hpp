#ifndef LVR2_TYPES_BYTE_ENCODING_HPP
#define LVR2_TYPES_BYTE_ENCODING_HPP

#include <cstddef>
#include <memory>
#include <optional>

namespace lvr2 {

/**
 * @brief Specialize this method in CustomChannelTypes.cpp for your CustomType.
 *        Only if specialized, datatype can be stored.
 *
 * @tparam T
 * @param data
 * @param bsize
 * @return std::shared_ptr<unsigned char[]>
 */
template<typename T>
std::shared_ptr<unsigned char[]> byteEncode(
    const T& data, size_t& bsize);

// default
template<typename T>
std::shared_ptr<unsigned char[]> byteEncode(const T& data, size_t& bsize)
{
    std::shared_ptr<unsigned char[]> ret;
    return ret;
}

template<typename T>
std::optional<T> byteDecode(const unsigned char* buffer, const size_t& bsize);

template<typename T>
std::optional<T> byteDecode(const unsigned char* buffer, const size_t& bsize)
{
    std::optional<T> ret;
    return ret;
}

} // namespace lvr2

#endif // LVR2_TYPES_BYTE_ENCODING_HPP
