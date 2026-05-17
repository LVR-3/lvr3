/**
 * Copyright (c) 2019, University Osnabrück
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
 * ChunkingPipeline<BaseVecT>.cpp
 *
 * @date 27.11.2019
 * @author Marcel Wiegand
 */

#include "lvr2/algorithm/ChunkingPipeline.hpp"

#include <yaml-cpp/yaml.h>

#include "lvr2/algorithm/NormalAlgorithms.hpp"
#include "lvr2/algorithm/GeometryAlgorithms.hpp"
#include "lvr2/algorithm/FinalizeAlgorithms.hpp"
#include "lvr2/config/LSROptionsYamlExtensions.hpp"
#include "lvr2/config/SLAMOptionsYamlExtensions.hpp"
#include "lvr2/registration/RegistrationPipeline.hpp"
#include "lvr2/io/scan.hpp"
#include <lvr2/util/Logging.hpp>



namespace lvr2
{
template <typename BaseVecT>
ChunkingPipeline<BaseVecT>::ChunkingPipeline(
        const std::filesystem::path& hdf5Path,
        const std::filesystem::path& configPath,
        std::shared_ptr<ChunkManager> chunkManager) :  m_hdf5Path(hdf5Path), m_configPath(configPath)
{
    if (chunkManager != nullptr)
    {
        m_chunkManager = chunkManager;
    }
    else
    {
        m_chunkManager = std::make_shared<ChunkManager>(m_hdf5Path.string());
    }

    parseYAMLConfig();
}

template <typename BaseVecT>
void ChunkingPipeline<BaseVecT>::parseYAMLConfig()
{
    if (std::filesystem::exists(m_configPath) && std::filesystem::is_regular_file(m_configPath))
    {
        YAML::Node config = YAML::LoadFile(m_configPath.string());

        if (config["lvr2_registration"])
        {
                        lvr2::log::info("{}", "Found config entry for lvr2_registration.");
            m_regOptions = config["lvr2_registration"].as<SLAMOptions>();
        }

        if (config["lvr2_largescale_reconstruct"])
        {
                        lvr2::log::info("{}", "Found config entry for lvr2_largescale_reconstruct.");
            m_lsrOptions = config["lvr2_largescale_reconstruct"].as<LSROptions>();
        }

        if (config["lvr2_practicability_analysis"] && config["lvr2_practicability_analysis"].IsMap())
        {
                        lvr2::log::info("{}", "Found config entry for lvr2_practicability_analysis.");
            YAML::Node practicabilityConfig = config["lvr2_practicability_analysis"];
            if (practicabilityConfig["roughnessRadius"])
            {
                m_roughnessRadius = practicabilityConfig["roughnessRadius"].as<double>();
            }
            if (practicabilityConfig["heightDifferencesRadius"])
            {
                m_heightDifferencesRadius = practicabilityConfig["heightDifferencesRadius"].as<double>();
            }
            if (practicabilityConfig["layers"] && practicabilityConfig["layers"].IsSequence())
            {
                m_practicabilityLayers = practicabilityConfig["layers"].as<std::vector<float>>();
            }
        }
    }
    else
    {
                lvr2::log::info("{}", "Config file does not exist or is not a regular file!");
    }
}

template <typename BaseVecT>
void ChunkingPipeline<BaseVecT>::practicabilityAnalysis(HalfEdgeMesh<BaseVecT>& hem, MeshBufferPtr meshBuffer)
{
    // Calc face normals
    DenseFaceMap <Normal<float>> faceNormals = calcFaceNormals(hem);
    // Calc vertex normals
    DenseVertexMap <Normal<float>> vertexNormals = calcVertexNormals(hem, faceNormals);
    // Calc average vertex angles
    DenseVertexMap<float> averageAngles = calcAverageVertexAngles(hem, vertexNormals);
    // Calc roughness
    DenseVertexMap<float> roughness = calcVertexRoughness(hem, m_roughnessRadius, vertexNormals);
    // Calc vertex height differences
    DenseVertexMap<float> heightDifferences = calcVertexHeightDifferences(hem, m_heightDifferencesRadius);

    // create and fill channels
    FloatChannel faceNormalChannel(faceNormals.numValues(), channel_type < Normal < float >> ::w);
    Index i = 0;
    for (auto handle : FaceIteratorProxy<BaseVecT>(hem)) {
        faceNormalChannel[i++] = faceNormals[handle]; //TODO handle deleted map values.
    }

    FloatChannel vertexNormalsChannel(vertexNormals.numValues(), channel_type<Normal<float>>::w);
    FloatChannel averageAnglesChannel(averageAngles.numValues(), channel_type<float>::w);
    FloatChannel roughnessChannel(roughness.numValues(), channel_type<float>::w);
    FloatChannel heightDifferencesChannel(heightDifferences.numValues(), channel_type<float>::w);

    Index j = 0;
    for (auto handle : VertexIteratorProxy<BaseVecT>(hem))
    {
        vertexNormalsChannel[j] = vertexNormals[handle]; //TODO handle deleted map values.
        averageAnglesChannel[j] = averageAngles[handle]; //TODO handle deleted map values.
        roughnessChannel[j] = roughness[handle]; //TODO handle deleted map values.
        heightDifferencesChannel[j] = heightDifferences[handle]; //TODO handle deleted map values.
        j++;
    }

    // add channels to mesh buffer
    meshBuffer->add("face_normals", faceNormalChannel);
    meshBuffer->add("vertex_normals", vertexNormalsChannel);
    meshBuffer->add("average_angles", averageAnglesChannel);
    meshBuffer->add("roughness", roughnessChannel);
    meshBuffer->add("height_diff", heightDifferencesChannel);
}

template <typename BaseVecT>
bool ChunkingPipeline<BaseVecT>::getScanProject(const std::filesystem::path& dirPath)
{
    auto hdf5Project = lvr2::io::scan::load_project(
        m_hdf5Path.string(),
        lvr2::io::scan::LoadOptions::hdf5());
    if (!hdf5Project)
    {
                lvr2::log::error("{}{}", "Could not load existing HDF5 scan project: ", hdf5Project.error().message);
        return false;
    }
    ScanProjectPtr scanProjectPtr = hdf5Project.value();

    auto directoryProject = lvr2::io::scan::load_project(
        dirPath.string(),
        lvr2::io::scan::LoadOptions::directory_raw_ply());
    if (!directoryProject)
    {
                lvr2::log::error("{}{}", "Could not load directory scan project: ", directoryProject.error().message);
        return false;
    }
    ScanProjectPtr dirScanProject = directoryProject.value();

    ScanProjectEditMark tmpScanProject;
    std::vector<bool> init(scanProjectPtr->positions.size(), false);
    tmpScanProject.changed = init;

    const auto existingPositions = scanProjectPtr->positions.size();
    const auto directoryPositions = dirScanProject->positions.size();
    const auto newPositions = directoryPositions > existingPositions
        ? directoryPositions - existingPositions
        : 0;
        lvr2::log::info("{}{}{}", "Found ", newPositions, " new scanPosition(s)");
    for (std::size_t i = existingPositions; i < directoryPositions; i++)
    {
        scanProjectPtr->positions.push_back(dirScanProject->positions[i]);
        tmpScanProject.changed.push_back(true);
    }

    tmpScanProject.project = scanProjectPtr;
    m_scanProject = std::make_shared<ScanProjectEditMark>(tmpScanProject);
    m_scanProject->changed.resize(scanProjectPtr->positions.size());

    return true;
}

template <typename BaseVecT>
bool ChunkingPipeline<BaseVecT>::start(const std::filesystem::path& scanDir)
{
    if (m_running)
    {
        std::cout << "Chunking Pipeline is already running!" << std::endl;
        return false;
    }

    m_running = true;

        lvr2::log::info("{}", "Starting chunking pipeline...");

        lvr2::log::info("{}", "Starting import tool...");

    if (!getScanProject(scanDir))
    {
        std::cout << "Import failed..." << std::endl;
        std::cout << "Aborting chunking pipeline!" << std::endl;

        m_running = false;
        return false;
    }

        lvr2::log::info("{}", "Finished import!");

        lvr2::log::info("{}", "Starting registration...");
    RegistrationPipeline registration(&m_regOptions, m_scanProject);
    registration.doRegistration();
        lvr2::log::info("{}", "Finished registration!");

    // Save raw data
    auto hdf5Store = lvr2::io::scan::open_hdf5(
        m_hdf5Path.string(),
        lvr2::io::scan::Schema::hdf5(),
        lvr2::io::storage::LoadMode::Lazy);
    if (!hdf5Store)
    {
                lvr2::log::error("{}{}", "Could not open HDF5 output project: ", hdf5Store.error().message);
        m_running = false;
        return false;
    }

    for (size_t idx = 0; idx < m_scanProject->changed.size(); idx++)
    {
        if (m_scanProject->changed[idx])
        {
            // Only save changed scanPositions
            auto saved = hdf5Store.value().save_position(idx, m_scanProject->project->positions[idx]);
            if (!saved)
            {
                                lvr2::log::error("{}{}{}{}", "Could not save changed scan position ", idx, ": ", saved.error().message);
                m_running = false;
                return false;
            }
        }
    }

    // Remove hyperspectral data from memory
    for (ScanPositionPtr pos : m_scanProject->project->positions)
    {
        for(auto cam : pos->hyperspectral_cameras)
        {
            cam.reset(new HyperspectralCamera);
        }
        //pos->hyperspectralCamera.reset(new HyperspectralCamera);
    }

        lvr2::log::info("{}", "Starting large scale reconstruction...");
    LargeScaleReconstruction<BaseVecT> lsr(m_lsrOptions);
    BoundingBox<BaseVecT> newChunksBB;
    lsr.chunkAndReconstruct(m_scanProject, newChunksBB, m_chunkManager);
        lvr2::log::info("{}", "Finished large scale reconstruction!");

    for (auto layer : m_lsrOptions.voxelSizes)
    {
        std::string voxelSizeStr = "[Layer " + std::to_string(layer) + "] ";
                lvr2::log::info("{}{}", voxelSizeStr, "Starting mesh generation...");
        HalfEdgeMesh<BaseVecT> hem = lsr.getPartialReconstruct(
                newChunksBB,
                m_chunkManager,
                layer);
                lvr2::log::info("{}{}", voxelSizeStr, "Finished mesh generation!");

                lvr2::log::info("{}{}", voxelSizeStr, "Starting mesh buffer creation...");
        lvr2::SimpleFinalizer<BaseVecT> finalize;
        MeshBufferPtr meshBuffer = MeshBufferPtr(finalize.apply(hem));
                lvr2::log::info("{}{}", voxelSizeStr, "Finished mesh buffer creation!");

        auto foundIt = std::find(m_practicabilityLayers.begin(), m_practicabilityLayers.end(), layer);
        if (foundIt != m_practicabilityLayers.end())
        {
                        lvr2::log::info("{}{}", voxelSizeStr, "Starting practicability analysis...");
            practicabilityAnalysis(hem, meshBuffer);
                        lvr2::log::info("{}{}", voxelSizeStr, "Finished practicability analysis!");
        }
        else
        {
                        lvr2::log::info("{}{}", voxelSizeStr, "Skipping practicability analysis...");
        }

                lvr2::log::info("{}{}", voxelSizeStr, "Starting chunking and saving of mesh buffer...");
        // TODO: get maxChunkOverlap size
        // TODO: savePath is not used in buildChunks (remove it?)
        m_chunkManager->buildChunks(meshBuffer, 0.1f, "", "mesh_" + std::to_string(layer));
                lvr2::log::info("{}{}", voxelSizeStr, "Finished chunking and saving of mesh buffer!");
    }

        lvr2::log::info("{}", "Finished chunking pipeline!");

    m_running = false;

    return true;
}

} /* namespace lvr2 */
