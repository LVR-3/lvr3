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
        lvr2::log::warning("{}{}{}", action, " failed: ", error.message);
}

void directoryProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/dirio_data";

        lvr2::log::debug("{}{}{}", "Save complete scan project to '", filename, "'");
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

        lvr2::log::debug("{}", "Load scan project into new buffer");
    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load directory scan project", loaded.error());
        return;
    }

    if (equal(sp, loaded.value()))
    {
                lvr2::log::debug("{}", "Directory ProjectStore saves and loads correctly.");
    }
    else
    {
                lvr2::log::warning("{}", "Something went wrong. Saved and loaded scan project are not equal");
    }

        lvr2::log::debug("{}{}{}", "Take a look at '", filename, "'");
        lvr2::log::debug("{}", "- You can use 'tree' to show the entire directory structure");
}

void hdf5ProjectStoreExample(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_simple/hdf5io_data.h5";

        lvr2::log::debug("{}{}{}", "Save complete scan project to '", filename, "'");
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

        lvr2::log::debug("{}", "Load scan project into new buffer");
    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load HDF5 scan project", loaded.error());
        return;
    }

    if (equal(sp, loaded.value()))
    {
                lvr2::log::debug("{}", "HDF5 ProjectStore saves and loads correctly.");
    }
    else
    {
                lvr2::log::warning("{}", "Something went wrong. Saved and loaded scan project are not equal");
    }

        lvr2::log::debug("{}{}{}", "Take a look at '", filename, "'");
        lvr2::log::debug("{}", "- You can use 'HDFCompass' to view the entire hdf5 structure");
}

} // namespace

int main(int argc, char** argv)
{
    lvr2::log::set_level(lvr2::log::Level::debug);

        lvr2::log::info("{}", "ScanProjects Simple");

        lvr2::log::debug("{}", "Generating dataset, wait.");
    ScanProjectPtr sp = dummyScanProjectStorage();

        lvr2::log::info("{}", "1. Example: Directory ProjectStore");
    directoryProjectStoreExample(sp);

        lvr2::log::info("{}", "2. Example: HDF5 ProjectStore");
    hdf5ProjectStoreExample(sp);

    return 0;
}
