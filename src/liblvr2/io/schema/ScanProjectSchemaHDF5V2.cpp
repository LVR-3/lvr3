#include <sstream>
#include <iomanip>

#include "lvr2/io/schema/ScanProjectSchemaHDF5V2.hpp"
#include "lvr2/io/IOUtils.hpp"
#include "lvr2/io/YAML.hpp"
#include "lvr2/types/ScanTypes.hpp"

#include <filesystem>

namespace lvr2
{

Description ScanProjectSchemaHDF5V2::scanProject() const
{
    Description d;
    d.groupName = "raw";           // All data is saved in the root dir
    d.dataSetName = std::nullopt;    // No dataset name for project root
    d.metaData = std::nullopt;       // No metadata for project
    d.metaName = "meta";
    return d;
}
Description ScanProjectSchemaHDF5V2::position(const size_t &scanPosNo) const
{
    Description d_parent = scanProject();

    Description d;

    // Save scan file name
    std::stringstream sstr;
    sstr << std::setfill('0') << std::setw(8) << scanPosNo;

    d.dataSetName = std::nullopt;
    d.metaName = "meta";
    d.metaData = std::nullopt;

    // Load meta data
    std::filesystem::path positionPath(sstr.str());

    // append positionPath to parent path if necessary
    if(d_parent.groupName)
    {
        positionPath = std::filesystem::path(*d_parent.groupName) / positionPath;
    }

    std::filesystem::path metaPath(*d.metaName);
    d.groupName = (positionPath).string();

    return d;
}

Description ScanProjectSchemaHDF5V2::scan(const size_t &scanPosNo, const size_t &scanNo) const
{
    // Get information about scan the associated scan position
    Description d_parent = position(scanPosNo);
    return scan(*d_parent.groupName, scanNo);
}

Description ScanProjectSchemaHDF5V2::scan(const std::string& scanPositionPath, const size_t &scanNo) const
{
    std::stringstream sstr;
    sstr << std::setfill('0') << std::setw(8) << scanNo;

    Description d;
    std::filesystem::path scansPath = std::filesystem::path(scanPositionPath) / "scans";
    std::filesystem::path scanPath = scansPath / sstr.str();

    d.groupName = scanPath.string();
    d.dataSetName = "data";
    d.metaName = "meta";

    return d;
}

Description ScanProjectSchemaHDF5V2::scanCamera(const size_t &scanPositionNo, const size_t &camNo) const
{
    // Group name
    Description d_parent = position(scanPositionNo);
    return scanCamera(*d_parent.groupName, camNo);
}

Description ScanProjectSchemaHDF5V2::scanCamera(const std::string &scanPositionPath, const size_t &camNo) const
{
    Description d;

    // Construct group path
    std::stringstream sstr;
    sstr << std::setfill('0') << std::setw(8) << camNo;

    std::filesystem::path positionPath(scanPositionPath);
    std::filesystem::path camerasPath("cameras");
    std::filesystem::path groupPath = scanPositionPath / camerasPath;

    std::filesystem::path camPath(sstr.str());
    d.groupName = (groupPath / camPath).string();

    // No data set information for camera position
    d.dataSetName = "data";
    d.metaName = "meta";
    d.metaData = std::nullopt;

    return d;
}

virtual Description cameraImage(
    const size_t &scanPosNo,
    const size_t &groupNo,
    const size_t &camNo,
    const size_t &imgNo) const
{
    Description d;
    return d;
}

virtual Description cameraImageGroup(
    const size_t &scanPosNo,
    const size_t &camNo,
    const size_t &GroupNo) const
{
    Description d;
    return d;
}

Description ScanProjectSchemaHDF5V2::scanImage(
    const size_t &scanPosNo,
    const size_t &scanCameraNo,
    const size_t &scanImageNo) const
{
    // Scan images are not supported
    Description d_cam = scanCamera(scanPosNo, scanCameraNo);
    return scanImage((*d_cam.groupName + "/" + *d_cam.dataSetName) , scanImageNo);
}

Description ScanProjectSchemaHDF5V2::scanImage(
    const std::string &scanImagePath, const size_t &scanImageNo) const
{
    Description d;

    // data/images/%8d
    // data/metas/%8d

    std::filesystem::path siPath(scanImagePath);
    std::filesystem::path iPath("images");
    std::filesystem::path mPath("meta");

    std::stringstream sstr;
    sstr << std::setfill('0') << std::setw(8) << scanImageNo;

    std::string imgName(sstr.str());
    std::string metaName(sstr.str());

    d.groupName = (siPath).string();
    d.dataSetName = (iPath / imgName).string();
    d.metaName = (mPath / metaName).string();
    d.metaData = std::nullopt;

    return d;
}

} // namespace lvr2
