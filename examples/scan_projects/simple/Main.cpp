#include <iostream>
#include <memory>
#include <lvr2/types/Variant.hpp>

#include "../helper/include/Logging.hpp"
#include "../helper/include/ScanTypesCompare.hpp"
#include "../helper/include/ScanTypesDummies.hpp"

#include "lvr2/io/scan.hpp"
#include "lvr2/types/ScanTypes.hpp"

using namespace lvr2;

namespace
{

void reportStorageError(const std::string& action, const lvr2::io::storage::Error& error)
{
    LOG(Logger::WARNING) << action << " failed: " << error.message << std::endl;
}

void directoryProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/dirio_data";

    LOG(Logger::DEBUG) << "Save complete scan project to '" << filename << "'" << std::endl;
    auto opened = lvr2::io::scan::open_directory(
        filename,
        lvr2::io::scan::Schema::raw_ply(),
        lvr2::io::storage::LoadMode::Eager);
    if (!opened)
    {
        reportStorageError("Open directory scan project", opened.error());
        return;
    }

    auto& store = opened.value();
    auto saved = store.save(*sp);
    if (!saved)
    {
        reportStorageError("Save directory scan project", saved.error());
        return;
    }

    LOG(Logger::DEBUG) << "Load scan project into new buffer" << std::endl;
    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load directory scan project", loaded.error());
        return;
    }

    if (equal(sp, loaded.value()))
    {
        LOG(Logger::DEBUG) << "Directory ProjectStore saves and loads correctly." << std::endl;
    }
    else
    {
        LOG(Logger::WARNING) << "Something went wrong. Saved and loaded scan project are not equal" << std::endl;
    }

    LOG(Logger::DEBUG) << "Take a look at '" << filename << "'" << std::endl;
    LOG(Logger::DEBUG) << "- You can use 'tree' to show the entire directory structure" << std::endl;
}

void hdf5ProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/hdf5io_data.h5";

    LOG(Logger::DEBUG) << "Save complete scan project to '" << filename << "'" << std::endl;
    auto opened = lvr2::io::scan::open_hdf5(
        filename,
        lvr2::io::scan::Schema::hdf5(),
        lvr2::io::storage::LoadMode::Eager);
    if (!opened)
    {
        reportStorageError("Open HDF5 scan project", opened.error());
        return;
    }

    auto& store = opened.value();
    auto saved = store.save(*sp);
    if (!saved)
    {
        reportStorageError("Save HDF5 scan project", saved.error());
        return;
    }

    LOG(Logger::DEBUG) << "Load scan project into new buffer" << std::endl;
    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load HDF5 scan project", loaded.error());
        return;
    }

    if (equal(sp, loaded.value()))
    {
        LOG(Logger::DEBUG) << "HDF5 ProjectStore saves and loads correctly." << std::endl;
    }
    else
    {
        LOG(Logger::WARNING) << "Something went wrong. Saved and loaded scan project are not equal" << std::endl;
    }

    LOG(Logger::DEBUG) << "Take a look at '" << filename << "'" << std::endl;
    LOG(Logger::DEBUG) << "- You can use 'HDFCompass' to view the entire hdf5 structure" << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
    LOG.setLoggerLevel(Logger::DEBUG);

    LOG(Logger::HIGHLIGHT) << "ScanProjects Simple" << std::endl;

    LOG(Logger::DEBUG) << "Generating dataset, wait." << std::endl;
    ScanProjectPtr sp = dummyScanProjectStorage();

    LOG(Logger::INFO) << "1. Example: Directory ProjectStore" << std::endl;
    LOG.tab();
    directoryProjectStoreExample(sp);
    LOG.deltab();

    LOG(Logger::INFO) << "2. Example: HDF5 ProjectStore" << std::endl;
    LOG.tab();
    hdf5ProjectStoreExample(sp);
    LOG.deltab();

    return 0;
}
