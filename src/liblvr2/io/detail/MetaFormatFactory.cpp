#include "lvr2/types/ScanTypes.hpp"
#include "lvr2/io/detail/MetaFormatFactory.hpp"
#include "lvr2/io/YAML.hpp"
#include "lvr2/util/IOUtils.hpp"
#include "lvr2/util/YAMLUtil.hpp"

#include <unordered_set>
#include <lvr2/util/Logging.hpp>

namespace lvr2
{

bool isMetaFile(const std::string& filename)
{
    std::filesystem::path p(filename);
    std::unordered_set<std::string> metaExtensions = {".yaml", ".slam6d", ".frames" , ".json" , ".pose" , ".scn", ".img"};
    return metaExtensions.find(p.extension().string()) != metaExtensions.end();
}

void saveMetaInformation(const std::string &outfile, const YAML::Node &node)
{
    std::filesystem::path p(outfile);

    if(p.extension() == "")
    {
        p += ".yaml";
    }

    if (p.extension() == ".yaml")
    {
        std::ofstream out(p.string().c_str());
        out << node;
        out.close();
    }
    else if (p.extension() == ".slam6d")
    {
        std::filesystem::path dir = p.parent_path();
        std::string filename = p.stem().string();
        std::filesystem::path posePath = dir / (filename + ".pose");
        std::filesystem::path framesPath = dir / (filename + ".frames");

        ScanPosition sp;

        if(YAML::convert<ScanPosition>::decode(node, sp))
        {
            // Is scan position
            writePose(sp.poseEstimation, posePath);
            writeFrame(sp.transformation, framesPath);
        } else {
            // what to do here?
        }
    } else {
                lvr2::log::warning("{}{}{}", "[MetaFormatFactory] Meta extension ", p.extension().string(), " unknown. ");
    }
}

YAML::Node loadMetaInformation(const std::string &in)
{
    std::filesystem::path inPath(in);

    if(inPath.extension() == "")
    {
        inPath += ".yaml";
    }

    if (inPath.extension() == ".yaml" || inPath.extension() == ".json" || inPath.extension() == ".pose" || inPath.extension() == ".img" || inPath.extension() == ".scn" )
    {
        YAML::Node n;
        if (std::filesystem::exists(inPath))
        {
            n = YAML::LoadFile(inPath.string());
        }
        else
        {
                        lvr2::log::error("{}{}", "[MetaFormatFactory] LoadMetaInformation(YAML): Unable to find yaml file: ", inPath.string());
        }
        return n;
    }

    else if (inPath.extension() == ".slam6d")
    {
        YAML::Node node;

        std::filesystem::path dir = inPath.parent_path();
        std::string filename = inPath.stem().string();
        std::filesystem::path posePath = dir / (filename + ".pose");
        std::filesystem::path framesPath = dir / (filename + ".frames");

        bool pose_exist = std::filesystem::exists(posePath);
        bool frames_exist = std::filesystem::exists(framesPath);

        if(!pose_exist && !frames_exist)
        {
            return node;
        }

        // assuming slam6d works on scanPostition level?
        ScanPosition sp;

        if (pose_exist)
        {
            sp.poseEstimation = getTransformationFromPose<double>(posePath);
        }
        else
        {
                        lvr2::log::warning("{}", "[MetaFormatFactory] LoadMetaInformation(SLAM6D): Warning: No pose file found.");
        }

        if (frames_exist)
        {
            sp.transformation = getTransformationFromFrames<double>(framesPath);
        }
        else
        {
            // node frames found. taking poseEstimate as transformation
            sp.transformation = sp.poseEstimation;

                        lvr2::log::warning("{}", "[MetaFormatFactory] LoadMetaInformation(SLAM6D): Warning: No frames file found.");
        }

        node = sp;

        return node;
    }
    else if(inPath.extension() == ".plyschema")
    {
        // Actually like .slam6d this is just a tag to make the loader
        // think we have meta data but in this schema we just take
        // the already transformed pointcloud so we just have to
        // return empty scan position meta information
        YAML::Node node;

        return node;
    }
    else
    {
                lvr2::log::error("{}{}{}", "[MetaFormatFactory] Meta extension ", inPath.extension().string(), " unknown. ");
        YAML::Node node;
        return node;
    }
}

} // namespace lvr2
