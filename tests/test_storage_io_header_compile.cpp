#include <lvr2/io/scan.hpp>
#include <lvr2/io/storage.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <type_traits>

int main()
{
    using namespace lvr2::io;

    static_assert(std::is_same<storage::Result<int>, tl::expected<int, storage::Error>>::value,
                  "storage result must use the approved expected backing");
    static_assert(std::is_same<storage::Status, tl::expected<void, storage::Error>>::value,
                  "storage status must use the approved expected backing");
    static_assert(!std::is_copy_constructible<storage::StorageContext>::value,
                  "storage context must keep unique backend ownership");
    static_assert(std::is_move_constructible<storage::StorageContext>::value,
                  "storage context must be movable");
    static_assert(!std::is_copy_constructible<scan::ProjectStore>::value,
                  "project store must keep unique opened storage ownership");
    static_assert(std::is_move_constructible<scan::ProjectStore>::value,
                  "project store must be movable");

    return 0;
}
