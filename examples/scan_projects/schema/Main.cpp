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
        LOG(Logger::WARNING) << "Something went wrong. Saved and loaded scan project are not equal" << std::endl;
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
        LOG(Logger::WARNING) << "Something went wrong. Saved and loaded scan project are not equal" << std::endl;
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
        LOG(Logger::WARNING) << "Something went wrong. Saved and loaded scan project are not equal" << std::endl;
    }
}

} // namespace

int main(int argc, char** argv)
{
    LOG.setLoggerLevel(Logger::DEBUG);

    LOG(Logger::HIGHLIGHT) << "ScanProjects Schema" << std::endl;

    LOG(Logger::DEBUG) << "Generating dataset, wait." << std::endl;
    ScanProjectPtr sp = dummyScanProjectStorage();

    LOG(Logger::HIGHLIGHT) << "1. Example: raw-PLY directory schema" << std::endl;
    LOG.tab();
    useDirectoryRawPlySchema(sp);
    LOG.deltab();

    LOG(Logger::HIGHLIGHT) << "2. Example: one-shot raw-PLY helpers" << std::endl;
    LOG.tab();
    useDirectoryOneShotHelpers(sp);
    LOG.deltab();

    LOG(Logger::HIGHLIGHT) << "3. Example: HDF5 schema" << std::endl;
    LOG.tab();
    useHdf5Schema(sp);
    LOG.deltab();

    return 0;
}
