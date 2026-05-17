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
        lvr2::log::warning("{}{}{}", action, " failed: ", error.message);
}

void useDirectoryRawPlySchema(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_schema/schema_raw_ply";

    auto opened = lvr2::io::scan::open_directory(
        filename,
        lvr2::io::scan::Schema::raw_ply(),
        lvr2::io::storage::LoadMode::Eager);
    if (!opened)
    {
        reportStorageError("Open raw-PLY directory scan project", opened.error());
        return;
    }

    auto& store = opened.value();
    auto saved = store.save(*sp);
    if (!saved)
    {
        reportStorageError("Save raw-PLY directory scan project", saved.error());
        return;
    }

    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load raw-PLY directory scan project", loaded.error());
        return;
    }

    if (!equal(sp, loaded.value()))
    {
                lvr2::log::warning("{}", "Something went wrong. Saved and loaded scan project are not equal");
    }
}

void useDirectoryOneShotHelpers(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_schema/one_shot_raw_ply";

    auto saved = lvr2::io::scan::save_project(
        filename,
        *sp,
        lvr2::io::scan::SaveOptions::directory_raw_ply());
    if (!saved)
    {
        reportStorageError("Save raw-PLY directory scan project", saved.error());
        return;
    }

    auto loadOptions = lvr2::io::scan::LoadOptions::directory_raw_ply();
    loadOptions.loadMode = lvr2::io::storage::LoadMode::Eager;
    auto loaded = lvr2::io::scan::load_project(filename, loadOptions);
    if (!loaded)
    {
        reportStorageError("Load raw-PLY directory scan project", loaded.error());
        return;
    }

    if (!equal(sp, loaded.value()))
    {
                lvr2::log::warning("{}", "Something went wrong. Saved and loaded scan project are not equal");
    }
}

void useHdf5Schema(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_schema/project_store.h5";

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

    auto loaded = store.load();
    if (!loaded)
    {
        reportStorageError("Load HDF5 scan project", loaded.error());
        return;
    }

    if (!equal(sp, loaded.value()))
    {
                lvr2::log::warning("{}", "Something went wrong. Saved and loaded scan project are not equal");
    }
}

} // namespace

int main(int argc, char** argv)
{
    lvr2::log::set_level(lvr2::log::Level::debug);

        lvr2::log::info("{}", "ScanProjects Schema");

        lvr2::log::debug("{}", "Generating dataset, wait.");
    ScanProjectPtr sp = dummyScanProjectStorage();

        lvr2::log::info("{}", "1. Example: raw-PLY directory schema");
    useDirectoryRawPlySchema(sp);

        lvr2::log::info("{}", "2. Example: one-shot raw-PLY helpers");
    useDirectoryOneShotHelpers(sp);

        lvr2::log::info("{}", "3. Example: HDF5 schema");
    useHdf5Schema(sp);

    return 0;
}
