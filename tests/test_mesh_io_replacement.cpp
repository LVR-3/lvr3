#include "lvr2/io/mesh.hpp"
#include "lvr2/types/MeshBuffer.hpp"
#include "support/PointFixtures.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace
{

std::filesystem::path uniquePath(const std::string& suffix)
{
    static std::atomic<unsigned long> counter{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("lvr2-mesh-io-replacement-" + std::to_string(now) + "-" +
            std::to_string(counter.fetch_add(1)) + suffix);
}

lvr2::MeshBufferPtr makeTriangleMesh()
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

void removeIfExists(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace

TEST(MeshIoReplacement, ObjFixtureLoadsThroughFacadeOnly)
{
    const auto path = uniquePath(".obj");
    ASSERT_TRUE(lvr2::testing::writeObjTriangleFixture(path));

    const auto result = lvr2::io::mesh::load(path);
    removeIfExists(path);

    ASSERT_TRUE(result) << result.error().message;
    ASSERT_TRUE(*result);
    EXPECT_EQ((*result)->numVertices(), 3u);
    EXPECT_EQ((*result)->numFaces(), 1u);
}

TEST(MeshIoReplacement, PlyRoundTripUsesFacadeOnly)
{
    const auto path = uniquePath(".ply");
    const auto saved = lvr2::io::mesh::save(makeTriangleMesh(), path);
    ASSERT_TRUE(saved) << saved.error().message;

    const auto loaded = lvr2::io::mesh::load(path);
    removeIfExists(path);

    ASSERT_TRUE(loaded) << loaded.error().message;
    ASSERT_TRUE(*loaded);
    EXPECT_EQ((*loaded)->numVertices(), 3u);
    EXPECT_EQ((*loaded)->numFaces(), 1u);
}

TEST(MeshIoReplacement, FormerLegacyWriterFormatsUsePrivateBackend)
{
    const auto mesh = makeTriangleMesh();

    const auto objPath = uniquePath(".obj");
    const auto objSaved = lvr2::io::mesh::save(mesh, objPath);
    removeIfExists(objPath);
    ASSERT_TRUE(objSaved) << objSaved.error().message;

    const auto stlPath = uniquePath(".stl");
    const auto stlSaved = lvr2::io::mesh::save(mesh, stlPath);
    removeIfExists(stlPath);
    ASSERT_TRUE(stlSaved) << stlSaved.error().message;
}
