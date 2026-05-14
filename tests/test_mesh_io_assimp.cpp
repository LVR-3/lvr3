#include "lvr2/mesh/io.hpp"
#include "support/MeshIoTestHelpers.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{

struct FormatCase
{
    lvr2::mesh::Format format;
    const char* suffix;
    bool binary;
};

void expectLoadsAsTriangleLikeMesh(const std::filesystem::path& path, lvr2::mesh::Format format)
{
    const auto loaded = lvr2::mesh::load(path, {format});
    ASSERT_TRUE(loaded) << loaded.error().message;
    ASSERT_TRUE(*loaded);
    EXPECT_GE((*loaded)->numVertices(), 3u);
    EXPECT_GE((*loaded)->numFaces(), 1u);
}

} // namespace

TEST(MeshIoAssimp, SavesAndLoadsEnabledFormatsThroughFacade)
{
    const FormatCase cases[] = {
        {lvr2::mesh::Format::Obj, ".obj", true},
        {lvr2::mesh::Format::Stl, ".stl", true},
        {lvr2::mesh::Format::Dae, ".dae", true},
        {lvr2::mesh::Format::Gltf, ".gltf", true},
        {lvr2::mesh::Format::Glb, ".glb", true},
    };

    for(const auto& current : cases)
    {
        const auto path = lvr2::testing::uniqueMeshIoPath("lvr2-assimp-roundtrip", current.suffix);
        const auto saved = lvr2::mesh::save(lvr2::testing::makeTriangleMesh(),
                                            path,
                                            {current.format, current.binary});
        ASSERT_TRUE(saved) << "format suffix " << current.suffix << ": " << saved.error().message;

        expectLoadsAsTriangleLikeMesh(path, current.format);
        lvr2::testing::removeMeshIoSidecars(path);
    }
}

TEST(MeshIoAssimp, LoadsAsciiStlTriangleFixture)
{
    const auto path = lvr2::testing::uniqueMeshIoPath("lvr2-assimp-ascii-stl", ".stl");
    ASSERT_TRUE(lvr2::testing::writeAsciiStlTriangleFixture(path));

    expectLoadsAsTriangleLikeMesh(path, lvr2::mesh::Format::Stl);
    lvr2::testing::removeMeshIoSidecars(path);
}

TEST(MeshIoAssimp, MalformedInputReturnsStructuredLvrError)
{
    const auto path = lvr2::testing::uniqueMeshIoPath("lvr2-assimp-malformed", ".stl");
    {
        std::ofstream out(path);
        ASSERT_TRUE(out.is_open());
        out << "this is not a valid mesh file\n";
    }

    const auto loaded = lvr2::mesh::load(path, {lvr2::mesh::Format::Stl});
    lvr2::testing::removeMeshIoSidecars(path);

    ASSERT_FALSE(loaded);
    EXPECT_TRUE(loaded.error().code == lvr2::mesh::ErrorCode::ReadFailed ||
                loaded.error().code == lvr2::mesh::ErrorCode::MissingMesh);
    EXPECT_EQ(loaded.error().format, lvr2::mesh::Format::Stl);
}

TEST(MeshIoAssimp, BinaryPlyRemainsLegacyFacadePath)
{
    const auto path = lvr2::testing::uniqueMeshIoPath("lvr2-legacy-ply", ".ply");
    const auto saved = lvr2::mesh::save(lvr2::testing::makeTriangleMesh(), path, {lvr2::mesh::Format::Ply, true});
    ASSERT_TRUE(saved) << saved.error().message;

    const auto loaded = lvr2::mesh::load(path, {lvr2::mesh::Format::Ply});
    lvr2::testing::removeMeshIoSidecars(path);

    ASSERT_TRUE(loaded) << loaded.error().message;
    ASSERT_TRUE(*loaded);
    EXPECT_EQ((*loaded)->numVertices(), 3u);
    EXPECT_EQ((*loaded)->numFaces(), 1u);
}
