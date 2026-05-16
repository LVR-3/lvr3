#pragma once

#include <yaml-cpp/yaml.h>
#include <lvr2/texture/Material.hpp>
#include <lvr2/util/Logging.hpp>

namespace YAML {

template<>
struct convert<lvr2::RGB8Color>
{
    static Node encode(const lvr2::RGB8Color& color)
    {

        Node node;
        node["r"] = (uint64_t) color[0];
        node["g"] = (uint64_t) color[1];
        node["b"] = (uint64_t) color[2];

        return node;
    }

    static bool decode(const Node& node, lvr2::RGB8Color& color)
    {
        if (!node["r"])
        {
                        lvr2::log::info("{}{}", fmt::streamed("[YAML::convert<Texture> - decode] "), fmt::streamed("Node has no tag 'r'."));
            return false;
        }
        if (!node["g"])
        {
                        lvr2::log::info("{}{}", fmt::streamed("[YAML::convert<Texture> - decode] "), fmt::streamed("Node has no tag 'g'."));
            return false;
        }
        if (!node["b"])
        {
                        lvr2::log::info("{}{}", fmt::streamed("[YAML::convert<Texture> - decode] "), fmt::streamed("Node has no tag 'b'."));
            return false;
        }

        color[0] = node["r"].as<uint64_t>();
        color[1] = node["g"].as<uint64_t>();
        color[2] = node["b"].as<uint64_t>();

        return true;
    }
};

template<>
struct convert<lvr2::Material>
{

    static Node encode(const lvr2::Material& material)
    {

        Node node;
        if (material.m_color)
        {
            node["color"] = *material.m_color;
        }
        else
        {
            node["color"] = lvr2::RGB8Color({255, 255, 255});
        }


        return node;
    }

    static bool decode(const Node& node, lvr2::Material& material)
    {
        if (!node["color"])
        {
                        lvr2::log::info("{}{}", fmt::streamed("[YAML::convert<Texture> - decode] "), fmt::streamed("Node has no tag 'color'."));
            return false;
        }

        lvr2::RGB8Color color = node["color"].as<lvr2::RGB8Color>();
        material.m_color = color;

        return true;
    }
};

}  // namespace YAML
