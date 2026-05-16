#include "lvr2/io/mesh.hpp"
#include "lvr2/types/MatrixTypes.hpp"

#include <type_traits>

static_assert(std::is_same<lvr2::io::mesh::Result<int>, tl::expected<int, lvr2::io::mesh::Error>>::value,
              "io::mesh::Result<T> must remain a tl::expected alias");
static_assert(lvr2::Matrix4fRM::IsRowMajor,
              "row-major matrix aliases must retain their storage contract");
static_assert(lvr2::Transformd::RowsAtCompileTime == 4 && lvr2::Transformd::ColsAtCompileTime == 4,
              "transform aliases must retain 4x4 dimensions");

int main()
{
    return 0;
}
