/**
 * @file io.hpp
 * @brief Public mesh I/O facade for LVR-owned load/save APIs.
 */

#ifndef LVR2_MESH_IO_HPP
#define LVR2_MESH_IO_HPP

#include <tl/expected.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace lvr2
{

class MeshBuffer;
using MeshBufferPtr = std::shared_ptr<MeshBuffer>;

namespace mesh
{

enum class Format
{
    Auto,
    Obj,
    Ply,
    Stl,
    Dae,
    Gltf,
    Glb
};

struct LoadOptions
{
    Format format = Format::Auto;
};

struct SaveOptions
{
    Format format = Format::Auto;
    bool binary = true;
};

enum class ErrorCode
{
    None,
    InvalidArgument,
    EmptyPath,
    FileNotFound,
    AlreadyExists,
    UnsupportedFormat,
    MissingMesh,
    ReadFailed,
    WriteFailed
};

struct Error
{
    ErrorCode code = ErrorCode::None;
    std::string message;
    std::filesystem::path path;
    Format format = Format::Auto;
};

template<class T>
using Result = tl::expected<T, Error>;

using Status = tl::expected<void, Error>;

inline tl::unexpected<Error> unexpected(Error error)
{
    return tl::unexpected<Error>(std::move(error));
}

Result<lvr2::MeshBufferPtr> load(const std::filesystem::path& path,
                                  const LoadOptions& options = {});

Status save(const lvr2::MeshBufferPtr& mesh,
            const std::filesystem::path& path,
            const SaveOptions& options = {});

} // namespace mesh
} // namespace lvr2

#endif // LVR2_MESH_IO_HPP
