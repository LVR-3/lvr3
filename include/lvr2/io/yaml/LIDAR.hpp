#ifndef LVR2_IO_YAML_LIDAR_HPP
#define LVR2_IO_YAML_LIDAR_HPP

#include "lvr2/io/YAML.hpp"
#include <lvr2/util/Logging.hpp>

using lvr2::timestamp;

namespace YAML {

/**
 * YAML-CPPs convert specialization
 *
 * example:
 */

// WRITE LIDAR META
template <>
struct convert<lvr2::LIDAR>
{
    static Node encode(const lvr2::LIDAR& lidar) {
        Node node;
        node["entity"] = lvr2::LIDAR::entity;
        node["type"] = lvr2::LIDAR::type;
        node["transformation"] = lidar.transformation;
        node["name"] = lidar.name;
        node["model"] = lidar.model;

        if(lidar.boundingBox)
        {
            node["aabb"] = *lidar.boundingBox;
        }

        return node;
    }

    static bool decode(const Node& node, lvr2::LIDAR& lidar)
    {
        if (!YAML_UTIL::ValidateEntityAndType(node,
            "lidar",
            lvr2::LIDAR::entity,
            lvr2::LIDAR::type))
        {
            // return false;
        }

        if(node["transformation"])
        {
            try
            {
                lidar.transformation = node["transformation"].as<lvr2::Transformd>();
            }
            catch(const YAML::TypedBadConversion<lvr2::Transformd>& ex)
            {
                                lvr2::log::error("{}{}{}", fmt::streamed("[YAML - LIDAR - decode] ERROR: Could not decode 'transformation': "), fmt::streamed(node["transformation"]), fmt::streamed(" as Transformd"));
                return false;
            }
        }
        else
        {
            lidar.transformation = lvr2::Transformd::Identity();
        }

        if(node["name"])
        {
            try
            {
                lidar.name = node["name"].as<std::string>();
            }
            catch(const YAML::TypedBadConversion<std::string>& ex)
            {
                                lvr2::log::error("{}{}{}", fmt::streamed("[YAML - LIDAR - decode] ERROR: Could not decode 'name': "), fmt::streamed(node["name"]), fmt::streamed(" as string"));
                return false;
            }
        }

        if(node["model"])
        {
            try
            {
                lidar.model = node["model"].as<lvr2::SphericalModel>();
            }
            catch(const YAML::TypedBadConversion<lvr2::SphericalModel>& ex)
            {
                                lvr2::log::error("{}{}{}", fmt::streamed("[YAML - LIDAR - decode] ERROR: Could not decode 'model': "), fmt::streamed(node["model"]), fmt::streamed(" as SphericalModel"));
                return false;
            }
        }
        else
        {
            lidar.model.range[0] = 0.0;
        }

        if(node["aabb"])
        {
            try
            {
                lidar.boundingBox = node["aabb"].as<lvr2::BoundingBox<lvr2::BaseVector<float> > >();
            }
            catch(const YAML::TypedBadConversion<lvr2::BoundingBox<lvr2::BaseVector<float> > >& ex)
            {
                                lvr2::log::error("{}{}{}", fmt::streamed("[YAML - LIDAR - decode] ERROR: Could not decode 'aabb': "), fmt::streamed(node["aabb"]), fmt::streamed(" as BoundingBox"));
                return false;
            }
        }

        return true;
    }
};

} // namespace YAML

#endif // LVR2_IO_YAML_LIDAR_HPP
