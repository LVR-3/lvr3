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
#include <type_traits>
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

static_assert(std::is_enum<Format>::value,
              "mesh::Format must remain a closed enum vocabulary");
static_assert(!std::is_convertible<Format, int>::value,
              "mesh::Format must not implicitly convert to integer values");
static_assert(std::is_enum<ErrorCode>::value,
              "mesh::ErrorCode must remain a closed enum vocabulary");
static_assert(!std::is_convertible<ErrorCode, int>::value,
              "mesh::ErrorCode must not implicitly convert to integer values");
static_assert(std::is_same<Result<MeshBufferPtr>, tl::expected<MeshBufferPtr, Error>>::value,
              "mesh::Result<T> must stay backed by tl::expected<T, Error>");
static_assert(std::is_same<Status, tl::expected<void, Error>>::value,
              "mesh::Status must stay backed by tl::expected<void, Error>");
static_assert(std::is_default_constructible<LoadOptions>::value,
              "mesh load options must remain default constructible");
static_assert(std::is_copy_constructible<LoadOptions>::value,
              "mesh load options must remain copyable");
static_assert(std::is_default_constructible<SaveOptions>::value,
              "mesh save options must remain default constructible");
static_assert(std::is_copy_constructible<SaveOptions>::value,
              "mesh save options must remain copyable");
static_assert([] { return LoadOptions{}.format == Format::Auto; }(),
              "mesh load options must default to suffix-based format detection");
static_assert([] { return SaveOptions{}.format == Format::Auto && SaveOptions{}.binary; }(),
              "mesh save options must default to suffix detection and binary output");

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
