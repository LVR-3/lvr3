#include <iostream>
#include <memory>

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

void hdf5CompressedExample(ScanProjectPtr sp, size_t compressionLevel)
{
    std::string filename = "examples_sp_compression/sp_compressed_" + std::to_string(compressionLevel) + ".h5";

    auto saveOptions = lvr2::io::scan::SaveOptions::hdf5();
    saveOptions.hdf5Options.compressionLevel = static_cast<unsigned int>(compressionLevel);

    LOG(Logger::DEBUG) << "Save complete scan project compressed to '" << filename << "'" << std::endl;
    auto saved = lvr2::io::scan::save_project(filename, *sp, saveOptions);
    if (!saved)
    {
        reportStorageError("Save HDF5 scan project", saved.error());
        return;
    }

    LOG(Logger::DEBUG) << "Load scan project into new buffer" << std::endl;
    auto loadOptions = lvr2::io::scan::LoadOptions::hdf5();
    loadOptions.loadMode = lvr2::io::storage::LoadMode::Eager;
    auto loaded = lvr2::io::scan::load_project(filename, loadOptions);
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
}

} // namespace

int main(int argc, char** argv)
{
    LOG.setLoggerLevel(Logger::DEBUG);

    LOG(Logger::HIGHLIGHT) << "ScanProjects Compression" << std::endl;

    LOG(Logger::DEBUG) << "Generating dataset, wait." << std::endl;
    ScanProjectPtr sp = dummyScanProjectStorage();

    LOG(Logger::INFO) << "Writing dummy scan project with different HDF5 compression levels" << std::endl;

    for (size_t i = 0; i < 10; i++)
    {
        LOG(Logger::HIGHLIGHT) << "Compression Level: " << i << std::endl;
        LOG.tab();
        hdf5CompressedExample(sp, i);
        LOG.deltab();
    }

    return 0;
}
