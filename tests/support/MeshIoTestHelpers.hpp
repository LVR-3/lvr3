#ifndef LVR2_TESTS_SUPPORT_MESH_IO_TEST_HELPERS_HPP
#define LVR2_TESTS_SUPPORT_MESH_IO_TEST_HELPERS_HPP

#include "lvr2/types/MeshBuffer.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace lvr2
{
namespace testing
{

inline std::filesystem::path uniqueMeshIoPath(const std::string& prefix, const std::string& suffix)
{
    static std::atomic<unsigned long> counter{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (prefix + "-" + std::to_string(now) + "-" +
            std::to_string(counter.fetch_add(1)) + suffix);
}

inline lvr2::MeshBufferPtr makeTriangleMesh()
{
    lvr2::floatArr vertices(new float[9]);
    vertices[0] = 0.0f; vertices[1] = 0.0f; vertices[2] = 0.0f;
    vertices[3] = 1.0f; vertices[4] = 0.0f; vertices[5] = 0.0f;
    vertices[6] = 0.0f; vertices[7] = 1.0f; vertices[8] = 0.0f;

    lvr2::indexArray faces(new unsigned int[3]);
    faces[0] = 0; faces[1] = 1; faces[2] = 2;

    auto mesh = std::make_shared<lvr2::MeshBuffer>();
    mesh->setVertices(vertices, 3);
    mesh->setFaceIndices(faces, 1);
    return mesh;
}

inline void removeMeshIoSidecars(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);

    std::vector<std::filesystem::path> sidecars;
    sidecars.push_back(path.parent_path() / (path.stem().string() + ".mtl"));
    sidecars.push_back(path.parent_path() / (path.stem().string() + ".bin"));
    sidecars.push_back(path.parent_path() / (path.filename().string() + ".bin"));
    for(const auto& sidecar : sidecars)
    {
        std::filesystem::remove(sidecar, ec);
    }
}

inline bool writeAsciiStlTriangleFixture(const std::filesystem::path& path)
{
    std::ofstream out(path);
    if(!out.is_open())
    {
        return false;
    }
    out << "solid triangle\n"
        << "  facet normal 0 0 1\n"
        << "    outer loop\n"
        << "      vertex 0 0 0\n"
        << "      vertex 1 0 0\n"
        << "      vertex 0 1 0\n"
        << "    endloop\n"
        << "  endfacet\n"
        << "endsolid triangle\n";
    return out.good();
}

} // namespace testing
} // namespace lvr2

#endif // LVR2_TESTS_SUPPORT_MESH_IO_TEST_HELPERS_HPP
