#include "lvr2/io/AssimpMeshAdapter.hpp"

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace lvr2
{
namespace mesh
{
namespace detail
{
namespace
{

static_assert(std::is_same<lvr2::indexArray::element_type, unsigned int>::value,
              "MeshBuffer face indices must stay compatible with the backend index type");

const char* formatName(Format format)
{
    switch(format)
    {
        case Format::Auto: return "auto";
        case Format::Obj: return "obj";
        case Format::Ply: return "ply";
        case Format::Stl: return "stl";
        case Format::Dae: return "dae";
        case Format::Gltf: return "gltf";
        case Format::Glb: return "glb";
    }
    return "unknown";
}

Error makeError(ErrorCode code,
                std::string message,
                const std::filesystem::path& path,
                Format format)
{
    return Error{code, std::move(message), path, format};
}

Result<MeshBufferPtr> makeUnexpected(ErrorCode code,
                                     std::string message,
                                     const std::filesystem::path& path,
                                     Format format)
{
    return unexpected(makeError(code, std::move(message), path, format));
}

Status makeStatusUnexpected(ErrorCode code,
                            std::string message,
                            const std::filesystem::path& path,
                            Format format)
{
    return unexpected(makeError(code, std::move(message), path, format));
}

unsigned int importFlags()
{
    return aiProcess_Triangulate |
           aiProcess_JoinIdenticalVertices |
           aiProcess_SortByPType |
           aiProcess_ValidateDataStructure |
           aiProcess_PreTransformVertices;
}

const char* exportIdFor(Format format, bool binary)
{
    switch(format)
    {
        case Format::Obj: return "obj";
        case Format::Stl: return binary ? "stlb" : "stl";
        case Format::Dae: return "collada";
        case Format::Gltf: return "gltf2";
        case Format::Glb: return "glb2";
        case Format::Ply:
        case Format::Auto:
            return nullptr;
    }
    return nullptr;
}

std::string backendMessage(const char* operation, const char* detail)
{
    std::ostringstream out;
    out << "Private mesh backend " << operation << " failed";
    if(detail && detail[0] != '\0')
    {
        out << ": " << detail;
    }
    out << '.';
    return out.str();
}

Result<MeshBufferPtr> sceneToMeshBuffer(const aiScene& scene,
                                        const std::filesystem::path& path,
                                        Format format)
{
    if(!scene.HasMeshes())
    {
        return makeUnexpected(ErrorCode::MissingMesh,
                              "The file did not contain mesh vertices and faces.",
                              path,
                              format);
    }

    std::size_t vertexCount = 0;
    std::size_t faceCount = 0;
    for(unsigned int meshIndex = 0; meshIndex < scene.mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = scene.mMeshes[meshIndex];
        if(!mesh || !mesh->HasPositions())
        {
            continue;
        }
        vertexCount += mesh->mNumVertices;
        for(unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            if(mesh->mFaces[faceIndex].mNumIndices == 3)
            {
                ++faceCount;
            }
        }
    }

    if(vertexCount == 0 || faceCount == 0)
    {
        return makeUnexpected(ErrorCode::MissingMesh,
                              "The file did not contain triangle mesh geometry.",
                              path,
                              format);
    }
    if(vertexCount > static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
    {
        return makeUnexpected(ErrorCode::ReadFailed,
                              "The mesh is too large for the current MeshBuffer index type.",
                              path,
                              format);
    }

    lvr2::floatArr vertices(new float[vertexCount * 3]);
    lvr2::indexArray faces(new unsigned int[faceCount * 3]);

    std::size_t vertexOffset = 0;
    std::size_t vertexCursor = 0;
    std::size_t faceCursor = 0;
    for(unsigned int meshIndex = 0; meshIndex < scene.mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = scene.mMeshes[meshIndex];
        if(!mesh || !mesh->HasPositions())
        {
            continue;
        }

        for(unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            vertices[vertexCursor++] = static_cast<float>(mesh->mVertices[i].x);
            vertices[vertexCursor++] = static_cast<float>(mesh->mVertices[i].y);
            vertices[vertexCursor++] = static_cast<float>(mesh->mVertices[i].z);
        }

        for(unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            if(face.mNumIndices != 3)
            {
                continue;
            }
            faces[faceCursor++] = static_cast<unsigned int>(vertexOffset + face.mIndices[0]);
            faces[faceCursor++] = static_cast<unsigned int>(vertexOffset + face.mIndices[1]);
            faces[faceCursor++] = static_cast<unsigned int>(vertexOffset + face.mIndices[2]);
        }

        vertexOffset += mesh->mNumVertices;
    }

    auto meshBuffer = std::make_shared<lvr2::MeshBuffer>();
    meshBuffer->setVertices(vertices, vertexCount);
    meshBuffer->setFaceIndices(faces, faceCount);
    return meshBuffer;
}

std::unique_ptr<aiScene> meshBufferToScene(const MeshBufferPtr& mesh,
                                           const std::filesystem::path& path,
                                           Format format,
                                           Error* error)
{
    const std::size_t vertexCount = mesh->numVertices();
    const std::size_t faceCount = mesh->numFaces();
    lvr2::floatArr vertices = mesh->getVertices();
    lvr2::indexArray faces = mesh->getFaceIndices();

    if(!vertices || !faces || vertexCount == 0 || faceCount == 0)
    {
        *error = makeError(ErrorCode::MissingMesh,
                           "A mesh with vertices and triangle faces is required for saving.",
                           path,
                           format);
        return nullptr;
    }
    if(vertexCount > static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
    {
        *error = makeError(ErrorCode::WriteFailed,
                           "The mesh is too large for the private mesh backend index type.",
                           path,
                           format);
        return nullptr;
    }

    auto scene = std::make_unique<aiScene>();
    scene->mRootNode = new aiNode("root");
    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh*[1];
    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial*[1];
    scene->mMaterials[0] = new aiMaterial();

    aiMesh* outMesh = new aiMesh();
    scene->mMeshes[0] = outMesh;
    outMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    outMesh->mMaterialIndex = 0;
    outMesh->mNumVertices = static_cast<unsigned int>(vertexCount);
    outMesh->mVertices = new aiVector3D[vertexCount];
    for(std::size_t i = 0; i < vertexCount; ++i)
    {
        outMesh->mVertices[i] = aiVector3D(vertices[3 * i + 0], vertices[3 * i + 1], vertices[3 * i + 2]);
    }

    outMesh->mNumFaces = static_cast<unsigned int>(faceCount);
    outMesh->mFaces = new aiFace[faceCount];
    for(std::size_t i = 0; i < faceCount; ++i)
    {
        aiFace& face = outMesh->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];
        face.mIndices[0] = faces[3 * i + 0];
        face.mIndices[1] = faces[3 * i + 1];
        face.mIndices[2] = faces[3 * i + 2];
    }

    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[1];
    scene->mRootNode->mMeshes[0] = 0;

    return scene;
}

} // namespace

Result<MeshBufferPtr> loadWithPrivateMeshBackend(const std::filesystem::path& path, Format format)
{
    if(format == Format::Ply || format == Format::Auto)
    {
        return makeUnexpected(ErrorCode::UnsupportedFormat,
                              std::string("Loading format '") + formatName(format) + "' is not routed to the private mesh backend.",
                              path,
                              format);
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.string(), importFlags());
    if(!scene)
    {
        return makeUnexpected(ErrorCode::ReadFailed,
                              backendMessage("load", importer.GetErrorString()),
                              path,
                              format);
    }

    return sceneToMeshBuffer(*scene, path, format);
}

Status saveWithPrivateMeshBackend(const MeshBufferPtr& mesh,
                                  const std::filesystem::path& path,
                                  const SaveOptions& options)
{
    const char* exportId = exportIdFor(options.format, options.binary);
    if(!exportId)
    {
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    std::string("Saving format '") + formatName(options.format) + "' is not routed to the private mesh backend.",
                                    path,
                                    options.format);
    }

    Error sceneError;
    std::unique_ptr<aiScene> scene = meshBufferToScene(mesh, path, options.format, &sceneError);
    if(!scene)
    {
        return unexpected(sceneError);
    }

    Assimp::Exporter exporter;
    const aiReturn result = exporter.Export(scene.get(), exportId, path.string());
    if(result != AI_SUCCESS)
    {
        return makeStatusUnexpected(ErrorCode::WriteFailed,
                                    backendMessage("save", exporter.GetErrorString()),
                                    path,
                                    options.format);
    }

    return {};
}

} // namespace detail
} // namespace mesh
} // namespace lvr2
