#include "lvr2/mesh/io.hpp"

#include <type_traits>

static_assert(std::is_same<lvr2::mesh::Result<int>, tl::expected<int, lvr2::mesh::Error>>::value,
              "lvr2::mesh::Result<T> must be backed by tl::expected<T, Error>");
static_assert(std::is_same<lvr2::mesh::Status, tl::expected<void, lvr2::mesh::Error>>::value,
              "lvr2::mesh::Status must be backed by tl::expected<void, Error>");
static_assert(lvr2::mesh::Format::Auto != lvr2::mesh::Format::Obj,
              "Format enum values must be distinct");

int main()
{
    lvr2::mesh::Result<int> value = 7;
    if(!value || *value != 7)
    {
        return 1;
    }

    lvr2::mesh::Status ok;
    if(!ok)
    {
        return 2;
    }

    lvr2::mesh::Error error{lvr2::mesh::ErrorCode::UnsupportedFormat,
                            "unsupported",
                            "mesh.glb",
                            lvr2::mesh::Format::Glb};
    lvr2::mesh::Result<int> failed = lvr2::mesh::unexpected(error);
    if(failed || failed.error().code != lvr2::mesh::ErrorCode::UnsupportedFormat)
    {
        return 3;
    }

    return 0;
}
