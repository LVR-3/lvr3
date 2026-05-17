/**
 * Copyright (c) 2018, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <random>
#include <string>
#include <algorithm>
#include <iostream>

#include <boost/filesystem.hpp>

#include "lvr2/reconstruction/SearchTreeFlann.hpp"
#include "lvr2/reconstruction/LargeScaleReconstruction.hpp"
#include "lvr2/io/scan.hpp"
#include "lvr2/util/IOUtils.hpp"

#include "Options.hpp"


using std::cout;
using std::endl;
using namespace lvr2;

#if defined CUDA_FOUND
#define GPU_FOUND
#include "lvr2/reconstruction/cuda/CudaSurface.hpp"
typedef CudaSurface GpuSurface;
#elif defined OPENCL_FOUND
#define GPU_FOUND
#include "lvr2/reconstruction/opencl/ClSurface.hpp"
#include <lvr2/util/Logging.hpp>
typedef ClSurface GpuSurface;
#endif

using Vec = lvr2::BaseVector<float>;

int main(int argc, char** argv)
{
    // =======================================================================
    // Parse and print command line parameters
    // =======================================================================
    // Parse command line arguments
    LargeScaleOptions::Options options(argc, argv);

    // Exit if options had to generate a usage message
    // (this means required parameters are missing)
    if (options.printUsage())
    {
        return EXIT_SUCCESS;
    }

    options.printLogo();

    fs::path selectedFile = options.m_inputFile;
    std::string input = selectedFile.string();

    std::string extension = selectedFile.extension().string();

    LargeScaleReconstruction<Vec> lsr(options.m_options);


    ScanProjectEditMarkPtr project(new ScanProjectEditMark);

    //reconstruction from hdf5
    if (extension == ".h5" || extension == ".hdf5")
    {
                lvr2::log::info("{}", "Reading project from HDF5 file");
        auto loaded = lvr2::io::scan::load_project(input, lvr2::io::scan::LoadOptions::hdf5());
        if (!loaded)
        {
                        lvr2::log::error("{}{}", "Unable to load HDF5 scan project: ", loaded.error().message);
            return EXIT_FAILURE;
        }

        project->project = loaded.value();
        project->changed.resize(project->project->positions.size(), true);
    }
    else
    {
        //reconstruction from ScanProject Folder
        if(boost::filesystem::is_directory(selectedFile))
        {
            auto loaded = lvr2::io::scan::load_project(
                input,
                lvr2::io::scan::LoadOptions::directory_raw_ply());
            if (loaded)
            {
                project->project = loaded.value();
                project->changed.resize(project->project->positions.size(), true);
            }
            else
            {
                                lvr2::log::error("{}{}", "Unable to load directory scan project: ", loaded.error().message);
                                lvr2::log::info("{}", "Trying directory as a folder of .ply files");

                // Setup basic scan project structure
                project->project.reset(new ScanProject);
                for (auto file : boost::filesystem::directory_iterator(selectedFile))
                {
                    auto path = file.path();
                    if(path.extension() != ".ply")
                    {
                                                lvr2::log::info("{}{}", "Skipping file: ", path.string());
                        continue;
                    }

                                        lvr2::log::info("{}{}", "Using file: ", path.string());

                    // Create new Scan
                    ScanPtr scan(new Scan);
                    scan->points_loader = [path](){ return ModelFactory::readModel(path.string())->m_pointCloud; };

                    // Wrap scan into lidar object
                    LIDARPtr lidar(new LIDAR);
                    lidar->scans.push_back(scan);

                    // Put lidar into new scan position
                    ScanPositionPtr position(new ScanPosition);
                    position->lidars.push_back(lidar);

                    // Add new scan position to scan project
                    project->project->positions.push_back(position);
                    project->changed.push_back(true);
                }
            }
        }
        //reconstruction from a .ply file
        else
        {
                        lvr2::log::info("{}{}", "Reading single file: ", selectedFile.string());
            ModelPtr model = ModelFactory::readModel(input);

            // Create new scan object and mark scan data as loaded
            ScanPtr scan(new Scan());
            scan->points = model->m_pointCloud;

            // Create new lidar object
            LIDARPtr lidar(new LIDAR());
            lidar->scans.push_back(scan);

            // Create new scan position
            ScanPositionPtr scanPosPtr(new ScanPosition());
            scanPosPtr->lidars.push_back(lidar);

            // Create new scan project
            project->project.reset(new ScanProject());
            project->project->positions.push_back(scanPosPtr);

            project->changed.push_back(true);
        }
    }

    BoundingBox<Vec> boundingBox;
    std::shared_ptr<ChunkHashGrid> cm = nullptr;
    fs::path chunkFile = options.m_options.tempDir / "chunk_manager.h5";
    if (options.m_options.partMethod == 1)
    {
        cm = std::shared_ptr<ChunkHashGrid>(new ChunkHashGrid(chunkFile.string(), 10, boundingBox, options.m_options.bgVoxelSize));
    }

    BoundingBox<Vec> bb;
    lsr.chunkAndReconstruct(project, bb, cm);

    project.reset();
    cm.reset();
    fs::remove_all(options.m_options.tempDir);

        lvr2::log::info("{}", "Program end.");

    return 0;
}
