#include "lvr2/util/ScanProjectUtils.hpp"
#include "lvr2/util/Logging.hpp"
#include "lvr2/io/schema/ScanProjectSchemaEuRoC.hpp"
#include "lvr2/io/schema/ScanProjectSchemaRaw.hpp"
#include "lvr2/io/schema/ScanProjectSchemaHyperlib.hpp"
// #include "lvr2/io/schema/ScanProjectSchemaOusterPLY.hpp"
#include "lvr2/io/schema/ScanProjectSchemaSlam6D.hpp"
#include "lvr2/io/schema/ScanProjectSchemaHDF5.hpp"
#include "lvr2/io/schema/ScanProjectSchemaHDF5V2.hpp"

#ifdef LVR2_USE_RDB
#include "lvr2/io/schema/ScanProjectSchemaRdbx.hpp"
#endif

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace lvr2
{

DirectorySchemaPtr directorySchemaFromName(const std::string& schemaName, const std::string& rootDirectory)
{
    // Check root directory
    if(!boost::filesystem::is_directory(boost::filesystem::path(rootDirectory)))
    {
                lvr2::log::warning("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Cannot create directory schema. Given root is not a directory: '"), fmt::streamed(rootDirectory), fmt::streamed("'."));
        return nullptr;
    }

    std::string name = boost::to_upper_copy<std::string>(schemaName);

    if(name == "EUROC")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Creating ScanProjectSchemaEuRoC with root directory '"), fmt::streamed(rootDirectory), fmt::streamed("."));
        return DirectorySchemaPtr(new ScanProjectSchemaEuRoC(rootDirectory));
    }
    else if (name == "HYPERLIB")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Creating ScanProjectSchemaRaw with root directory '"), fmt::streamed(rootDirectory), fmt::streamed("."));
        return DirectorySchemaPtr(new ScanProjectSchemaRaw(rootDirectory));
    }
      else if (name == "RAWPLY")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Creating ScanProjectSchemaRawPly with root directory '"), fmt::streamed(rootDirectory), fmt::streamed("."));
        return DirectorySchemaPtr(new ScanProjectSchemaRaw(rootDirectory));
    }
    else if (name == "SLAM6D")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Creating ScanProjectSchemaSlam6D with root directory '"), fmt::streamed(rootDirectory), fmt::streamed("."));
        return DirectorySchemaPtr(new ScanProjectSchemaSlam6D(rootDirectory));
    }
    // else if (name == "OUSTERPLY")
    // {
    //     return DirectorySchemaPtr(new ScanProjectSchemaOusterPly(rootDirectory));
    // }
#ifdef LVR2_USE_RDB
    else if (name == "RDBX")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Creating ScanProjectSchemaRDBX with root directory '"), fmt::streamed(rootDirectory), fmt::streamed("."));
        return DirectorySchemaPtr(new ScanProjectSchemaRdbx(rootDirectory));
    }
#endif

        lvr2::log::error("{}{}{}", fmt::streamed("[DirectorySchemaFromName] Unknown directory schema name '"), fmt::streamed(schemaName), fmt::streamed("'."));
    return nullptr;
}

HDF5SchemaPtr hdf5SchemaFromName(const std::string& schemaName)
{
    std::string name = boost::to_upper_copy<std::string>(schemaName);

    if(name == "HDF5")
    {
                lvr2::log::info("{}", fmt::streamed("[HDF5SchemaFromName] Creating ScanProjectSchemaHDF5."));
        return HDF5SchemaPtr(new ScanProjectSchemaHDF5);
    }
    else if(name == "HDFV5V2")
    {
                lvr2::log::error("{}", fmt::streamed("[HDF5SchemaFromName] ScanProjectSchemaHDF5V2 not fully implemented."));
        return nullptr;
        //return HDF5SchemaPtr(new ScanProjectSchemaHDF5V2);
    }

        lvr2::log::error("{}{}{}", fmt::streamed("[HDF5SchemaFromName] Unknown HDF5 schema name '"), fmt::streamed(schemaName), fmt::streamed("'."));
    return nullptr;
}

ScanProjectSchemaPtr schemaFromName(const std::string& schemaName, const std::string root)
{
    boost::filesystem::path path(root);

    if(boost::filesystem::is_directory(path))
    {
        return directorySchemaFromName(schemaName, root);
    }
    else if(path.extension() == ".h5")
    {
        return hdf5SchemaFromName(schemaName);
    }
    else
    {
        return nullptr;
    }
}

} //namespace lvr2
