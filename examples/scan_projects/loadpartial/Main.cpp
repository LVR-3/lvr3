#include <iostream>
#include <memory>

#include "../helper/include/Logging.hpp"
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
    LOG(Logger::DEBUG) << "Save complete scan project to '" << filename << "'" << std::endl;
    auto saved = store.save(*sp);
    if (!saved)
    {
        reportStorageError("Save directory scan project", saved.error());
        return;
    }

    std::cout << std::endl;
    LOG(Logger::INFO) << "Load project hierarchy lazily" << std::endl;
    auto loadedProject = store.load();
    if (!loadedProject)
    {
        reportStorageError("Load directory scan project", loadedProject.error());
        return;
    }

    LOG(Logger::DEBUG) << "Total points after explicit scan loads: "
                       << countLoadedPoints(loadedProject.value()) << std::endl;
    std::cout << std::endl;

    LOG.tab();
    LOG(Logger::INFO) << "Load one scan position through ProjectStore" << std::endl;
    auto position = store.load_position(1);
    if (!position)
    {
        reportStorageError("Load scan position", position.error());
    }
    else if (position.value())
    {
        ScanProjectPtr singlePositionProject(new ScanProject);
        singlePositionProject->positions.push_back(position.value());
        LOG(Logger::DEBUG) << "Total points in loaded position: "
                           << countLoadedPoints(singlePositionProject) << std::endl;
    }
    std::cout << std::endl;

    LOG.tab();
    LOG(Logger::INFO) << "Load one LIDAR through ProjectStore" << std::endl;
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
        LOG(Logger::DEBUG) << "Total points in loaded LIDAR: " << totalPoints << std::endl;
    }
    std::cout << std::endl;

    LOG.tab();
    LOG(Logger::INFO) << "Load one scan payload through ProjectStore" << std::endl;
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
            LOG(Logger::DEBUG) << "Total points in scan: " << scan.value()->points->numPoints() << std::endl;
        }
        scan.value()->release();
    }
    std::cout << std::endl;

    LOG.deltab();
    LOG.deltab();
    LOG.deltab();
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

    LOG(Logger::INFO) << "Load scan-project metadata through ProjectStore" << std::endl;
    auto meta = store.load_meta();
    if (!meta)
    {
        reportStorageError("Load project metadata", meta.error());
        return;
    }
    LOG(Logger::DEBUG) << meta.value().text << std::endl;
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
            std::cout << timestamp << "Unable to open HDF5 scan project: " << opened.error().message << std::endl;
            return 1;
        }

        std::cout << timestamp << "1. Load ScanProject no data" << std::endl;
        auto loaded = opened.value().load();
        if (!loaded)
        {
            std::cout << timestamp << "Unable to load HDF5 scan project: " << loaded.error().message << std::endl;
            return 1;
        }
        std::cout << timestamp << "- Done." << std::endl;

        ScanProjectPtr sp_loaded = loaded.value();
        if (sp_loaded && !sp_loaded->positions.empty() &&
            !sp_loaded->positions[0]->lidars.empty() &&
            !sp_loaded->positions[0]->lidars[0]->scans.empty())
        {
            ScanPtr scan = sp_loaded->positions[0]->lidars[0]->scans[0];
            if (scan)
            {
                std::cout << timestamp << "- Load " << scan->numPoints << " points completely" << std::endl;
                scan->load();
                std::cout << timestamp << "- Done." << std::endl;
                if (scan->points)
                {
                    std::cout << *scan->points << std::endl;
                }
                scan->release();

                ReductionAlgorithmPtr red(new FixedSizeReductionAlgorithm(1000));
                std::cout << timestamp << "- Load " << scan->numPoints << " points reduced" << std::endl;
                scan->load(red);
                std::cout << timestamp << "- Done." << std::endl;
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
        LOG.setLoggerLevel(Logger::DEBUG);

        LOG(Logger::HIGHLIGHT) << "ScanProjects Load Partial" << std::endl;

        LOG(Logger::DEBUG) << "Generating dataset, wait." << std::endl;
        ScanProjectPtr sp = dummyScanProjectStorage();

        std::cout << std::endl;
        LOG(Logger::HIGHLIGHT) << "1. Example: Load datasets partially" << std::endl;
        LOG.tab();
        loadPartial(sp);
        LOG.deltab();

        std::cout << std::endl;
        LOG(Logger::HIGHLIGHT) << "2. Example: Load project metadata" << std::endl;
        LOG.tab();
        loadProjectMeta(sp);
        LOG.deltab();
    }

    return 0;
}
