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

#include <iostream>
#include <vector>
#include <algorithm>

#include <string.h>

#include "lvr2/io/ModelFactory.hpp"
#include "lvr2/util/Timestamp.hpp"
#include <lvr2/util/Logging.hpp>
#include "lvr2/algorithm/NormalAlgorithms.hpp"
#include "lvr2/algorithm/GeometryAlgorithms.hpp"
#include "lvr2/geometry/HalfEdgeMesh.hpp"
#include "lvr2/algorithm/ReductionAlgorithms.hpp"

#include "Options.hpp"

#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <memory>

#include "lvr2/io/MeshStores.hpp"

using namespace lvr2;

int main( int argc, char ** argv )
{
  hdf5meshtool::Options options(argc, argv);
    lvr2::log::info("{}", "Load HDF5 file structure...");

  using MeshToolStore = lvr2::io::mesh::Hdf5MeshStore;

  // Get extension
  std::filesystem::path selectedFile(options.getInputFile());
  std::string extension = selectedFile.extension().string();
  MeshBufferPtr meshBuffer;
  MeshToolStore hdf5In;
  bool readFromHdf5 = false;

  // check extension
  if (extension == ".h5")
  {
    hdf5In.open(options.getInputFile());
    if (hdf5In.is_open())
    {
        readFromHdf5 = true;
    }
    meshBuffer = hdf5In.load_mesh(options.getMeshName());
  }
  else // use model reader
  {
    ModelPtr model = ModelFactory::readModel(options.getInputFile());
    meshBuffer = model->m_mesh;
  }

  if (meshBuffer != nullptr)
  {
    HalfEdgeMesh<BaseVector<float>> hem;
    size_t numFaces = meshBuffer->numFaces();
    size_t numVertices = meshBuffer->numVertices();
        lvr2::log::info("{}{}{}{}{}", "Building mesh from buffers with ", numFaces, " faces and ", numVertices, " vertices...");

    floatArr vertices = meshBuffer->getVertices();
    indexArray indices = meshBuffer->getFaceIndices();

    for(size_t i = 0; i < numVertices; i++)
    {
      size_t pos = 3 * i;
      hem.addVertex(BaseVector<float>(
          vertices[pos],
          vertices[pos + 1],
          vertices[pos + 2]));
    }

    size_t invalid_face_cnt = 0;
    for(size_t i = 0; i < numFaces; i++) {
      size_t pos = 3 * i;
      VertexHandle v1(indices[pos]);
      VertexHandle v2(indices[pos + 1]);
      VertexHandle v3(indices[pos + 2]);
      try{
        hem.addFace(v1, v2, v3);
      }
      catch(lvr2::PanicException)
      {
        invalid_face_cnt++;
      }
    }
    if (invalid_face_cnt > 0)
    {
            lvr2::log::info("{}{}", "Invalid faces found during HalfEdgeMesh construction: ", invalid_face_cnt);
    }

    MeshToolStore hdf5;
    bool writeToHdf5Input = false;
    if (readFromHdf5 && options.getInputFile() == options.getOutputFile())
    {
      hdf5.open(options.getInputFile());
      writeToHdf5Input = true;
    }
    else
    {
      hdf5.open(options.getOutputFile());
    }
    hdf5.set_mesh_name(options.getMeshName());

    // face normals
    DenseFaceMap<Normal<float>> faceNormals;
    std::optional<DenseFaceMap<Normal<float>>> faceNormalsOpt;
    if (readFromHdf5)
    {
      faceNormalsOpt = hdf5In.getDenseAttributeMap<DenseFaceMap<Normal<float>>>("face_normals");
    }
    if (faceNormalsOpt)
    {
            lvr2::log::info("{}", "Using existing face normals...");
      faceNormals = *faceNormalsOpt;
    }
    else
    {
            lvr2::log::info("{}", "Computing face normals...");
      faceNormals = calcFaceNormals(hem);
    }
    if(options.getEdgeCollapseNum() > 0)
    {
      double percent = options.getEdgeCollapseNum() > 100 ? 1 : options.getEdgeCollapseNum() / 100.0;
      size_t numCollapse = static_cast<size_t>(percent * hem.numEdges());
            lvr2::log::info("{}{}{}{}{}{}{}", "Reduce mesh by collapsing ", percent * 100, "% of the edges (", numCollapse, " out of ", hem.numEdges(), ")");
      simpleMeshReduction(hem, numCollapse, faceNormals);
    }

    // add mesh to file
    if(options.getEdgeCollapseNum() > 0 || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding mesh to file...");
      // add mesh to file
      bool addedMesh = hdf5.addMesh(hem);
      if (addedMesh)
      {
                lvr2::log::info("{}", "successfully added mesh");
      }
      else
      {
                lvr2::log::error("{}", "could not add the mesh!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Mesh already included.");
    }

    // add face normals to file
    if(!faceNormalsOpt || options.getEdgeCollapseNum() > 0 || !writeToHdf5Input)
    {
      bool addedFaceNormals = hdf5.addDenseAttributeMap<DenseFaceMap<Normal<float>>>(
              hem, faceNormals, "face_normals");
      if(addedFaceNormals)
      {
                lvr2::log::info("{}", "successfully added face normals");
      }
      else
      {
                lvr2::log::error("{}", "could not add face normals!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Face normals already included.");
    }

    // vertex normals
    DenseVertexMap<Normal<float>> vertexNormals;
    std::optional<DenseVertexMap<Normal<float>>> vertexNormalsOpt;
    if (readFromHdf5)
    {
      vertexNormalsOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<Normal<float>>>("vertex_normals");
    }
    if (vertexNormalsOpt)
    {
            lvr2::log::info("{}", "Using existing vertex normals...");
      vertexNormals = *vertexNormalsOpt;
    }
    else if (meshBuffer != nullptr && meshBuffer->hasVertexNormals())
    {
            lvr2::log::info("{}", "Using existing vertex normals from mesh buffer...");
      const FloatChannelOptional channel_opt = meshBuffer->getChannel<float>("vertex_normals");
      if (channel_opt && channel_opt.value().width() == 3 and channel_opt.value().numElements() == hem.numVertices())
      {
        auto &channel = channel_opt.value();
        vertexNormals.reserve(channel.numElements());
        for (size_t i = 0; i < channel.numElements(); i++)
        {
          vertexNormals.insert(VertexHandle(i), channel[i]);
        }
      }
      else
      {
                lvr2::log::error("{}", "Error while reading vertex normals...");
      }
    }

    if(vertexNormals.numValues() == 0)
    {
            lvr2::log::info("{}", "Computing vertex normals...");
      vertexNormals = calcVertexNormals(hem, faceNormals);
    }
    if (!vertexNormalsOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding vertex normals...");
      bool addedVertexNormals = hdf5.addDenseAttributeMap<DenseVertexMap<Normal<float>>>(
              hem, vertexNormals, "vertex_normals");
      if (addedVertexNormals)
      {
                lvr2::log::info("{}", "successfully added vertex normals");
      }
      else
      {
                lvr2::log::error("{}", "could not add vertex normals!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Vertex normals already included.");
    }

    // vertex colors
    using color = std::array<uint8_t, 3>;
    DenseVertexMap<color> colors;
    std::optional<DenseVertexMap<color>> colorsOpt;
    ChannelOptional<uint8_t> channel_opt;
    bool colorsFoundInSource = false;
    if (readFromHdf5)
    {
      colorsOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<color>>("vertex_colors");
    }
    if (colorsOpt)
    {
            lvr2::log::info("{}", "Using existing vertex colors...");
      colors = *colorsOpt;
      colorsFoundInSource = true;
    }
    else if (meshBuffer != nullptr && (channel_opt = meshBuffer->getChannel<uint8_t>("vertex_colors"))
      && channel_opt && channel_opt.value().width() == 3 && channel_opt.value().numElements() == hem.numVertices()) {
            lvr2::log::info("{}", "Using existing colors from mesh buffer...");
      colorsFoundInSource = true;

      auto &channel = channel_opt.value();
      colors.reserve(channel.numElements());
      for (size_t i = 0; i < channel.numElements(); i++)
      {
        colors.insert(VertexHandle(i), channel[i]);
      }
    }
    if (!colorsOpt || !writeToHdf5Input)
    {
      if (colorsFoundInSource)
      {
                lvr2::log::info("{}", "Adding vertex colors found in source...");
        bool addedVertexColors = hdf5.addDenseAttributeMap<DenseVertexMap<color>>(
            hem, colors, "vertex_colors");
        if (addedVertexColors)
        {
                    lvr2::log::info("{}", "successfully added vertex colors");
        }
        else
        {
                    lvr2::log::error("{}", "could not add vertex colors!");
        }
      }
      else
      {
                    lvr2::log::info("{}", "Skipping vertex colors: No colors found in input file.");
      }
    }
    else
    {
            lvr2::log::info("{}", "Vertex colors already included.");
    }


    // vertex average angles
    DenseVertexMap<float> averageAngles;
    std::optional<DenseVertexMap<float>> averageAnglesOpt;
    if (readFromHdf5)
    {
      averageAnglesOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<float>>("average_angles");
    }
    if (averageAnglesOpt)
    {
            lvr2::log::info("{}", "Using existing vertex average angles...");
      averageAngles = *averageAnglesOpt;
    }
    else
    {
            lvr2::log::info("{}", "Computing vertex average angles...");
      averageAngles = calcAverageVertexAngles(hem, vertexNormals);
    }
    if (!averageAnglesOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding vertex average angles...");
      bool addedAverageAngles = hdf5.addDenseAttributeMap<DenseVertexMap<float>>(
              hem, averageAngles, "average_angles");
      if (addedAverageAngles)
      {
                lvr2::log::info("{}", "successfully added vertex average angles");
      }
      else
      {
                lvr2::log::error("{}", "could not add vertex average angles!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Vertex average angles already included.");
    }

    // roughness
    DenseVertexMap<float> roughness;
    std::optional<DenseVertexMap<float>> roughnessOpt;
    if (readFromHdf5)
    {
      roughnessOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<float>>("roughness");
    }
    if (roughnessOpt)
    {
            lvr2::log::info("{}", "Using existing roughness...");
      roughness = *roughnessOpt;
    }
    else
    {
            lvr2::log::info("{}{}{}", "Computing roughness with a local radius of ", options.getLocalRadius(), "m ...");
      roughness = calcVertexRoughness(hem, options.getLocalRadius(), vertexNormals);
    }
    if (!roughnessOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding roughness...");
      bool addedRoughness = hdf5.addDenseAttributeMap<DenseVertexMap<float>>(
              hem, roughness, "roughness");
      if (addedRoughness)
      {
                lvr2::log::info("{}", "successfully added roughness.");
      }
      else
      {
                lvr2::log::error("{}", "could not add roughness!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Roughness already included.");
    }

    // height differences
    DenseVertexMap<float> heightDifferences;
    std::optional<DenseVertexMap<float>> heightDifferencesOpt;
    if (readFromHdf5)
    {
      heightDifferencesOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<float>>("height_diff");
    }
    if (heightDifferencesOpt)
    {
            lvr2::log::info("{}", "Using existing height differences...");
      heightDifferences = *heightDifferencesOpt;
    }
    else
    {
            lvr2::log::info("{}{}{}", "Computing height diff with a local radius of ", options.getLocalRadius(), "m ...");
      heightDifferences = calcVertexHeightDifferences(hem, vertexNormals, options.getLocalRadius());
    }
    if (!heightDifferencesOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding roughness...");
      bool addedHeightDiff = hdf5.addDenseAttributeMap<DenseVertexMap<float>>(
              hem, heightDifferences, "height_diff");
      if (addedHeightDiff)
      {
                lvr2::log::info("{}", "successfully added height differences.");
      }
      else
      {
                lvr2::log::error("{}", "could not add height differences!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Height differences already included.");
    }


    // border costs
    DenseVertexMap<float> borderCosts;
    std::optional<DenseVertexMap<float>> borderCostsOpt;
    if (readFromHdf5)
    {
      borderCostsOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<float>>("border");
    }
    if (borderCostsOpt)
    {
            lvr2::log::info("{}", "Using existing border costs...");
      borderCosts = *borderCostsOpt;
    }
    else
    {
            lvr2::log::info("{}{}{}", "Computing border costs ... Setting border vertex costs to ", options.getBorderVertexCost(), " ...");
      borderCosts = calcBorderCosts(hem, 1.0);
    }
    if (!borderCostsOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding border costs...");
      bool addedBorderCosts = hdf5.addDenseAttributeMap<DenseVertexMap<float>>(
              hem, borderCosts, "border");
      if (addedBorderCosts)
      {
                lvr2::log::info("{}", "successfully added border costs.");
      }
      else
      {
                lvr2::log::error("{}", "could not add border costs!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Border costs already included.");
    }

    // Free space above vertices
    DenseVertexMap<float> freeSpace;
    std::optional<DenseVertexMap<float>> freeSpaceOpt;
    if (readFromHdf5)
    {
      freeSpaceOpt = hdf5In.getDenseAttributeMap<DenseVertexMap<float>>("freespace");
    }
    if (freeSpaceOpt)
    {
            lvr2::log::info("{}", "Using existing free space ...");
      freeSpace = freeSpaceOpt.value();
    }
    else
    {
            lvr2::log::info("{}", "Computing free space ...");
      freeSpace = calcNormalClearance(hem, vertexNormals);
    }
    if (!freeSpaceOpt || !writeToHdf5Input)
    {
            lvr2::log::info("{}", "Adding free space...");
      bool addedBorderCosts = hdf5.addDenseAttributeMap<DenseVertexMap<float>>(
              hem, freeSpace, "freespace");
      if (addedBorderCosts)
      {
                lvr2::log::info("{}", "successfully added free space.");
      }
      else
      {
                lvr2::log::error("{}", "could not add free space!");
      }
    }
    else
    {
            lvr2::log::info("{}", "Free space already included.");
    }
  }
  else
  {
        lvr2::log::error("{}{}", "Error reading mesh data from ", options.getOutputFile());
  }

  return 0;
}
