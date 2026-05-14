#include "lvr2/io/AssimpMeshAdapter.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{

struct FormatCase
{
    lvr2::mesh::Format format;
    const char* suffix;
    bool binary;
};

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

std::filesystem::path smokePath(const std::string& stem, const char* suffix)
{
    return std::filesystem::temp_directory_path() / (stem + suffix);
}

void removeSidecars(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path() / (path.stem().string() + ".mtl"), ec);
    std::filesystem::remove(path.parent_path() / (path.stem().string() + ".bin"), ec);
    std::filesystem::remove(path.parent_path() / (path.filename().string() + ".bin"), ec);
}

bool expectTriangleLike(const lvr2::mesh::Result<lvr2::MeshBufferPtr>& loaded, const char* label)
{
    if(!loaded)
    {
        std::cerr << "load " << label << " failed: " << loaded.error().message << '\n';
        return false;
    }
    if(!*loaded || (*loaded)->numVertices() < 3 || (*loaded)->numFaces() < 1)
    {
        std::cerr << "load " << label << " returned insufficient geometry\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    const auto mesh = makeTriangleMesh();
    const FormatCase cases[] = {
        {lvr2::mesh::Format::Obj, ".obj", true},
        {lvr2::mesh::Format::Stl, ".stl", true},
        {lvr2::mesh::Format::Dae, ".dae", true},
        {lvr2::mesh::Format::Gltf, ".gltf", true},
        {lvr2::mesh::Format::Glb, ".glb", true},
    };

    bool ok = true;
    for(const auto& current : cases)
    {
        const auto path = smokePath("lvr2-assimp-adapter-smoke", current.suffix);
        const auto saved = lvr2::mesh::detail::saveWithPrivateMeshBackend(
            mesh, path, {current.format, current.binary});
        if(!saved)
        {
            std::cerr << "save " << current.suffix << " failed: " << saved.error().message << '\n';
            ok = false;
            continue;
        }

        ok = expectTriangleLike(
                 lvr2::mesh::detail::loadWithPrivateMeshBackend(path, current.format),
                 current.suffix) && ok;
        removeSidecars(path);
    }

    const auto malformed = smokePath("lvr2-assimp-adapter-malformed", ".stl");
    {
        std::ofstream out(malformed);
        out << "this is not a valid mesh file\n";
    }
    const auto loadedMalformed = lvr2::mesh::detail::loadWithPrivateMeshBackend(
        malformed, lvr2::mesh::Format::Stl);
    removeSidecars(malformed);
    if(loadedMalformed)
    {
        std::cerr << "malformed STL unexpectedly loaded\n";
        ok = false;
    }
    else if(loadedMalformed.error().code != lvr2::mesh::ErrorCode::ReadFailed &&
            loadedMalformed.error().code != lvr2::mesh::ErrorCode::MissingMesh)
    {
        std::cerr << "malformed STL returned unexpected error code\n";
        ok = false;
    }

    return ok ? 0 : 1;
}
