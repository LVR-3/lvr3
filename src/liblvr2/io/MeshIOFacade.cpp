#include "lvr2/mesh/io.hpp"

#include "lvr2/io/modelio/ObjIO.hpp"
#include "lvr2/io/modelio/PLYIO.hpp"
#include "lvr2/types/MeshBuffer.hpp"
#include "lvr2/types/Model.hpp"

#if defined(LVR2_MESH_IO_HAS_ASSIMP)
#include "lvr2/io/AssimpMeshAdapter.hpp"
#endif

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <string>

namespace lvr2
{
namespace mesh
{
namespace
{

std::string lowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

Format formatFromSuffix(const std::filesystem::path& path)
{
    const auto extension = lowerCopy(path.extension().string());
    if(extension == ".obj") return Format::Obj;
    if(extension == ".ply") return Format::Ply;
    if(extension == ".stl") return Format::Stl;
    if(extension == ".dae" || extension == ".collada") return Format::Dae;
    if(extension == ".gltf") return Format::Gltf;
    if(extension == ".glb") return Format::Glb;
    return Format::Auto;
}

Format resolveFormat(const std::filesystem::path& path, Format requested)
{
    if(requested != Format::Auto)
    {
        return requested;
    }
    return formatFromSuffix(path);
}

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

bool fileExists(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

bool nonEmptyFileExists(const std::filesystem::path& path)
{
    std::error_code ec;
    if(!std::filesystem::exists(path, ec) || ec)
    {
        return false;
    }
    const auto size = std::filesystem::file_size(path, ec);
    return !ec && size > 0;
}

bool meshHasGeometry(const MeshBufferPtr& mesh)
{
    return mesh && mesh->numVertices() > 0 && mesh->numFaces() > 0;
}

bool hasExplicitFormatSuffixMismatch(const std::filesystem::path& path, Format requested)
{
    if(requested == Format::Auto)
    {
        return false;
    }
    const Format suffixFormat = formatFromSuffix(path);
    return suffixFormat != Format::Auto && suffixFormat != requested;
}

bool isPrivateBackendLoadFormat(Format format)
{
    switch(format)
    {
        case Format::Stl:
        case Format::Dae:
        case Format::Gltf:
        case Format::Glb:
            return true;
        case Format::Auto:
        case Format::Obj:
        case Format::Ply:
            return false;
    }
    return false;
}

bool isPrivateBackendSaveFormat(Format format)
{
    switch(format)
    {
        case Format::Obj:
        case Format::Stl:
        case Format::Dae:
        case Format::Gltf:
        case Format::Glb:
            return true;
        case Format::Auto:
        case Format::Ply:
            return false;
    }
    return false;
}

Result<MeshBufferPtr> readWithLegacyIo(const std::filesystem::path& path, Format format)
{
    ModelPtr model;
    const auto filename = path.string();

    switch(format)
    {
        case Format::Obj:
        {
            ObjIO io;
            model = io.read(filename);
            break;
        }
        case Format::Ply:
        {
            PLYIO io;
            model = io.read(filename);
            break;
        }
        default:
            return makeUnexpected(ErrorCode::UnsupportedFormat,
                                  std::string("Loading format '") + formatName(format) + "' is not supported by the mesh facade yet.",
                                  path,
                                  format);
    }

    if(!model)
    {
        return makeUnexpected(ErrorCode::ReadFailed,
                              "Legacy reader did not return a model.",
                              path,
                              format);
    }
    if(!meshHasGeometry(model->m_mesh))
    {
        return makeUnexpected(ErrorCode::MissingMesh,
                              "The file did not contain mesh vertices and faces.",
                              path,
                              format);
    }

    return model->m_mesh;
}

Status saveWithLegacyIo(const MeshBufferPtr& mesh, const std::filesystem::path& path, Format format)
{
    ModelPtr model(new Model(mesh));
    const auto filename = path.string();

    switch(format)
    {
        case Format::Ply:
        {
            PLYIO io;
            io.save(model, filename);
            break;
        }
        default:
            return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                        std::string("Saving format '") + formatName(format) + "' is not supported by the mesh facade yet.",
                                        path,
                                        format);
    }

    if(!nonEmptyFileExists(path))
    {
        return makeStatusUnexpected(ErrorCode::WriteFailed,
                                    "Legacy writer did not create a non-empty output file.",
                                    path,
                                    format);
    }

    return {};
}

} // namespace

Result<MeshBufferPtr> load(const std::filesystem::path& path, const LoadOptions& options)
{
    const Format format = resolveFormat(path, options.format);

    if(path.empty())
    {
        return makeUnexpected(ErrorCode::EmptyPath, "Mesh load path is empty.", path, format);
    }
    if(format == Format::Auto)
    {
        return makeUnexpected(ErrorCode::UnsupportedFormat,
                              "Could not infer a supported mesh format from the path suffix.",
                              path,
                              format);
    }
    if(hasExplicitFormatSuffixMismatch(path, options.format))
    {
        return makeUnexpected(ErrorCode::UnsupportedFormat,
                              std::string("Requested mesh format '") + formatName(options.format) + "' conflicts with the path suffix.",
                              path,
                              format);
    }
    if(format != Format::Obj && format != Format::Ply && !isPrivateBackendLoadFormat(format))
    {
        return makeUnexpected(ErrorCode::UnsupportedFormat,
                              std::string("Loading format '") + formatName(format) + "' is not supported by the mesh facade yet.",
                              path,
                              format);
    }
    if(!fileExists(path))
    {
        return makeUnexpected(ErrorCode::FileNotFound, "Mesh input file does not exist.", path, format);
    }

    try
    {
        if(format == Format::Obj || format == Format::Ply)
        {
            return readWithLegacyIo(path, format);
        }
#if defined(LVR2_MESH_IO_HAS_ASSIMP)
        return detail::loadWithPrivateMeshBackend(path, format);
#else
        return makeUnexpected(ErrorCode::UnsupportedFormat,
                              std::string("Loading format '") + formatName(format) + "' requires the optional private mesh backend.",
                              path,
                              format);
#endif
    }
    catch(const std::exception& e)
    {
        return makeUnexpected(ErrorCode::ReadFailed,
                              std::string("Mesh reader failed: ") + e.what(),
                              path,
                              format);
    }
    catch(...)
    {
        return makeUnexpected(ErrorCode::ReadFailed, "Mesh reader failed with an unknown exception.", path, format);
    }
}

Status save(const MeshBufferPtr& mesh, const std::filesystem::path& path, const SaveOptions& options)
{
    const Format format = resolveFormat(path, options.format);

    if(path.empty())
    {
        return makeStatusUnexpected(ErrorCode::EmptyPath, "Mesh save path is empty.", path, format);
    }
    if(format == Format::Auto)
    {
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    "Could not infer a supported mesh format from the path suffix.",
                                    path,
                                    format);
    }
    if(hasExplicitFormatSuffixMismatch(path, options.format))
    {
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    std::string("Requested mesh format '") + formatName(options.format) + "' conflicts with the path suffix.",
                                    path,
                                    format);
    }
    if(!meshHasGeometry(mesh))
    {
        return makeStatusUnexpected(ErrorCode::MissingMesh,
                                    "A mesh with at least one vertex and one face is required for saving.",
                                    path,
                                    format);
    }
    if(format == Format::Ply && !options.binary)
    {
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    "Text/ASCII PLY saving is not supported by the mesh facade yet.",
                                    path,
                                    format);
    }
    if(format != Format::Ply && !isPrivateBackendSaveFormat(format))
    {
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    std::string("Saving format '") + formatName(format) + "' is not supported by the mesh facade yet.",
                                    path,
                                    format);
    }

    try
    {
        if(format == Format::Ply)
        {
            return saveWithLegacyIo(mesh, path, format);
        }
#if defined(LVR2_MESH_IO_HAS_ASSIMP)
        SaveOptions resolvedOptions = options;
        resolvedOptions.format = format;
        return detail::saveWithPrivateMeshBackend(mesh, path, resolvedOptions);
#else
        return makeStatusUnexpected(ErrorCode::UnsupportedFormat,
                                    std::string("Saving format '") + formatName(format) + "' requires the optional private mesh backend.",
                                    path,
                                    format);
#endif
    }
    catch(const std::exception& e)
    {
        return makeStatusUnexpected(ErrorCode::WriteFailed,
                                    std::string("Mesh writer failed: ") + e.what(),
                                    path,
                                    format);
    }
    catch(...)
    {
        return makeStatusUnexpected(ErrorCode::WriteFailed, "Mesh writer failed with an unknown exception.", path, format);
    }
}

} // namespace mesh
} // namespace lvr2
