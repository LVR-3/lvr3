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

/**
 * Main.cpp
 *
 *  Created on: Aug 9, 2013
 *      Author: Thomas Wiemann
 */


#include "Options.hpp"

#include "lvr2/io/ModelFactory.hpp"
#include "lvr2/io/ScanDirectoryParser.hpp"
#include "lvr2/util/IOUtils.hpp"
#include "lvr2/registration/OctreeReduction.hpp"
#include <lvr2/util/Logging.hpp>

using namespace lvr2;

const kaboom::Options* options;

int main(int argc, char** argv) {
    kaboom::Options options(argc, argv);
    ModelPtr model = ModelFactory::readModel("/home/kyrill/rab/exchange/botanischer_garten_2018/2018-06-04_bot_garden.RiSCAN/SCANS/ScanPos001/POINTCLOUDS/180604_155149.rdbx");
    //ModelPtr model = ModelFactory::readModel("/mnt/exchange/praktikum_sose_2022/tree.ply");
    ModelFactory::saveModel(model, "/home/kyrill/Uni/Robotik_Prog_Praktikum/Test/Botanischer_Garten.ply");
    return 0;
    // Parse command line arguments

    if (options.getTargetSize() && options.getVoxelSize())
    {
                lvr2::log::warning("{}", fmt::streamed("Warning: Octree reduction and random reduction requested."));
                lvr2::log::info("{}", fmt::streamed("Please chose set either octree voxel size with -v or target "));
                lvr2::log::info("{}", fmt::streamed("size with random reduction using --targetSize."));
        return 0;
    }

    if(options.getInputFile() != "")
    {
                lvr2::log::info("{}{}{}", fmt::streamed("Reading '"), fmt::streamed(options.getInputFile()), fmt::streamed("."));
        ModelPtr model = ModelFactory::readModel(options.getInputFile());
        if(model)
        {
            PointBufferPtr result = model->m_pointCloud;
            PointBufferPtr buffer = model->m_pointCloud;

            // Reduce if requested using the specified technique
            if(options.getTargetSize())
            {
                                lvr2::log::info("{}{}{}", fmt::streamed("Random sampling "), fmt::streamed(options.getTargetSize()), fmt::streamed(" points."));
                result = subSamplePointBuffer(buffer, options.getTargetSize());
            }
            else if(options.getVoxelSize())
            {
                                lvr2::log::info("{}{}", fmt::streamed("Octree reduction with voxel size "), fmt::streamed(options.getVoxelSize()));
                RandomSampleOctreeReduction oct(buffer, options.getVoxelSize(), 5);
                result = oct.getReducedPoints();
            }

            // Convert coordinates of result buffer is nessessary
            if(options.convertToLVR())
            {
                                lvr2::log::info("{}", fmt::streamed("Converting from SLAM6D to LVR coordinates"));
                slamToLVRInPlace(result);
            }

            string targetFileName;
            if(options.getOutputFile() == "")
            {
                targetFileName = "result.ply";
            }
            else
            {
                targetFileName = options.getOutputFile();
            }

                        lvr2::log::info("{}{}{}", fmt::streamed("Saving '"), fmt::streamed(targetFileName), fmt::streamed("'"));
            ModelFactory::saveModel(ModelPtr(new Model(result)), targetFileName);
        }
        else
        {
                        lvr2::log::error("{}{}{}", fmt::streamed("Error: Could not load '"), fmt::streamed(options.getInputFile()), fmt::streamed("'."));
        }

    }
    else
    {
        ScanDirectoryParser parser(options.getInputDir());
        parser.setStart(options.getStart());
        parser.setEnd(options.getEnd());
        parser.setPointCloudPrefix(options.getScanPrefix());
        parser.setPosePrefix(options.getPosePrefix());
        parser.setPointCloudExtension(options.getScanExtension());
        parser.setPoseExtension(options.getPoseExtension());
        parser.parseDirectory();

        if(options.getTargetSize())
        {
            PointBufferPtr result = parser.randomSubSample(options.getTargetSize());
        }
        else if(options.getVoxelSize())
        {
            PointBufferPtr result = parser.octreeSubSample(options.getVoxelSize(), options.getMinPointsPerVoxel());
        }
        else
        {
            PointBufferPtr result = parser.transform();
        }

    }


    return 0;
}
