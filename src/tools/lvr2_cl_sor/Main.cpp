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

#include <boost/filesystem.hpp>

#include "lvr2/reconstruction/opencl/ClStatisticalOutlierFilter.hpp"
#include "lvr2/io/ModelFactory.hpp"
#include "lvr2/io/DataStruct.hpp"
#include "lvr2/util/Timestamp.hpp"
#include <lvr2/util/Logging.hpp>
#include "lvr2/util/IOUtils.hpp"
//#include "Options.hpp"


using namespace lvr2;

void filter(lvr2::PointBufferPtr& cloud, lvr2::indexArray& inlier, size_t j)
{
    lvr2::PointBufferPtr cloud_filtered(new lvr2::PointBuffer());
    int type;
    std::map<std::string, lvr2::Channel<float> > floatChannels;
    type = cloud->getAllChannelsOfType<float>(floatChannels);

    std::vector<lvr2::FloatChannelPtr> channels;
    for(auto channelPair: floatChannels)
    {
        std::string name = channelPair.first;
        std::cout << name << std::endl;
        auto channel2 = channelPair.second;
        lvr2::FloatChannelPtr filtered(new lvr2::FloatChannel(j, channel2.width()));
        channels.push_back(filtered);
        for(size_t i = 0; i < j; ++i)
        {
            size_t index = inlier[i];
            for(size_t k = 0; k < channel2.width(); ++k)
            {
                (*filtered)[i][k] = channel2[index][k];
            }
        }
//        cloud->removeFloatChannel(name);
        cloud_filtered->addFloatChannel(filtered, name);

    }

    std::vector<lvr2::UCharChannelPtr> channels_uchar;
    std::map<std::string, lvr2::Channel<unsigned char> >  uCharChannels;
    int ucharType = cloud->getAllChannelsOfType<unsigned char>(uCharChannels);
    for(auto channelPair: uCharChannels)
    {
        std::string name = channelPair.first;
        auto channel2 = channelPair.second;
        lvr2::UCharChannelPtr filtered(new lvr2::UCharChannel(j, channel2.width()));
        channels_uchar.push_back(filtered);
        for(size_t i = 0; i < j; ++i)
        {
            size_t index = inlier[i];
            for(size_t k = 0; k < channel2.width(); ++k)
            {
                (*filtered)[i][k] = channel2[index][k];
            }
        }
//        cloud->removeUCharChannel(name);
        cloud_filtered->addUCharChannel(filtered, name);
    }

    cloud = cloud_filtered;
}

int main(int argc, char** argv){
    ModelPtr model = ModelFactory::readModel(argv[1]);
    size_t num_points;


    UCharChannelOptional colorsOpt = model->m_pointCloud->getUCharChannel("colors");

    // filter based on grayscale from rgb
    //
//    if(colorsOpt)
//    {
//     		lvr2::uintArr inlier2 = lvr2::uintArr(new unsigned int[model->m_pointCloud->numPoints()]);
//            size_t k = 0;
//            for(size_t i = 0; i < colorsOpt->numElements(); ++i)
//            {
//        	    int scalar = (*colorsOpt)[i][0] +
//        		         (*colorsOpt)[i][1] +
//        		         (*colorsOpt)[i][2];
//        	    scalar /= 3;
//        	    if(scalar < 255)
//        	    {
//        		inlier2[k++] = i;
//        	    }
//            }
//
//	    filter(model->m_pointCloud, inlier2, k);
//    }

    floatArr points;
    if (model && model->m_pointCloud )
    {
        num_points = model->m_pointCloud->numPoints();
        points = model->m_pointCloud->getPointArray();
                lvr2::log::info("{}{}{}{}", "Read ", num_points, " points from ", argv[1]);
    }
    else
    {
                lvr2::log::warning("{}{}", "Warning: No point cloud data found in ", argv[1]);
        return 0;
    }

    lvr2::uintArr inlier = lvr2::uintArr(new unsigned int[num_points]);

        lvr2::log::info("{}", "Constructing kd-tree...");
    ClSOR sor(points, num_points, 40);
        lvr2::log::info("{}", "Finished kd-tree construction.");


    sor.calcDistances();
        lvr2::log::info("{}", "Got Nearest Neighbors");
    sor.calcStatistics();
        lvr2::log::info("{}", "Got Statistics");
    sor.setMult(1.5);

    int j  = sor.getInliers(inlier);

        lvr2::log::info("{}{}", "outliers ", num_points - j);
        lvr2::log::info("{}{}", "inliers ", j);

    filter(model->m_pointCloud, inlier, j);

//    lvr2::PointBufferPtr cloud = model->m_pointCloud;
//    lvr2::PointBufferPtr cloud_filtered(new lvr2::PointBuffer());
//    int type;
//    std::map<std::string, lvr2::Channel<float> > floatChannels;
//    type = cloud->getAllChannelsOfType<float>(floatChannels);
//
//    std::vector<lvr2::FloatChannelPtr> channels;
//    for(auto channelPair: floatChannels)
//    {
//        std::string name = channelPair.first;
//        auto channel2 = channelPair.second;
//        lvr2::FloatChannelPtr filtered(new lvr2::FloatChannel(j, channel2.width()));
//        channels.push_back(filtered);
//        for(size_t i = 0; i < j; ++i)
//        {
//            size_t index = inlier[i];
//            for(size_t k = 0; k < channel2.width(); ++k)
//            {
//                (*filtered)[i][k] = channel2[index][k];
//            }
//        }
////        cloud->removeFloatChannel(name);
//        cloud_filtered->addFloatChannel(filtered, name);
//
//    }
//
//    std::vector<lvr2::UCharChannelPtr> channels_uchar;
//    std::map<std::string, lvr2::Channel<unsigned char> >  uCharChannels;
//    int ucharType = cloud->getAllChannelsOfType<unsigned char>(uCharChannels);
//    for(auto channelPair: uCharChannels)
//    {
//        std::string name = channelPair.first;
//        auto channel2 = channelPair.second;
//        lvr2::UCharChannelPtr filtered(new lvr2::UCharChannel(j, channel2.width()));
//        channels_uchar.push_back(filtered);
//        for(size_t i = 0; i < j; ++i)
//        {
//            size_t index = inlier[i];
//            for(size_t k = 0; k < channel2.width(); ++k)
//            {
//                (*filtered)[i][k] = channel2[index][k];
//            }
//        }
////        cloud->removeUCharChannel(name);
//        cloud_filtered->addUCharChannel(filtered, name);
//    }
//    lvr2::ModelPtr model_filtered(new lvr2::Model(cloud_filtered, lvr2::MeshBufferPtr()));
    lvr2::ModelFactory::saveModel(model, argv[2]);

    return 0;

}
