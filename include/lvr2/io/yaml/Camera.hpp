
#ifndef LVR2_IO_YAML_CAMERA_HPP
#define LVR2_IO_YAML_CAMERA_HPP

#include <sstream>

#include <yaml-cpp/yaml.h>

#include "lvr2/util/Timestamp.hpp"
#include <lvr2/util/Logging.hpp>
#include "lvr2/types/ScanTypes.hpp"
#include "lvr2/io/YAML.hpp"
#include "lvr2/util/YAMLUtil.hpp"

using lvr2::timestamp;

namespace YAML {

/**
 * YAML-CPPs convert specialization
 *
 * example:
 */

// WRITE SCAN PARTIALLY
template<>
struct convert<lvr2::Camera>
{

    /**
     * Encode Eigen matrix to yaml.
     */
    static Node encode(const lvr2::Camera& camera) {

        Node node;
        node["entity"] = lvr2::Camera::entity;
        node["type"] = lvr2::Camera::type;
        node["name"] = camera.name;
        node["transformation"] = camera.transformation;
        node["model"] = camera.model;

        return node;
    }

    static bool decode(const Node& node, lvr2::Camera& camera)
    {
        // Check if 'entity' and 'type' Tags are valid
        if (!YAML_UTIL::ValidateEntityAndType(node, "camera", lvr2::Camera::entity, lvr2::Camera::type))
        {
            // return false;
        }

        if(node["name"])
        {
            try {
                camera.name = node["name"].as<std::string>();
            } catch(const YAML::TypedBadConversion<std::string>& ex) {
                                lvr2::log::error("{}{}{}", "[YAML - Camera - decode] ERROR: Could not decode 'name': ", YAML_UTIL::NodeSummary(node["name"]), " as string");
                return false;
            }
        }
        else
        {
            camera.name = "";
        }

        if(node["transformation"])
        {
            try
            {
                camera.transformation = node["transformation"].as<lvr2::Transformd>();
            }
            catch(const YAML::TypedBadConversion<lvr2::Transformd>& ex)
            {
                                lvr2::log::error("{}{}{}", "[YAML - Camera - decode] ERROR: Could not decode 'transformation': ", YAML_UTIL::NodeSummary(node["transformation"]), " as Transformd");
                return false;
            }
        }

        if(node["model"])
        {
            try
            {
                camera.model = node["model"].as<decltype(camera.model)>();
            }
            catch(const YAML::TypedBadConversion<decltype(camera.model)>& ex)
            {
                                lvr2::log::error("{}{}{}", "[YAML - Camera - decode] ERROR: Could not decode 'model': ", YAML_UTIL::NodeSummary(node["model"]), " as CameraModel");
                return false;
            }
        }

        return true;
    }
};

}  // namespace YAML

#endif // LVR2_IO_YAML_PINHOLECAMERAMODEL_IO_HPP

