#include "lvr2/io/mesh.hpp"

#include <type_traits>

static_assert(std::is_same<lvr2::io::mesh::Result<int>, tl::expected<int, lvr2::io::mesh::Error>>::value,
              "lvr2::io::mesh::Result<T> must be backed by tl::expected<T, Error>");
static_assert(std::is_same<lvr2::io::mesh::Status, tl::expected<void, lvr2::io::mesh::Error>>::value,
              "lvr2::io::mesh::Status must be backed by tl::expected<void, Error>");
static_assert(lvr2::io::mesh::Format::Auto != lvr2::io::mesh::Format::Obj,
              "Format enum values must be distinct");

int main()
{
    lvr2::io::mesh::Result<int> value = 7;
    if(!value || *value != 7)
    {
        return 1;
    }

    lvr2::io::mesh::Status ok;
    if(!ok)
    {
        return 2;
    }

    lvr2::io::mesh::Error error{lvr2::io::mesh::ErrorCode::UnsupportedFormat,
                            "unsupported",
                            "mesh.glb",
                            lvr2::io::mesh::Format::Glb};
    lvr2::io::mesh::Result<int> failed = lvr2::io::mesh::unexpected(error);
    if(failed || failed.error().code != lvr2::io::mesh::ErrorCode::UnsupportedFormat)
    {
        return 3;
    }

    return 0;
}
