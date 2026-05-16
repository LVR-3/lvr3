#include <lvr2/io/mesh.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{

constexpr std::size_t kMaxInputBytes = 1U << 20;
static_assert(kMaxInputBytes > 0, "fuzz input cap must be non-zero");

std::string extensionFor(std::string_view input)
{
    if(input.rfind("ply", 0) == 0) return ".ply";
    if(input.rfind("solid", 0) == 0) return ".stl";
    if(input.rfind("{", 0) == 0) return ".gltf";
    if(input.rfind("<", 0) == 0) return ".dae";
    if(input.size() >= 4 && input.substr(0, 4) == "glTF") return ".glb";
    return ".obj";
}

std::filesystem::path writeInputFile(const std::uint8_t* data, std::size_t size)
{
    static std::atomic<unsigned long long> counter{0};
    const auto cappedSize = std::min(size, kMaxInputBytes);
    const std::string_view view(reinterpret_cast<const char*>(data), cappedSize);

    std::error_code ec;
    const auto directory = std::filesystem::temp_directory_path(ec) / "lvr2_mesh_io_fuzz";
    std::filesystem::create_directories(directory, ec);

    const auto id = counter.fetch_add(1, std::memory_order_relaxed);
    const auto path = directory / ("input_" + std::to_string(id) + extensionFor(view));

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if(output && cappedSize > 0)
    {
        output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(cappedSize));
    }
    return path;
}

void runOneInput(const std::uint8_t* data, std::size_t size)
{
    if(data == nullptr && size != 0)
    {
        return;
    }

    const auto path = writeInputFile(data, size);
    lvr2::io::mesh::LoadOptions options;
    options.format = lvr2::io::mesh::Format::Auto;
    (void)lvr2::io::mesh::load(path.string(), options);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

std::vector<std::uint8_t> readFile(const char* path)
{
    std::ifstream input(path, std::ios::binary);
    if(!input)
    {
        throw std::runtime_error(std::string("could not open fuzz seed: ") + path);
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    runOneInput(data, size);
    return 0;
}

#if defined(LVR2_STANDALONE_FUZZ_DRIVER)
int main(int argc, char** argv)
{
    try
    {
        if(argc <= 1)
        {
            const std::uint8_t empty = 0;
            runOneInput(&empty, 0);
            return 0;
        }

        for(int i = 1; i < argc; ++i)
        {
            const auto data = readFile(argv[i]);
            LLVMFuzzerTestOneInput(data.data(), data.size());
        }
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 2;
    }
}
#endif
