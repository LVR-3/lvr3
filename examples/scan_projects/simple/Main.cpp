#include <iostream>
#include <memory>
#include <lvr2/types/Variant.hpp>

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

void directoryProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/dirio_data";

        lvr2::log::debug("{}{}{}", fmt::streamed("Save complete scan project to '"), fmt::streamed(filename), fmt::streamed("'"));
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

        lvr2::log::debug("{}", fmt::streamed("Load scan project into new buffer"));
    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load directory scan project", loaded.error());
        return;
    }

    if (equal(sp, loaded.value()))
    {
                lvr2::log::debug("{}", fmt::streamed("Directory ProjectStore saves and loads correctly."));
    }
    else
    {
                lvr2::log::warning("{}", fmt::streamed("Something went wrong. Saved and loaded scan project are not equal"));
    }

        lvr2::log::debug("{}{}{}", fmt::streamed("Take a look at '"), fmt::streamed(filename), fmt::streamed("'"));
        lvr2::log::debug("{}", fmt::streamed("- You can use 'tree' to show the entire directory structure"));
}

void hdf5ProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/hdf5io_data.h5";

        lvr2::log::debug("{}{}{}", fmt::streamed("Save complete scan project to '"), fmt::streamed(filename), fmt::streamed("'"));
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

        lvr2::log::debug("{}", fmt::streamed("Load scan project into new buffer"));
    auto loaded = store.load();
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

        lvr2::log::debug("{}{}{}", fmt::streamed("Take a look at '"), fmt::streamed(filename), fmt::streamed("'"));
        lvr2::log::debug("{}", fmt::streamed("- You can use 'HDFCompass' to view the entire hdf5 structure"));
}

} // namespace

int main(int argc, char** argv)
{
    lvr2::log::set_level(lvr2::log::Level::debug);

        lvr2::log::info("{}", fmt::streamed("ScanProjects Simple"));

        lvr2::log::debug("{}", fmt::streamed("Generating dataset, wait."));
    ScanProjectPtr sp = dummyScanProjectStorage();

        lvr2::log::info("{}", fmt::streamed("1. Example: Directory ProjectStore"));
    directoryProjectStoreExample(sp);

        lvr2::log::info("{}", fmt::streamed("2. Example: HDF5 ProjectStore"));
    hdf5ProjectStoreExample(sp);

    return 0;
}
