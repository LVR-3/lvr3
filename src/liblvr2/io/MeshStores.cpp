#include "lvr2/io/MeshStores.hpp"

#include "lvr2/io/YAML.hpp"
#include "lvr2/io/kernels/DirectoryKernel.hpp"
#include "lvr2/io/kernels/HDF5Kernel.hpp"
#include "lvr2/io/schema/MeshSchemaDirectory.hpp"
#include "lvr2/io/schema/MeshSchemaHDF5.hpp"
#include "lvr2/texture/Material.hpp"
#include "lvr2/texture/Texture.hpp"
#include "lvr2/types/Model.hpp"
#include "lvr2/util/Util.hpp"

#include <filesystem>
#include <memory>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace lvr2::io::mesh
{
namespace
{

constexpr const char* kDirectGeometryGroup = "geometry";
constexpr const char* kDirectMaterialsGroup = "materials";
constexpr const char* kDirectTexturesGroup = "textures";
constexpr const char* kDirectId = "MeshStore";
constexpr const char* kDirectClass = "MeshBuffer";

YAML::Node arrayMeta(const std::string& dataType, std::vector<std::size_t> shape)
{
    YAML::Node node;
    node["entity"] = "channel";
    node["type"] = "array";
    node["data_type"] = dataType;
    node["shape"] = std::move(shape);
    return node;
}

std::vector<std::size_t> shapeFrom(const YAML::Node& node)
{
    if (node["shape"])
    {
        return node["shape"].as<std::vector<std::size_t>>();
    }
    if (node["SHAPE"])
    {
        return node["SHAPE"].as<std::vector<std::size_t>>();
    }
    return {};
}

std::string dataTypeFrom(const YAML::Node& node)
{
    if (node["data_type"])
    {
        return node["data_type"].as<std::string>();
    }
    if (node["TYPE"])
    {
        return node["TYPE"].as<std::string>();
    }
    return {};
}

template<typename T>
void saveKernelArray(const lvr2::FileKernelPtr& kernel,
                     const lvr2::Description& description,
                     const std::vector<std::size_t>& shape,
                     const std::shared_ptr<T[]>& data,
                     const std::string& dataType)
{
    kernel->saveArray(*description.dataRoot, *description.data, shape, data);
    kernel->saveMetaYAML(*description.metaRoot, *description.meta, arrayMeta(dataType, shape));
}

template<typename T>
std::shared_ptr<T[]> loadKernelArray(const lvr2::FileKernelPtr& kernel,
                                       const lvr2::Description& description,
                                       std::vector<std::size_t>& shape)
{
    return kernel->template loadArray<T>(*description.dataRoot, *description.data, shape);
}

void saveVertices(const lvr2::FileKernelPtr& kernel,
                  const lvr2::MeshSchemaPtr& schema,
                  const std::string& meshName,
                  const lvr2::MeshBufferPtr& mesh)
{
    if (mesh->hasVertices())
    {
        saveKernelArray(kernel,
                        schema->vertexChannel(meshName, "coordinates"),
                        {mesh->numVertices(), 3},
                        mesh->getVertices(),
                        "float");
    }

    if (mesh->hasVertexNormals())
    {
        saveKernelArray(kernel,
                        schema->vertexChannel(meshName, "normals"),
                        {mesh->numVertices(), 3},
                        mesh->getVertexNormals(),
                        "float");
    }

    if (mesh->hasVertexColors())
    {
        std::size_t colorWidth = 0;
        auto colors = mesh->getVertexColors(colorWidth);
        saveKernelArray(kernel,
                        schema->vertexChannel(meshName, "colors"),
                        {mesh->numVertices(), colorWidth},
                        colors,
                        "uchar");
    }

    if (mesh->getTextureCoordinates())
    {
        saveKernelArray(kernel,
                        schema->vertexChannel(meshName, "texture_coordinates"),
                        {mesh->numVertices(), 2},
                        mesh->getTextureCoordinates(),
                        "float");
    }
}

void loadVertices(const lvr2::FileKernelPtr& kernel,
                  const lvr2::MeshSchemaPtr& schema,
                  const std::string& meshName,
                  const lvr2::MeshBufferPtr& mesh)
{
    auto coordinates = schema->vertexChannel(meshName, "coordinates");
    if (kernel->exists(*coordinates.dataRoot, *coordinates.data))
    {
        YAML::Node meta;
        kernel->loadMetaYAML(*coordinates.metaRoot, *coordinates.meta, meta);
        auto shape = shapeFrom(meta);
        if (dataTypeFrom(meta).empty() || dataTypeFrom(meta) == "float")
        {
            auto vertices = loadKernelArray<float>(kernel, coordinates, shape);
            if (vertices && !shape.empty())
            {
                mesh->setVertices(std::move(vertices), shape[0]);
            }
        }
    }

    auto normals = schema->vertexChannel(meshName, "normals");
    if (kernel->exists(*normals.dataRoot, *normals.data))
    {
        YAML::Node meta;
        kernel->loadMetaYAML(*normals.metaRoot, *normals.meta, meta);
        auto shape = shapeFrom(meta);
        auto data = loadKernelArray<float>(kernel, normals, shape);
        if (data)
        {
            mesh->setVertexNormals(std::move(data));
        }
    }

    auto colorsDesc = schema->vertexChannel(meshName, "colors");
    if (kernel->exists(*colorsDesc.dataRoot, *colorsDesc.data))
    {
        YAML::Node meta;
        kernel->loadMetaYAML(*colorsDesc.metaRoot, *colorsDesc.meta, meta);
        auto shape = shapeFrom(meta);
        auto colors = loadKernelArray<unsigned char>(kernel, colorsDesc, shape);
        if (colors && shape.size() > 1)
        {
            mesh->setVertexColors(std::move(colors), shape[1]);
        }
    }

    auto textureCoordinates = schema->vertexChannel(meshName, "texture_coordinates");
    if (kernel->exists(*textureCoordinates.dataRoot, *textureCoordinates.data))
    {
        YAML::Node meta;
        kernel->loadMetaYAML(*textureCoordinates.metaRoot, *textureCoordinates.meta, meta);
        auto shape = shapeFrom(meta);
        auto coordinatesData = loadKernelArray<float>(kernel, textureCoordinates, shape);
        if (coordinatesData)
        {
            mesh->setTextureCoordinates(coordinatesData);
        }
    }
}

bool saveFaceIndices(const lvr2::FileKernelPtr& kernel,
                     const lvr2::MeshSchemaPtr& schema,
                     const std::string& meshName,
                     const lvr2::MeshBufferPtr& mesh)
{
    if (!mesh->hasFaces())
    {
        return false;
    }
    saveKernelArray(kernel,
                    schema->faceIndices(meshName),
                    {mesh->numFaces(), 3},
                    mesh->getFaceIndices(),
                    "uint");
    return true;
}

bool saveFaceColors(const lvr2::FileKernelPtr& kernel,
                    const lvr2::MeshSchemaPtr& schema,
                    const std::string& meshName,
                    const lvr2::MeshBufferPtr& mesh)
{
    if (!mesh->hasFaceColors())
    {
        return false;
    }
    std::size_t colorWidth = 0;
    const auto& colors = mesh->getFaceColors(colorWidth);
    saveKernelArray(kernel,
                    schema->faceColors(meshName),
                    {mesh->numFaces(), colorWidth},
                    colors,
                    "uchar");
    return true;
}

bool saveFaceNormals(const lvr2::FileKernelPtr& kernel,
                     const lvr2::MeshSchemaPtr& schema,
                     const std::string& meshName,
                     const lvr2::MeshBufferPtr& mesh)
{
    if (!mesh->hasFaceNormals())
    {
        return false;
    }
    saveKernelArray(kernel,
                    schema->faceNormals(meshName),
                    {mesh->numFaces(), 3},
                    mesh->getFaceNormals(),
                    "float");
    return true;
}

bool saveFaceMaterialIndices(const lvr2::FileKernelPtr& kernel,
                             const lvr2::MeshSchemaPtr& schema,
                             const std::string& meshName,
                             const lvr2::MeshBufferPtr& mesh)
{
    auto materials = mesh->getFaceMaterialIndices();
    if (!materials)
    {
        return false;
    }
    saveKernelArray(kernel,
                    schema->faceMaterialIndices(meshName),
                    {mesh->numFaces(), 1},
                    materials,
                    "uint");
    return true;
}

void loadFaces(const lvr2::FileKernelPtr& kernel,
               const lvr2::MeshSchemaPtr& schema,
               const std::string& meshName,
               const lvr2::MeshBufferPtr& mesh)
{
    auto indices = schema->faceIndices(meshName);
    if (kernel->exists(*indices.dataRoot, *indices.data))
    {
        std::vector<std::size_t> shape;
        auto data = loadKernelArray<indexArray::element_type>(kernel, indices, shape);
        if (data && !shape.empty())
        {
            mesh->setFaceIndices(data, shape[0]);
        }
    }

    auto colors = schema->faceColors(meshName);
    if (kernel->exists(*colors.dataRoot, *colors.data))
    {
        std::vector<std::size_t> shape;
        auto data = loadKernelArray<ucharArr::element_type>(kernel, colors, shape);
        if (data && shape.size() > 1)
        {
            mesh->setFaceColors(data, shape[1]);
        }
    }

    auto normals = schema->faceNormals(meshName);
    if (kernel->exists(*normals.dataRoot, *normals.data))
    {
        std::vector<std::size_t> shape;
        auto data = loadKernelArray<floatArr::element_type>(kernel, normals, shape);
        if (data)
        {
            mesh->setFaceNormals(data);
        }
    }

    auto materialIndices = schema->faceMaterialIndices(meshName);
    if (kernel->exists(*materialIndices.dataRoot, *materialIndices.data))
    {
        std::vector<std::size_t> shape;
        auto data = loadKernelArray<indexArray::element_type>(kernel, materialIndices, shape);
        if (data)
        {
            mesh->setFaceMaterialIndices(data);
        }
    }
}

void saveClusters(const lvr2::FileKernelPtr& kernel,
                  const lvr2::MeshSchemaPtr& schema,
                  const std::string& meshName,
                  const lvr2::MeshBufferPtr& mesh)
{
    std::vector<IndexChannel::DataType> combinedFaceIndices;
    std::vector<std::size_t> clusterRanges;

    std::size_t clusterIndex = 0;
    while (true)
    {
        const std::string clusterName = "cluster" + std::to_string(clusterIndex) + "_face_indices";
        auto cluster = mesh->getIndexChannel(clusterName);
        if (!cluster)
        {
            break;
        }

        const std::size_t clusterStart = combinedFaceIndices.size();
        combinedFaceIndices.insert(combinedFaceIndices.end(),
                                   cluster->dataPtr().get(),
                                   cluster->dataPtr().get() + cluster->numElements());
        const std::size_t clusterEnd = combinedFaceIndices.size();
        clusterRanges.push_back(clusterStart);
        clusterRanges.push_back(clusterEnd);
        ++clusterIndex;
    }

    if (!combinedFaceIndices.empty() && !clusterRanges.empty())
    {
        saveKernelArray(kernel,
                        schema->surfaceCombinedFaceIndices(meshName),
                        {combinedFaceIndices.size(), 1},
                        Util::convert_vector_to_shared_array(combinedFaceIndices),
                        "uint");
        saveKernelArray(kernel,
                        schema->surfaceFaceIndexRanges(meshName),
                        {clusterIndex, 2},
                        Util::convert_vector_to_shared_array(clusterRanges),
                        "uint");
    }

    auto materialChannel = mesh->getIndexChannel("cluster_material_indices");
    if (materialChannel)
    {
        saveKernelArray(kernel,
                        schema->surfaceMaterialIndices(meshName),
                        {materialChannel->numElements(), materialChannel->width()},
                        materialChannel->dataPtr(),
                        "uint");
    }
}

void loadClusters(const lvr2::FileKernelPtr& kernel,
                  const lvr2::MeshSchemaPtr& schema,
                  const std::string& meshName,
                  const lvr2::MeshBufferPtr& mesh)
{
    std::vector<std::size_t> combinedShape;
    auto combinedDesc = schema->surfaceCombinedFaceIndices(meshName);
    auto combined = loadKernelArray<indexArray::element_type>(kernel, combinedDesc, combinedShape);

    std::vector<std::size_t> rangeShape;
    auto rangeDesc = schema->surfaceFaceIndexRanges(meshName);
    auto ranges = loadKernelArray<indexArray::element_type>(kernel, rangeDesc, rangeShape);

    if (ranges && combined && !rangeShape.empty())
    {
        for (std::size_t i = 0; i < rangeShape[0]; ++i)
        {
            const std::size_t begin = ranges[i * 2 + 0];
            const std::size_t end = ranges[i * 2 + 1];
            const std::size_t length = end - begin;
            indexArray cluster(new indexArray::element_type[length]);
            std::copy(combined.get() + begin, combined.get() + end, cluster.get());
            mesh->addIndexChannel(cluster,
                                  "cluster" + std::to_string(i) + "_face_indices",
                                  length,
                                  1);
        }
    }

    auto materialDesc = schema->surfaceMaterialIndices(meshName);
    if (kernel->exists(*materialDesc.dataRoot, *materialDesc.data))
    {
        std::vector<std::size_t> shape;
        auto materialIndices = loadKernelArray<indexArray::element_type>(kernel, materialDesc, shape);
        if (materialIndices && shape.size() > 1)
        {
            auto channel = std::make_shared<IndexChannel>(shape[0], shape[1], materialIndices);
            mesh->addIndexChannel(channel, "cluster_material_indices");
        }
    }
}

void saveTexture(const lvr2::FileKernelPtr& kernel,
                 const lvr2::MeshSchemaPtr& schema,
                 const std::string& meshName,
                 std::size_t materialIndex,
                 const std::string& textureName,
                 const lvr2::Texture& texture)
{
    auto desc = schema->texture(meshName, materialIndex, textureName);
    const std::size_t byteCount = texture.m_width * texture.m_height * texture.m_numChannels * texture.m_numBytesPerChan;
    if (byteCount == 0)
    {
        return;
    }

    ucharArr data(new unsigned char[byteCount]);
    std::copy(texture.m_data, texture.m_data + byteCount, data.get());
    kernel->saveUCharArray(*desc.dataRoot,
                           *desc.data,
                           {static_cast<std::size_t>(texture.m_height),
                            static_cast<std::size_t>(texture.m_width),
                            static_cast<std::size_t>(texture.m_numChannels),
                            static_cast<std::size_t>(texture.m_numBytesPerChan)},
                           data);
    YAML::Node meta = texture;
    kernel->saveMetaYAML(*desc.metaRoot, *desc.meta, meta);
}

std::optional<lvr2::Texture> loadTexture(const lvr2::FileKernelPtr& kernel,
                                           const lvr2::MeshSchemaPtr& schema,
                                           const std::string& meshName,
                                           std::size_t materialIndex,
                                           const std::string& textureName)
{
    auto desc = schema->texture(meshName, materialIndex, textureName);
    if (!kernel->exists(*desc.dataRoot, *desc.data))
    {
        return std::nullopt;
    }

    YAML::Node meta;
    kernel->loadMetaYAML(*desc.metaRoot, *desc.meta, meta);
    lvr2::Texture texture = meta.as<lvr2::Texture>();
    texture.m_layerName = textureName;

    std::vector<std::size_t> dims;
    auto data = kernel->loadUCharArray(*desc.dataRoot, *desc.data, dims);
    const std::size_t byteCount = std::accumulate(dims.begin(), dims.end(), std::size_t{1}, std::multiplies<std::size_t>());
    std::copy(data.get(), data.get() + byteCount, texture.m_data);
    return texture;
}

void saveMaterials(const lvr2::FileKernelPtr& kernel,
                   const lvr2::MeshSchemaPtr& schema,
                   const std::string& meshName,
                   const lvr2::MeshBufferPtr& mesh)
{
    const auto& materials = mesh->getMaterials();
    const auto& textures = mesh->getTextures();
    for (std::size_t index = 0; index < materials.size(); ++index)
    {
        auto desc = schema->material(meshName, index);
        YAML::Node meta = materials[index];
        kernel->saveMetaYAML(*desc.metaRoot, *desc.meta, meta);

        const auto& material = materials[index];
        for (const auto& layer : material.m_layers)
        {
            const auto& texture = textures[layer.second.idx()];
            saveTexture(kernel, schema, meshName, index, texture.m_layerName, texture);
        }
        if (material.m_texture)
        {
            const auto& texture = textures[material.m_texture->idx()];
            saveTexture(kernel, schema, meshName, index, texture.m_layerName, texture);
        }
    }
}

void loadMaterials(const lvr2::FileKernelPtr& kernel,
                   const lvr2::MeshSchemaPtr& schema,
                   const std::string& meshName,
                   const lvr2::MeshBufferPtr& mesh)
{
    std::size_t count = 0;
    while (true)
    {
        auto desc = schema->material(meshName, count);
        if (!kernel->exists(*desc.dataRoot, *desc.data))
        {
            break;
        }
        ++count;
    }

    mesh->getMaterials().clear();
    mesh->getTextures().clear();
    mesh->getTextures().resize(count);

    for (std::size_t index = 0; index < count; ++index)
    {
        auto desc = schema->material(meshName, index);
        YAML::Node meta;
        kernel->loadMetaYAML(*desc.metaRoot, *desc.meta, meta);
        lvr2::Material material = meta.as<lvr2::Material>();

        if (kernel->exists(*desc.dataRoot + "/textures"))
        {
            std::vector<std::string> textureNames;
            kernel->subGroupNames(*desc.dataRoot + "/textures", textureNames);
            for (const auto& textureName : textureNames)
            {
                auto texture = loadTexture(kernel, schema, meshName, index, textureName);
                if (texture)
                {
                    if (mesh->getTextures().size() <= texture->m_index)
                    {
                        mesh->getTextures().resize(texture->m_index + 1);
                    }
                    material.m_layers.insert({texture->m_layerName, texture->m_index});
                    if (!material.m_texture)
                    {
                        material.m_texture = lvr2::TextureHandle(texture->m_index);
                    }
                    mesh->getTextures()[texture->m_index] = std::move(*texture);
                }
            }
        }

        mesh->getMaterials().push_back(material);
    }
}

template<typename T>
std::shared_ptr<T[]> loadArray(const std::shared_ptr<HighFive::File>& file,
                                 HighFive::Group& group,
                                 const std::string& datasetName,
                                 std::vector<std::size_t>& dimensions)
{
    std::shared_ptr<T[]> result;
    if (!file || !file->isValid() || !group.exist(datasetName))
    {
        return result;
    }

    auto dataset = group.getDataSet(datasetName);
    dimensions = dataset.getSpace().getDimensions();
    const std::size_t elementCount = std::accumulate(dimensions.begin(), dimensions.end(), std::size_t{1}, std::multiplies<std::size_t>());
    if (elementCount == 0)
    {
        return result;
    }

    result.reset(new T[elementCount]);
    dataset.read(result.get());
    return result;
}

template<typename T>
std::shared_ptr<T[]> loadArray(const std::shared_ptr<HighFive::File>& file,
                                 const std::string& groupName,
                                 const std::string& datasetName,
                                 std::vector<std::size_t>& dimensions)
{
    auto group = hdf5util::getGroup(file, groupName, false);
    return loadArray<T>(file, group, datasetName, dimensions);
}

template<typename T>
std::shared_ptr<T[]> loadArray(const std::shared_ptr<HighFive::File>& file,
                                 const std::string& groupName,
                                 const std::string& datasetName,
                                 std::size_t& size)
{
    std::vector<std::size_t> dimensions;
    auto result = loadArray<T>(file, groupName, datasetName, dimensions);
    size = std::accumulate(dimensions.begin(), dimensions.end(), std::size_t{1}, std::multiplies<std::size_t>());
    return result;
}

template<typename T>
void saveArray(const std::shared_ptr<HighFive::File>& file,
               bool compress,
               std::size_t defaultChunkSize,
               HighFive::Group& group,
               const std::string& datasetName,
               std::vector<std::size_t> dimensions,
               std::vector<hsize_t> chunkSizes,
               std::shared_ptr<T[]> data)
{
    if (!file || !file->isValid())
    {
        throw std::runtime_error("HDF5 mesh store is not open");
    }

    HighFive::DataSpace dataSpace(dimensions);
    HighFive::DataSetCreateProps properties;
    if (defaultChunkSize > 0)
    {
        for (std::size_t i = 0; i < chunkSizes.size(); ++i)
        {
            if (chunkSizes[i] > dimensions[i])
            {
                chunkSizes[i] = dimensions[i];
            }
        }
        properties.add(HighFive::Chunking(chunkSizes));
    }
    if (compress)
    {
        properties.add(HighFive::Deflate(9));
    }

    auto dataset = hdf5util::createDataset<T>(group, datasetName, dataSpace, properties);
    dataset->write_raw(data.get());
    file->flush();
}

template<typename T>
void saveArray(const std::shared_ptr<HighFive::File>& file,
               bool compress,
               std::size_t defaultChunkSize,
               const std::string& groupName,
               const std::string& datasetName,
               std::vector<std::size_t> dimensions,
               std::shared_ptr<T[]> data)
{
    std::vector<hsize_t> chunks;
    chunks.reserve(dimensions.size());
    for (auto dimension : dimensions)
    {
        chunks.push_back(static_cast<hsize_t>(dimension));
    }
    auto group = hdf5util::getGroup(file, groupName, true);
    saveArray(file, compress, defaultChunkSize, group, datasetName, std::move(dimensions), std::move(chunks), std::move(data));
}

template<typename T>
ChannelOptional<T> loadChannel(const std::shared_ptr<HighFive::File>& file,
                               HighFive::Group& group,
                               const std::string& datasetName)
{
    ChannelOptional<T> result;
    if (!file || !file->isValid() || !group.exist(datasetName))
    {
        return result;
    }

    auto dataset = group.getDataSet(datasetName);
    const auto dimensions = dataset.getSpace().getDimensions();
    const std::size_t elementCount = std::accumulate(dimensions.begin(), dimensions.end(), std::size_t{1}, std::multiplies<std::size_t>());
    if (elementCount == 0 || dimensions.size() < 2)
    {
        return result;
    }

    result = Channel<T>(dimensions[0], dimensions[1]);
    dataset.read(result->dataPtr().get());
    return result;
}

template<typename T>
ChannelOptional<T> loadChannel(const std::shared_ptr<HighFive::File>& file,
                               const std::string& groupName,
                               const std::string& datasetName)
{
    if (!hdf5util::exist(file, groupName))
    {
        return ChannelOptional<T>{};
    }
    auto group = hdf5util::getGroup(file, groupName, false);
    return loadChannel<T>(file, group, datasetName);
}

template<typename T>
void saveChannel(const std::shared_ptr<HighFive::File>& file,
                 bool compress,
                 std::size_t defaultChunkSize,
                 HighFive::Group& group,
                 const std::string& datasetName,
                 const Channel<T>& channel)
{
    std::vector<std::size_t> dimensions = {channel.numElements(), channel.width()};
    std::vector<hsize_t> chunks = {static_cast<hsize_t>(channel.numElements()), static_cast<hsize_t>(channel.width())};
    saveArray(file, compress, defaultChunkSize, group, datasetName, std::move(dimensions), std::move(chunks), channel.dataPtr());
}

template<typename T>
void saveChannel(const std::shared_ptr<HighFive::File>& file,
                 bool compress,
                 std::size_t defaultChunkSize,
                 const std::string& groupName,
                 const std::string& datasetName,
                 const Channel<T>& channel)
{
    auto group = hdf5util::getGroup(file, groupName, true);
    saveChannel(file, compress, defaultChunkSize, group, datasetName, channel);
}

template<typename Derived, typename VariantT, int R>
requires (R == 0)
void saveVariantChannel(const std::shared_ptr<HighFive::File>& file,
                        bool compress,
                        std::size_t defaultChunkSize,
                        HighFive::Group& group,
                        const std::string& name,
                        const VariantT& channel)
{
    if (R == channel.type())
    {
        saveChannel(file, compress, defaultChunkSize, group, name, channel.template extract<typename VariantT::template type_of_index<R>>());
    }
}

template<typename Derived, typename VariantT, int R>
requires (R != 0)
void saveVariantChannel(const std::shared_ptr<HighFive::File>& file,
                        bool compress,
                        std::size_t defaultChunkSize,
                        HighFive::Group& group,
                        const std::string& name,
                        const VariantT& channel)
{
    if (R == channel.type())
    {
        saveChannel(file, compress, defaultChunkSize, group, name, channel.template extract<typename VariantT::template type_of_index<R>>());
    }
    else
    {
        saveVariantChannel<Derived, VariantT, R - 1>(file, compress, defaultChunkSize, group, name, channel);
    }
}

template<typename VariantT, int R>
requires (R == 0)
std::optional<VariantT> loadVariantChannel(const std::shared_ptr<HighFive::File>& file,
                                             const HighFive::DataType& type,
                                             HighFive::Group& group,
                                             const std::string& name)
{
    using ValueType = typename VariantT::template type_of_index<R>;
    if (type == HighFive::AtomicType<ValueType>())
    {
        auto channel = loadChannel<ValueType>(file, group, name);
        if (channel)
        {
            return VariantT(*channel);
        }
    }
    return std::nullopt;
}

template<typename VariantT, int R>
requires (R != 0)
std::optional<VariantT> loadVariantChannel(const std::shared_ptr<HighFive::File>& file,
                                             const HighFive::DataType& type,
                                             HighFive::Group& group,
                                             const std::string& name)
{
    using ValueType = typename VariantT::template type_of_index<R>;
    if constexpr (hdf5util::H5AllowedTypes::contains<ValueType>())
    {
        if (type == HighFive::AtomicType<ValueType>())
        {
            auto channel = loadChannel<ValueType>(file, group, name);
            if (channel)
            {
                return VariantT(*channel);
            }
            return std::nullopt;
        }
    }
    return loadVariantChannel<VariantT, R - 1>(file, type, group, name);
}

std::string meshAttributeGroup(const std::string& meshName, const std::string& group)
{
    return (std::filesystem::path(meshName) / group).string();
}

template<typename T>
std::optional<AttributeChannel<T>> loadAttributeChannel(const std::shared_ptr<HighFive::File>& file,
                                                          const std::string& group,
                                                          const std::string& name)
{
    if (!file || !file->isValid() || !hdf5util::exist(file, group))
    {
        return std::nullopt;
    }
    auto hdf5Group = hdf5util::getGroup(file, group, false);
    return loadChannel<T>(file, hdf5Group, name);
}

template<typename T>
bool saveAttributeChannel(const std::shared_ptr<HighFive::File>& file,
                          bool compress,
                          std::size_t defaultChunkSize,
                          const std::string& group,
                          const std::string& name,
                          const AttributeChannel<T>& channel)
{
    auto hdf5Group = hdf5util::getGroup(file, group, true);
    saveChannel(file, compress, defaultChunkSize, hdf5Group, name, channel);
    return true;
}

} // namespace

KernelMeshStore::KernelMeshStore(lvr2::FileKernelPtr kernel, lvr2::MeshSchemaPtr schema)
    : kernel_(std::move(kernel))
    , schema_(std::move(schema))
{
    if (!kernel_ || !schema_)
    {
        throw std::invalid_argument("KernelMeshStore requires a kernel and schema");
    }
}

void KernelMeshStore::save_mesh(const std::string& meshName, const lvr2::MeshBufferPtr& mesh) const
{
    if (!mesh)
    {
        throw std::invalid_argument("mesh buffer must not be null");
    }

    {
        auto desc = schema_->mesh(meshName);
        YAML::Node node;
        node["n_materials"] = static_cast<std::uint64_t>(mesh->getMaterials().size());
        node["n_textures"] = static_cast<std::uint64_t>(mesh->getTextures().size());
        node["n_faces"] = static_cast<std::uint64_t>(mesh->numFaces());
        kernel_->saveMetaYAML(*desc.metaRoot, *desc.meta, node);
    }

    saveVertices(kernel_, schema_, meshName, mesh);
    saveFaceIndices(kernel_, schema_, meshName, mesh);
    saveFaceColors(kernel_, schema_, meshName, mesh);
    saveFaceNormals(kernel_, schema_, meshName, mesh);
    saveFaceMaterialIndices(kernel_, schema_, meshName, mesh);
    saveClusters(kernel_, schema_, meshName, mesh);
    saveMaterials(kernel_, schema_, meshName, mesh);
}

lvr2::MeshBufferPtr KernelMeshStore::load_mesh(const std::string& meshName) const
{
    auto desc = schema_->mesh(meshName);
    if (!kernel_->exists(*desc.dataRoot))
    {
        return nullptr;
    }

    auto mesh = std::make_shared<lvr2::MeshBuffer>();
    loadVertices(kernel_, schema_, meshName, mesh);
    loadFaces(kernel_, schema_, meshName, mesh);
    loadClusters(kernel_, schema_, meshName, mesh);
    loadMaterials(kernel_, schema_, meshName, mesh);
    return mesh;
}

std::vector<std::string> KernelMeshStore::available_meshes() const
{
    std::vector<std::string> meshes;
    kernel_->subGroupNames("meshes", meshes);
    return meshes;
}

Hdf5MeshStore::Hdf5MeshStore() = default;

Hdf5MeshStore::Hdf5MeshStore(const std::string& filename)
{
    open(filename);
}

void Hdf5MeshStore::open(const std::string& filename)
{
    filename_ = filename;
    file_ = hdf5util::open(filename);
    if (!file_ || !file_->isValid())
    {
        throw std::runtime_error("HDF5 mesh store file is not valid");
    }
}

bool Hdf5MeshStore::is_open() const
{
    return file_ && file_->isValid();
}

void Hdf5MeshStore::set_mesh_name(std::string meshName)
{
    meshName_ = std::move(meshName);
}

bool Hdf5MeshStore::is_mesh(HighFive::Group& group) const
{
    std::string id(kDirectId);
    std::string legacyId("MeshIO");
    std::string objectClass(kDirectClass);
    return (hdf5util::checkAttribute(group, "IO", id)
            || hdf5util::checkAttribute(group, "IO", legacyId))
        && hdf5util::checkAttribute(group, "CLASS", objectClass);
}

void Hdf5MeshStore::ensure_mesh_group()
{
    if (meshName_.empty())
    {
        throw std::runtime_error("mesh name must be set before writing mesh geometry");
    }

    auto group = hdf5util::getGroup(file_, meshName_, true);
    std::string id(kDirectId);
    std::string objectClass(kDirectClass);
    hdf5util::setAttribute(group, "IO", id);
    hdf5util::setAttribute(group, "CLASS", objectClass);
}

void Hdf5MeshStore::save_mesh(const std::string& name, const lvr2::MeshBufferPtr& buffer)
{
    auto group = hdf5util::getGroup(file_, name, true);
    set_mesh_name(name);
    save_mesh(group, buffer);
}

void Hdf5MeshStore::save_mesh(HighFive::Group& group, const lvr2::MeshBufferPtr& buffer)
{
    if (!buffer)
    {
        throw std::invalid_argument("mesh buffer must not be null");
    }

    std::string id(kDirectId);
    std::string objectClass(kDirectClass);
    hdf5util::setAttribute(group, "IO", id);
    hdf5util::setAttribute(group, "CLASS", objectClass);

    auto geometry = hdf5util::getGroup(group, kDirectGeometryGroup, true);
    for (const auto& entry : *buffer)
    {
        saveVariantChannel<Hdf5MeshStore, lvr2::MeshBuffer::val_type, lvr2::MeshBuffer::num_types - 1>(
            file_, compress_, chunkSize_, geometry, entry.first, entry.second);
    }

    if (!buffer->getTextures().empty())
    {
        auto textures = hdf5util::getGroup(group, kDirectTexturesGroup, true);
        for (const auto& texture : buffer->getTextures())
        {
            const std::vector<std::size_t> dimensions{
                static_cast<std::size_t>(texture.m_width),
                static_cast<std::size_t>(texture.m_height),
                static_cast<std::size_t>(texture.m_numChannels)};
            std::vector<hsize_t> chunks{dimensions[0], dimensions[1], dimensions[2]};
            const std::size_t byteCount = dimensions[0] * dimensions[1] * dimensions[2];
            std::shared_ptr<unsigned char[]> copy(new unsigned char[byteCount]);
            std::memcpy(copy.get(), texture.m_data, byteCount);
            saveArray(file_, compress_, chunkSize_, textures, std::to_string(texture.m_index), dimensions, chunks, copy);
        }
    }

    if (!buffer->getMaterials().empty())
    {
        auto materials = hdf5util::getGroup(group, kDirectMaterialsGroup, true);
        const std::size_t count = buffer->getMaterials().size();
        std::shared_ptr<int[]> textureHandles(new int[count]);
        std::shared_ptr<int16_t[]> colors(new int16_t[count * 3]);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& material = buffer->getMaterials()[i];
            textureHandles[i] = material.m_texture ? static_cast<int>(material.m_texture->idx()) : -1;
            if (material.m_color)
            {
                colors[3 * i + 0] = static_cast<int16_t>((*material.m_color)[0]);
                colors[3 * i + 1] = static_cast<int16_t>((*material.m_color)[1]);
                colors[3 * i + 2] = static_cast<int16_t>((*material.m_color)[2]);
            }
            else
            {
                colors[3 * i + 0] = -1;
                colors[3 * i + 1] = -1;
                colors[3 * i + 2] = -1;
            }
        }

        saveArray(file_, compress_, chunkSize_, materials, "texture_handles", {count, 1}, {static_cast<hsize_t>(count), 1}, textureHandles);
        saveArray(file_, compress_, chunkSize_, materials, "rgb_color", {count, 3}, {static_cast<hsize_t>(count), 3}, colors);
    }
}

lvr2::MeshBufferPtr Hdf5MeshStore::load_mesh(const std::string& name)
{
    set_mesh_name(name);
    if (!hdf5util::exist(file_, name))
    {
        return nullptr;
    }
    auto group = hdf5util::getGroup(file_, name, false);
    return load_mesh(group);
}

lvr2::MeshBufferPtr Hdf5MeshStore::load_mesh(HighFive::Group& group)
{
    if (!is_mesh(group))
    {
        return nullptr;
    }

    lvr2::MeshBufferPtr result;
    if (group.exist(kDirectGeometryGroup))
    {
        auto geometry = group.getGroup(kDirectGeometryGroup);
        for (const auto& name : geometry.listObjectNames())
        {
            std::unique_ptr<HighFive::DataSet> dataset;
            try
            {
                dataset.reset(new HighFive::DataSet(geometry.getDataSet(name)));
            }
            catch (const HighFive::DataSetException&)
            {
            }

            if (dataset)
            {
                auto channel = loadVariantChannel<lvr2::MeshBuffer::val_type, lvr2::MeshBuffer::num_types - 1>(
                    file_, dataset->getDataType(), geometry, name);
                if (channel)
                {
                    if (!result)
                    {
                        result = std::make_shared<lvr2::MeshBuffer>();
                    }
                    result->insert({name, *channel});
                }
            }
        }
    }

    if (!result)
    {
        return nullptr;
    }

    if (group.exist(kDirectTexturesGroup))
    {
        auto texturesGroup = group.getGroup(kDirectTexturesGroup);
        std::vector<lvr2::Texture> textures;
        for (const auto& name : texturesGroup.listObjectNames())
        {
            std::vector<std::size_t> dimensions;
            auto data = loadArray<unsigned char>(file_, texturesGroup, name, dimensions);
            if (data && dimensions.size() == 3)
            {
                textures.emplace_back(std::stoi(name),
                                      dimensions[0],
                                      dimensions[1],
                                      dimensions[2],
                                      1,
                                      1.0,
                                      data.get());
            }
        }
        result->setTextures(textures);
    }

    if (group.exist(kDirectMaterialsGroup))
    {
        auto materialsGroup = group.getGroup(kDirectMaterialsGroup);
        std::vector<lvr2::Material> materials;
        std::vector<std::size_t> colorDimensions;
        std::vector<std::size_t> textureDimensions;
        auto materialColor = loadArray<int16_t>(file_, materialsGroup, "rgb_color", colorDimensions);
        auto materialTexture = loadArray<int>(file_, materialsGroup, "texture_handles", textureDimensions);

        if (materialColor && materialTexture && colorDimensions.size() >= 2 && textureDimensions.size() >= 1)
        {
            const std::size_t count = textureDimensions[0];
            if (colorDimensions[0] == count && colorDimensions[1] == 3)
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    lvr2::Material material;
                    if (materialColor[3 * i] != -1)
                    {
                        material.m_color = std::optional<lvr2::RGB8Color>({
                            static_cast<std::uint8_t>(materialColor[3 * i + 0]),
                            static_cast<std::uint8_t>(materialColor[3 * i + 1]),
                            static_cast<std::uint8_t>(materialColor[3 * i + 2])});
                    }
                    if (materialTexture[i] != -1)
                    {
                        material.m_texture = std::optional<lvr2::TextureHandle>(materialTexture[i]);
                    }
                    materials.push_back(material);
                }
                result->setMaterials(materials);
            }
        }
    }

    return result;
}

FloatChannelOptional Hdf5MeshStore::getVertices()
{
    return loadChannel<float>(file_, (std::filesystem::path(meshName_) / kDirectGeometryGroup).string(), "vertices");
}

IndexChannelOptional Hdf5MeshStore::getIndices()
{
    return loadChannel<unsigned int>(file_, (std::filesystem::path(meshName_) / kDirectGeometryGroup).string(), "face_indices");
}

bool Hdf5MeshStore::addVertices(const FloatChannel& channel)
{
    ensure_mesh_group();
    saveChannel(file_, compress_, chunkSize_, (std::filesystem::path(meshName_) / kDirectGeometryGroup).string(), "vertices", channel);
    return true;
}

bool Hdf5MeshStore::addIndices(const IndexChannel& channel)
{
    ensure_mesh_group();
    saveChannel(file_, compress_, chunkSize_, (std::filesystem::path(meshName_) / kDirectGeometryGroup).string(), "face_indices", channel);
    return true;
}

bool Hdf5MeshStore::getChannel(const std::string group, const std::string name, FloatChannelOptional& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    channel = loadAttributeChannel<float>(file_, meshAttributeGroup(meshName_, group), name);
    return static_cast<bool>(channel);
}

bool Hdf5MeshStore::getChannel(const std::string group, const std::string name, IndexChannelOptional& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    channel = loadAttributeChannel<unsigned int>(file_, meshAttributeGroup(meshName_, group), name);
    return static_cast<bool>(channel);
}

bool Hdf5MeshStore::getChannel(const std::string group, const std::string name, UCharChannelOptional& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    channel = loadAttributeChannel<unsigned char>(file_, meshAttributeGroup(meshName_, group), name);
    return static_cast<bool>(channel);
}

bool Hdf5MeshStore::addChannel(const std::string group, const std::string name, const FloatChannel& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    return saveAttributeChannel(file_, compress_, chunkSize_, meshAttributeGroup(meshName_, group), name, channel);
}

bool Hdf5MeshStore::addChannel(const std::string group, const std::string name, const IndexChannel& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    return saveAttributeChannel(file_, compress_, chunkSize_, meshAttributeGroup(meshName_, group), name, channel);
}

bool Hdf5MeshStore::addChannel(const std::string group, const std::string name, const UCharChannel& channel)
{
    if (meshName_.empty())
    {
        return false;
    }
    return saveAttributeChannel(file_, compress_, chunkSize_, meshAttributeGroup(meshName_, group), name, channel);
}

} // namespace lvr2::io::mesh
