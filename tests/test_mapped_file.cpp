#include "lvr2/util/MappedFile.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace
{

bool check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main()
{
    static_assert(!std::is_copy_constructible_v<lvr2::util::MappedFile>);
    static_assert(std::is_move_constructible_v<lvr2::util::MappedFile>);

    const auto path = std::filesystem::temp_directory_path() / "lvr2-mapped-file-contract.bin";
    std::filesystem::remove(path);

    lvr2::util::MappedFile file;
    file.open_truncated(path, 4 * sizeof(float));

    if (!check(file.is_open(), "mapped file did not open")) return 1;
    if (!check(file.bytes().size() == 4 * sizeof(float), "mapped byte span has wrong size")) return 1;

    auto floats = file.span_as<float>();
    if (!check(floats.size() == 4, "typed span has wrong element count")) return 1;

    floats[0] = 1.0F;
    floats[1] = 2.0F;
    floats[2] = 3.0F;
    floats[3] = 4.0F;
    file.close();

    std::ifstream input(path, std::ios::binary);
    std::array<float, 4> values{};
    input.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(float)));
    if (!check(input.good(), "mapped file contents were not persisted")) return 1;
    if (!check(values[0] == 1.0F && values[3] == 4.0F, "mapped file values changed")) return 1;

    lvr2::util::MappedFile empty;
    empty.open_truncated(path, 0);
    if (!check(empty.is_open(), "zero-sized mapped file did not keep an open file handle")) return 1;
    if (!check(empty.bytes().empty(), "zero-sized mapped file must expose an empty byte span")) return 1;
    empty.close();

    std::filesystem::remove(path);
    return 0;
}
