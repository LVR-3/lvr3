
#ifndef LVR2_IO_YAML_SCANMETA_IO_HPP
#define LVR2_IO_YAML_SCANMETA_IO_HPP

#include <yaml-cpp/yaml.h>
#include "lvr2/types/ScanTypes.hpp"
#include "lvr2/util/Timestamp.hpp"
#include <lvr2/util/Logging.hpp>
#include "lvr2/io/YAML.hpp"

using lvr2::timestamp;

namespace YAML
{

    /**
     * YAML-CPPs convert specialization
     *
     * example:
     */

    // WRITE SCAN PARTIALLY
    template <>
    struct convert<lvr2::Scan>
    {

        /**
         * Encode Eigen matrix to yaml.
         */
        static Node encode(const lvr2::Scan &scan)
        {

            // std::cout << "Encode Scan"

            Node node = scan.metadata;

            node["entity"] = lvr2::Scan::entity;
            node["type"] = lvr2::Scan::type;

            node["start_time"] = scan.startTime;
            node["end_time"] = scan.endTime;
            node["num_points"] = scan.numPoints;

            node["pose_estimation"] = scan.poseEstimation;
            node["transformation"] = scan.transformation;

            if (scan.boundingBox)
            {
                node["aabb"] = *scan.boundingBox;
            }

            if (scan.model)
            {
                node["model"] = *scan.model;
            }

            if (scan.points)
            {
                node["channels"] = Load("[]");
                for (auto elem : *scan.points)
                {
                    node["channels"].push_back(elem.first);
                }
            }

            return node;
        }

        static bool decode(const Node &node, lvr2::Scan &scan)
        {
            // Auskommentiert von der RDBX Gruppe
            //  Check if 'entity' and 'type' Tags are valid
            //        if (!YAML_UTIL::ValidateEntityAndType(node,
            //            "scan",
            //            lvr2::Scan::entity,
            //            lvr2::Scan::type))
            //        {
            //            return false;
            //        }
            Node parsed_node = node;

            if (parsed_node["start_time"])
            {
                try
                {
                    scan.startTime = parsed_node["start_time"].as<double>();
                }
                catch (const YAML::TypedBadConversion<double> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'start_time': "), fmt::streamed(parsed_node["start_time"]), fmt::streamed(" as double"));
                    return false;
                }
                parsed_node.remove("start_time");
            }
            else
            {
                scan.startTime = -1.0;
            }

            if (node["end_time"])
            {
                try
                {
                    scan.endTime = parsed_node["end_time"].as<double>();
                }
                catch (const YAML::TypedBadConversion<double> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'end_time': "), fmt::streamed(parsed_node["end_time"]), fmt::streamed(" as double"));
                    return false;
                }
                parsed_node.remove("end_time");
            }
            else
            {
                scan.endTime = -1.0;
            }

            if (node["num_points"])
            {
                try
                {
                    scan.numPoints = parsed_node["num_points"].as<unsigned int>();
                }
                catch (const YAML::TypedBadConversion<unsigned int> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'num_points': "), fmt::streamed(parsed_node["num_points"]), fmt::streamed(" as unsigned int"));
                    return false;
                }
                parsed_node.remove("num_points");
            }

            if (node["pose_estimation"])
            {
                try
                {
                    scan.poseEstimation = parsed_node["pose_estimation"].as<lvr2::Transformd>();
                }
                catch (const YAML::TypedBadConversion<lvr2::Transformd> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'pose_estimation': "), fmt::streamed(parsed_node["pose_estimation"]), fmt::streamed(" as Transformd"));
                    return false;
                }
                parsed_node.remove("pose_estimation");
            }
            else
            {
                scan.poseEstimation = lvr2::Transformd::Identity();
            }

            if (node["transformation"])
            {
                try
                {
                    scan.transformation = parsed_node["transformation"].as<lvr2::Transformd>();
                }
                catch (const YAML::TypedBadConversion<lvr2::Transformd> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'transformation': "), fmt::streamed(parsed_node["transformation"]), fmt::streamed(" as Transformd"));
                    return false;
                }
                parsed_node.remove("transformation");
            }
            else
            {
                scan.transformation = lvr2::Transformd::Identity();
            }

            if (node["model"])
            {
                try
                {
                    scan.model = parsed_node["model"].as<lvr2::SphericalModel>();
                }
                catch (const YAML::TypedBadConversion<lvr2::SphericalModel> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'model': "), fmt::streamed(parsed_node["model"]), fmt::streamed(" as SphericalModel"));
                    return false;
                }
                parsed_node.remove("model");
            }

            if (node["aabb"])
            {
                try
                {
                    scan.boundingBox = parsed_node["aabb"].as<lvr2::BoundingBox<lvr2::BaseVector<float>>>();
                }
                catch (const YAML::TypedBadConversion<lvr2::BoundingBox<lvr2::BaseVector<float>>> &ex)
                {
                                        lvr2::log::error("{}{}{}", fmt::streamed("[YAML - Scan - decode] ERROR: Could not decode 'aabb': "), fmt::streamed(parsed_node["aabb"]), fmt::streamed(" as BoundingBox"));
                    return false;
                }
                parsed_node.remove("aabb");
            }
            scan.metadata = parsed_node;

            return true;
        }
    };

} // namespace YAML

#endif // LVR2_IO_YAML_SCANMETA_IO_HPP
