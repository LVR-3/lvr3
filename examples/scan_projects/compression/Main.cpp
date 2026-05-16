#include <iostream>
#include <memory>

#include <lvr2/util/Logging.hpp>
#include "../helper/include/ScanTypesCompare.hpp"
#include "../helper/include/ScanTypesDummies.hpp"

#include "lvr2/io/scan.hpp"
#include "lvr2/types/ScanTypes.hpp"

using namespace lvr2;

namespace
{

void reportStorageError(const std::string& action, const lvr2::io::storage::Error& error)
{
        lvr2::log::warning("{}{}{}", fmt::streamed(action), fmt::streamed(" failed: "), fmt::streamed(error.message));
}

void hdf5CompressedExample(ScanProjectPtr sp, size_t compressionLevel)
{
    std::string filename = "examples_sp_compression/sp_compressed_" + std::to_string(compressionLevel) + ".h5";

    auto saveOptions = lvr2::io::scan::SaveOptions::hdf5();
    saveOptions.hdf5Options.compressionLevel = static_cast<unsigned int>(compressionLevel);

        lvr2::log::debug("{}{}{}", fmt::streamed("Save complete scan project compressed to '"), fmt::streamed(filename), fmt::streamed("'"));
    auto saved = lvr2::io::scan::save_project(filename, *sp, saveOptions);
    if (!saved)
    {
        reportStorageError("Save HDF5 scan project", saved.error());
        return;
    }

        lvr2::log::debug("{}", fmt::streamed("Load scan project into new buffer"));
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
                lvr2::log::debug("{}", fmt::streamed("HDF5 ProjectStore saves and loads correctly."));
    }
    else
    {
                lvr2::log::warning("{}", fmt::streamed("Something went wrong. Saved and loaded scan project are not equal"));
    }
}

} // namespace

int main(int argc, char** argv)
{
    lvr2::log::set_level(lvr2::log::Level::debug);

        lvr2::log::info("{}", fmt::streamed("ScanProjects Compression"));

        lvr2::log::debug("{}", fmt::streamed("Generating dataset, wait."));
    ScanProjectPtr sp = dummyScanProjectStorage();

        lvr2::log::info("{}", fmt::streamed("Writing dummy scan project with different HDF5 compression levels"));

    for (size_t i = 0; i < 10; i++)
    {
                lvr2::log::info("{}{}", fmt::streamed("Compression Level: "), fmt::streamed(i));
        hdf5CompressedExample(sp, i);
    }

    return 0;
}
