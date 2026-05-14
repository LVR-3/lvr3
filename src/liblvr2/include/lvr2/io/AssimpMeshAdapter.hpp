#ifndef LVR2_IO_PRIVATE_MESH_ADAPTER_HPP
#define LVR2_IO_PRIVATE_MESH_ADAPTER_HPP

#include "lvr2/mesh/io.hpp"
#include "lvr2/types/MeshBuffer.hpp"

#include <filesystem>

namespace lvr2
{
namespace mesh
{
namespace detail
{

Result<MeshBufferPtr> loadWithPrivateMeshBackend(const std::filesystem::path& path, Format format);
Status saveWithPrivateMeshBackend(const MeshBufferPtr& mesh,
                                  const std::filesystem::path& path,
                                  const SaveOptions& options);

} // namespace detail
} // namespace mesh
} // namespace lvr2

#endif // LVR2_IO_PRIVATE_MESH_ADAPTER_HPP
