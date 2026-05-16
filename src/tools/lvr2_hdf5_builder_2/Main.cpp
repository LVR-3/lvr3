#include "Options.hpp"
#include "lvr2/geometry/BoundingBox.hpp"
#include "lvr2/io/scan.hpp"
#include "lvr2/types/ScanTypes.hpp"
#include "lvr2/util/Timestamp.hpp"
#include <lvr2/util/Logging.hpp>

#include <boost/filesystem.hpp>
#include <boost/shared_array.hpp>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace lvr2;

bool m_usePreviews;
int m_previewReductionFactor;

namespace
{

template <typename T>
boost::shared_array<T> reduceData(boost::shared_array<T> data,
                                  size_t dataCount,
                                  size_t dataWidth,
                                  unsigned int reductionFactor,
                                  size_t* reducedDataCount)
{
    const unsigned int step = std::max(1u, reductionFactor);
    *reducedDataCount = dataCount == 0 ? 0 : ((dataCount - 1) / step) + 1;

    boost::shared_array<T> reducedData(new T[(*reducedDataCount) * dataWidth]);

    size_t reducedDataIdx = 0;
    for (size_t i = 0; i < dataCount; i++)
    {
        if (i % step == 0)
        {
            std::copy(data.get() + i * dataWidth,
                      data.get() + (i + 1) * dataWidth,
                      reducedData.get() + reducedDataIdx * dataWidth);
            reducedDataIdx++;
        }
    }

    return reducedData;
}

ScanPtr firstScan(const ScanPositionPtr& position)
{
    if (!position || position->lidars.empty() || !position->lidars[0] || position->lidars[0]->scans.empty())
    {
        return {};
    }
    return position->lidars[0]->scans[0];
}

lvr2::io::storage::Status writePreviews(const std::string& outputFile,
                                        const ScanProjectPtr& scanProject)
{
    auto registry = lvr2::io::storage::make_default_registry();
    lvr2::io::storage::OpenRequest request;
    request.uri = outputFile;
    request.kind = lvr2::io::storage::StorageKind::hdf5();

    auto backend = registry.open(request);
    if (!backend)
    {
        return lvr2::io::storage::unexpected(backend.error());
    }

    for (size_t i = 0; i < scanProject->positions.size(); i++)
    {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%08zu", i);
        std::string nr_str(buffer);
        std::string previewGroupName = "/preview/" + nr_str;

                lvr2::log::info("{}{}", fmt::streamed("Generating preview for position "), fmt::streamed(nr_str));

        ScanPtr scanPtr = firstScan(scanProject->positions[i]);
        if (!scanPtr)
        {
                        lvr2::log::info("{}{}", fmt::streamed("No scan payload for preview position "), fmt::streamed(nr_str));
            continue;
        }
        if (!scanPtr->points && scanPtr->loadable())
        {
            scanPtr->load();
        }
        if (!scanPtr->points)
        {
                        lvr2::log::info("{}{}", fmt::streamed("No point data for preview position "), fmt::streamed(nr_str));
            continue;
        }

        floatArr points = scanPtr->points->getPointArray();
        if (points)
        {
            size_t numPreview = 0;
            const unsigned int reduction = m_previewReductionFactor > 0
                ? static_cast<unsigned int>(m_previewReductionFactor)
                : 1u;
            floatArr previewData = reduceData(points,
                                              scanPtr->points->numPoints(),
                                              3,
                                              reduction,
                                              &numPreview);

            std::vector<size_t> previewDim = {numPreview, 3};
            lvr2::io::storage::FloatArrayView view{previewData.get(), previewDim};
            auto wrote = backend.value()->writeFloatArray({previewGroupName, "points"}, view);
            if (!wrote)
            {
                return wrote;
            }
        }
    }

    return {};
}

} // namespace

int main(int argc, char** argv)
{
    hdf5tool2::Options options(argc, argv);
    boost::filesystem::path inputDir(options.getInputDir());
    boost::filesystem::path outputDir(options.getOutputDir());

    boost::filesystem::path outputPath(outputDir / options.getOutputFile());

    m_usePreviews = options.getPreview();
    m_previewReductionFactor = options.getPreviewReductionRatio();

    // check if input directory exists
    if (!boost::filesystem::exists(inputDir))
    {
                lvr2::log::error("{}{}{}", fmt::streamed("Error: Directory "), fmt::streamed(options.getInputDir()), fmt::streamed(" does not exist"));
        exit(-1);
    }

    // check if output directory exists
    if (!boost::filesystem::exists(outputDir))
    {
                lvr2::log::info("{}{}", fmt::streamed("Creating directory "), fmt::streamed(options.getOutputDir()));
        if (!boost::filesystem::create_directory(outputDir))
        {
                        lvr2::log::error("{}{}", fmt::streamed("Error: Unable to create "), fmt::streamed(options.getOutputDir()));
            exit(-1);
        }
    }

    bool exitsts = false;
    ScanProjectPtr existingScanProject;

    // check if HDF5 already exists
    if (boost::filesystem::exists(outputPath))
    {
                lvr2::log::info("{}", fmt::streamed("File already exists. Expanding File..."));

        // get existing scans
        auto loaded = lvr2::io::scan::load_project(
            outputPath.string(),
            lvr2::io::scan::LoadOptions::hdf5());
        if (loaded)
        {
            existingScanProject = loaded.value();
            exitsts = true;
        }
        else
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to load existing HDF5 scan project: "), fmt::streamed(loaded.error().message));
            exitsts = false;
        }
    }

    ScanProjectPtr scanProject;

    // reading scan project from given directory into ScanProject
        lvr2::log::info("{}", fmt::streamed("Reading ScanProject from directory"));
    auto loadedInput = lvr2::io::scan::load_project(
        inputDir.string(),
        lvr2::io::scan::LoadOptions::directory_raw_ply());
    if (!loadedInput)
    {
                lvr2::log::error("{}{}", fmt::streamed("Unable to load input scan project: "), fmt::streamed(loadedInput.error().message));
        return 1;
    }
    scanProject = loadedInput.value();

    // saving ScanProject into HDF5 file
        lvr2::log::info("{}", fmt::streamed("Writing ScanProject to HDF5"));
    if (exitsts)
    {
        for (ScanPositionPtr scanPosPtr : scanProject->positions)
        {
            existingScanProject->positions.push_back(scanPosPtr);
        }
        auto saved = lvr2::io::scan::save_project(
            outputPath.string(),
            *existingScanProject,
            lvr2::io::scan::SaveOptions::hdf5());
        if (!saved)
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to save HDF5 scan project: "), fmt::streamed(saved.error().message));
            return 1;
        }
    }
    else
    {
        auto saved = lvr2::io::scan::save_project(
            outputPath.string(),
            *scanProject,
            lvr2::io::scan::SaveOptions::hdf5());
        if (!saved)
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to save HDF5 scan project: "), fmt::streamed(saved.error().message));
            return 1;
        }
    }

    if (m_usePreviews)
    {
        auto previews = writePreviews(outputPath.string(), scanProject);
        if (!previews)
        {
                        lvr2::log::error("{}{}", fmt::streamed("Unable to write preview arrays: "), fmt::streamed(previews.error().message));
            return 1;
        }
    }

        lvr2::log::info("{}", fmt::streamed("Program finished"));
    return 0;
}
