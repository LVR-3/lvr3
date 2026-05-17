#pragma once

#ifndef LVR2_TYPES_CUSTOMCHANNELTYPES_HPP
#define LVR2_TYPES_CUSTOMCHANNELTYPES_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

#include "ByteEncoding.hpp"

namespace lvr2 {

struct WaveformData {
    std::vector<uint16_t>   samples;
    uint16_t                echo_type;
    bool                    low_power;
};

template<>
std::shared_ptr<unsigned char[]> byteEncode(
    const WaveformData& data, size_t& bsize);

template<>
std::optional<WaveformData> byteDecode(
    const unsigned char* buffer, const size_t& bsize);




} // namespace lvr2

#endif // LVR2_TYPES_CUSTOMCHANNELTYPES_HPP