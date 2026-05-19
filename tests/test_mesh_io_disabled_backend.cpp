#include "lvr2/io/mesh.hpp"
#include "lvr2/types/MeshBuffer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace
{

int expectUnsupported(const lvr2::io::mesh::Error& error, const char* operation)
{
    if(error.code != lvr2::io::mesh::ErrorCode::UnsupportedFormat)
    {
        std::cerr << operation << " returned unexpected error code\n";
        return 1;
    }
    if(error.message.find("shared build with private mesh asset I/O enabled") == std::string::npos)
    {
        std::cerr << operation << " returned unclear disabled-backend message: " << error.message << "\n";
        return 1;
    }
    return 0;
}

} // namespace

int main()
{
    const auto path = std::filesystem::temp_directory_path() / "lvr2-mesh-disabled-backend.obj";
    {
        std::ofstream out(path);
        out << "# disabled backend contract\n";
    }

    const auto loaded = lvr2::io::mesh::load(path, {lvr2::io::mesh::Format::Obj});
    std::filesystem::remove(path);
    if(loaded)
    {
        std::cerr << "disabled mesh backend unexpectedly loaded a mesh\n";
        return 1;
    }
    if(const int code = expectUnsupported(loaded.error(), "load"))
    {
        return code;
    }

    const auto mesh = std::make_shared<lvr2::MeshBuffer>(3, 1);
    const auto status = lvr2::io::mesh::save(mesh, path, {lvr2::io::mesh::Format::Obj});
    std::filesystem::remove(path);
    if(status)
    {
        std::cerr << "disabled mesh backend unexpectedly saved a mesh\n";
        return 1;
    }
    return expectUnsupported(status.error(), "save");
}
