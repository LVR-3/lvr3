#pragma once

#include <yaml-cpp/yaml.h>
#include <lvr2/texture/Texture.hpp>
#include <lvr2/util/Logging.hpp>

namespace YAML {

/**
 * YAML-CPPs convert specialization
 *
 * example:
 */

template<>
struct convert<lvr2::Texture>
{

    static Node encode(const lvr2::Texture& texture) {

        Node node;
        node["index"]           = (int64_t) texture.m_index;
        node["width"]           = (int64_t) texture.m_width;
        node["height"]          = (int64_t) texture.m_height;
        node["num_channels"]    = (int64_t) texture.m_numChannels;
        node["channel_width"]   = (int64_t) texture.m_numBytesPerChan;
        node["texel_size"]      = (double_t) texture.m_texelSize;

        return node;
    }

    static bool decode(const Node& node, lvr2::Texture& texture)
    {
        if (!node["index"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'index'.");
            return false;
        }

        if (!node["width"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'width'.");
            return false;
        }

        if (!node["height"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'height'.");
            return false;
        }

        if (!node["num_channels"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'num_channels'.");
            return false;
        }

        if (!node["channel_width"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'channel_width'.");
            return false;
        }

        if (!node["texel_size"])
        {
                        lvr2::log::info("{}{}", "[YAML::convert<Texture> - decode] ", "Node has no tag 'texel_size'.");
            return false;
        }

        texture = lvr2::Texture(
            node["index"].as<int>(),
            node["width"].as<int>(),
            node["height"].as<int>(),
            node["num_channels"].as<int>(),
            node["channel_width"].as<int>(),
            node["texel_size"].as<double_t>()
        );

        return true;
    }
};

}  // namespace YAML
