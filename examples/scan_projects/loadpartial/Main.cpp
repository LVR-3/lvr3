#include <iostream>
#include <memory>

#include <lvr2/util/Logging.hpp>
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

size_t countLoadedPoints(const ScanProjectPtr& project)
{
    size_t totalPoints = 0;
    if (!project)
    {
        return totalPoints;
    }

    for (const auto& pos : project->positions)
    {
        if (!pos)
        {
            continue;
        }
        for (const auto& lidar : pos->lidars)
        {
            if (!lidar)
            {
                continue;
            }
            for (const auto& scan : lidar->scans)
            {
                if (!scan)
                {
                    continue;
                }
                if (!scan->points && scan->loadable())
                {
                    scan->load();
                }
                if (scan->points)
                {
                    totalPoints += scan->points->numPoints();
                    scan->release();
                }
            }
        }
    }
    return totalPoints;
}

void loadPartial(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_loadpartial/dirio_data";

    auto opened = lvr2::io::scan::open_directory(
        filename,
        lvr2::io::scan::Schema::raw_ply(),
        lvr2::io::storage::LoadMode::Lazy);
    if (!opened)
    {
        reportStorageError("Open directory scan project", opened.error());
        return;
    }

    auto& store = opened.value();
        lvr2::log::debug("{}{}{}", fmt::streamed("Save complete scan project to '"), fmt::streamed(filename), fmt::streamed("'"));
    auto saved = store.save(*sp);
    if (!saved)
    {
        reportStorageError("Save directory scan project", saved.error());
        return;
    }

    std::cout << std::endl;
        lvr2::log::info("{}", fmt::streamed("Load project hierarchy lazily"));
    auto loadedProject = store.load();
    if (!loadedProject)
    {
        reportStorageError("Load directory scan project", loadedProject.error());
        return;
    }

        lvr2::log::debug("{}{}", fmt::streamed("Total points after explicit scan loads: "), fmt::streamed(countLoadedPoints(loadedProject.value())));
    std::cout << std::endl;

        lvr2::log::info("{}", fmt::streamed("Load one scan position through ProjectStore"));
    auto position = store.load_position(1);
    if (!position)
    {
        reportStorageError("Load scan position", position.error());
    }
    else if (position.value())
    {
        ScanProjectPtr singlePositionProject(new ScanProject);
        singlePositionProject->positions.push_back(position.value());
                lvr2::log::debug("{}{}", fmt::streamed("Total points in loaded position: "), fmt::streamed(countLoadedPoints(singlePositionProject)));
    }
    std::cout << std::endl;

        lvr2::log::info("{}", fmt::streamed("Load one LIDAR through ProjectStore"));
    auto lidar = store.load_lidar(1, 0);
    if (!lidar)
    {
        reportStorageError("Load LIDAR", lidar.error());
    }
    else if (lidar.value())
    {
        size_t totalPoints = 0;
        for (const auto& scan : lidar.value()->scans)
        {
            if (scan && !scan->points && scan->loadable())
            {
                scan->load();
            }
            if (scan && scan->points)
            {
                totalPoints += scan->points->numPoints();
                scan->release();
            }
        }
                lvr2::log::debug("{}{}", fmt::streamed("Total points in loaded LIDAR: "), fmt::streamed(totalPoints));
    }
    std::cout << std::endl;

        lvr2::log::info("{}", fmt::streamed("Load one scan payload through ProjectStore"));
    auto scan = store.load_scan(1, 0, 0);
    if (!scan)
    {
        reportStorageError("Load scan", scan.error());
    }
    else if (scan.value())
    {
        scan.value()->load();
        if (scan.value()->points)
        {
                        lvr2::log::debug("{}{}", fmt::streamed("Total points in scan: "), fmt::streamed(scan.value()->points->numPoints()));
        }
        scan.value()->release();
    }
    std::cout << std::endl;

}

void loadProjectMeta(ScanProjectPtr sp)
{
    std::string filename = "examples_sp_loadpartial/dirio_data";

    auto opened = lvr2::io::scan::open_directory(
        filename,
        lvr2::io::scan::Schema::raw_ply(),
        lvr2::io::storage::LoadMode::Lazy);
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

        lvr2::log::info("{}", fmt::streamed("Load scan-project metadata through ProjectStore"));
    auto meta = store.load_meta();
    if (!meta)
    {
        reportStorageError("Load project metadata", meta.error());
        return;
    }
        lvr2::log::debug("{}", fmt::streamed(meta.value().text));
    std::cout << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        std::string filename = argv[1];
        auto opened = lvr2::io::scan::open_hdf5(
            filename,
            lvr2::io::scan::Schema::hdf5(),
            lvr2::io::storage::LoadMode::Lazy);
        if (!opened)
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to open HDF5 scan project: "), fmt::streamed(opened.error().message));
            return 1;
        }

                lvr2::log::info("{}", fmt::streamed("1. Load ScanProject no data"));
        auto loaded = opened.value().load();
        if (!loaded)
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to load HDF5 scan project: "), fmt::streamed(loaded.error().message));
            return 1;
        }
                lvr2::log::info("{}", fmt::streamed("- Done."));

        ScanProjectPtr sp_loaded = loaded.value();
        if (sp_loaded && !sp_loaded->positions.empty() &&
            !sp_loaded->positions[0]->lidars.empty() &&
            !sp_loaded->positions[0]->lidars[0]->scans.empty())
        {
            ScanPtr scan = sp_loaded->positions[0]->lidars[0]->scans[0];
            if (scan)
            {
                                lvr2::log::info("{}{}{}", fmt::streamed("- Load "), fmt::streamed(scan->numPoints), fmt::streamed(" points completely"));
                scan->load();
                                lvr2::log::info("{}", fmt::streamed("- Done."));
                if (scan->points)
                {
                    std::cout << *scan->points << std::endl;
                }
                scan->release();

                ReductionAlgorithmPtr red(new FixedSizeReductionAlgorithm(1000));
                                lvr2::log::info("{}{}{}", fmt::streamed("- Load "), fmt::streamed(scan->numPoints), fmt::streamed(" points reduced"));
                scan->load(red);
                                lvr2::log::info("{}", fmt::streamed("- Done."));
                if (scan->points)
                {
                    std::cout << *scan->points << std::endl;
                }
                scan->release();
            }
        }
    }
    else
    {
        lvr2::log::set_level(lvr2::log::Level::debug);

                lvr2::log::info("{}", fmt::streamed("ScanProjects Load Partial"));

                lvr2::log::debug("{}", fmt::streamed("Generating dataset, wait."));
        ScanProjectPtr sp = dummyScanProjectStorage();

        std::cout << std::endl;
                lvr2::log::info("{}", fmt::streamed("1. Example: Load datasets partially"));
        loadPartial(sp);

        std::cout << std::endl;
                lvr2::log::info("{}", fmt::streamed("2. Example: Load project metadata"));
        loadProjectMeta(sp);
    }

    return 0;
}
