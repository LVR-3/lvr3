#include "lvr2/mesh/io.hpp"
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
           ("lvr2-mesh-io-" + std::to_string(now) + "-" +
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

TEST(MeshIoFacade, LoadReportsEmptyPath)
{
    const auto result = lvr2::mesh::load({});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, lvr2::mesh::ErrorCode::EmptyPath);
    EXPECT_EQ(result.error().format, lvr2::mesh::Format::Auto);
}

TEST(MeshIoFacade, LoadReportsMissingFileAfterFormatResolution)
{
    const auto path = uniquePath(".obj");
    const auto result = lvr2::mesh::load(path);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, lvr2::mesh::ErrorCode::FileNotFound);
    EXPECT_EQ(result.error().format, lvr2::mesh::Format::Obj);
}

TEST(MeshIoFacade, LoadReportsUnsupportedExplicitFormat)
{
    const auto path = uniquePath(".obj");
    {
        std::ofstream out(path);
        ASSERT_TRUE(out.is_open());
        out << "# placeholder\n";
    }

    const auto result = lvr2::mesh::load(path, {lvr2::mesh::Format::Gltf});
    removeIfExists(path);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, lvr2::mesh::ErrorCode::UnsupportedFormat);
    EXPECT_EQ(result.error().format, lvr2::mesh::Format::Gltf);
}

TEST(MeshIoFacade, ObjTriangleFixtureLoadsMesh)
{
    const auto path = uniquePath(".obj");
    ASSERT_TRUE(lvr2::testing::writeObjTriangleFixture(path));

    const auto result = lvr2::mesh::load(path);
    removeIfExists(path);

    ASSERT_TRUE(result) << result.error().message;
    ASSERT_TRUE(*result);
    EXPECT_EQ((*result)->numVertices(), 3u);
    EXPECT_EQ((*result)->numFaces(), 1u);
}

TEST(MeshIoFacade, SaveRejectsNullMesh)
{
    const auto path = uniquePath(".ply");
    const auto status = lvr2::mesh::save(nullptr, path);
    ASSERT_FALSE(status);
    EXPECT_EQ(status.error().code, lvr2::mesh::ErrorCode::MissingMesh);
    EXPECT_EQ(status.error().format, lvr2::mesh::Format::Ply);
}

TEST(MeshIoFacade, SaveRejectsTextModeRatherThanIgnoringBinaryOption)
{
    const auto path = uniquePath(".ply");
    const auto status = lvr2::mesh::save(makeTriangleMesh(), path, {lvr2::mesh::Format::Ply, false});
    removeIfExists(path);

    ASSERT_FALSE(status);
    EXPECT_EQ(status.error().code, lvr2::mesh::ErrorCode::UnsupportedFormat);
    EXPECT_EQ(status.error().format, lvr2::mesh::Format::Ply);
}

TEST(MeshIoFacade, SaveRejectsObjUntilSafeWriterIsAvailable)
{
    const auto path = uniquePath(".obj");
    const auto status = lvr2::mesh::save(makeTriangleMesh(), path);
    removeIfExists(path);

#if defined(LVR2_MESH_IO_TEST_HAS_ASSIMP)
    ASSERT_TRUE(status) << status.error().message;
#else
    ASSERT_FALSE(status);
    EXPECT_EQ(status.error().code, lvr2::mesh::ErrorCode::UnsupportedFormat);
    EXPECT_EQ(status.error().format, lvr2::mesh::Format::Obj);
#endif
}

TEST(MeshIoFacade, SaveRejectsStlUntilSafeWriterIsTestBacked)
{
    const auto path = uniquePath(".stl");
    const auto status = lvr2::mesh::save(makeTriangleMesh(), path);
    removeIfExists(path);

#if defined(LVR2_MESH_IO_TEST_HAS_ASSIMP)
    ASSERT_TRUE(status) << status.error().message;
#else
    ASSERT_FALSE(status);
    EXPECT_EQ(status.error().code, lvr2::mesh::ErrorCode::UnsupportedFormat);
    EXPECT_EQ(status.error().format, lvr2::mesh::Format::Stl);
#endif
}

TEST(MeshIoFacade, PlyRoundTripPreservesTriangleCounts)
{
    const auto path = uniquePath(".ply");
    const auto saveStatus = lvr2::mesh::save(makeTriangleMesh(), path);
    ASSERT_TRUE(saveStatus) << saveStatus.error().message;

    const auto loaded = lvr2::mesh::load(path);
    removeIfExists(path);

    ASSERT_TRUE(loaded) << loaded.error().message;
    ASSERT_TRUE(*loaded);
    EXPECT_EQ((*loaded)->numVertices(), 3u);
    EXPECT_EQ((*loaded)->numFaces(), 1u);
}
