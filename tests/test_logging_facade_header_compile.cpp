#include <lvr2/util/Logging.hpp>

#include <cstdint>
#include <filesystem>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <spdlog/spdlog.h>

namespace lvr2::log
{
void set_level(Level) {}
void flush() {}
void write(Level, std::string_view) {}
namespace detail
{
void* logger_handle()
{
    return spdlog::default_logger_raw();
}
} // namespace detail
} // namespace lvr2::log

int main()
{
    static_assert(std::is_same<lvr2::LogLevel, lvr2::log::Level>::value,
                  "Monitor log level compatibility aliases the format facade level");
    static_assert(sizeof(lvr2::log::Level) == sizeof(std::uint8_t),
                  "log level stays compact");
    static_assert(std::is_same<decltype(lvr2::log::Level::warning), lvr2::log::Level>::value,
                  "warning level is exposed through lvr2::log");
    static_assert(std::is_same<lvr2::log::SourceLocation, std::source_location>::value,
                  "source locations use the C++20 standard type");
    static_assert(std::is_same<decltype(lvr2::log::info("Loaded {} vertices", 12)), void>::value,
                  "std-format info calls compile for the C++20 baseline facade");
    static_assert(std::is_same<decltype(lvr2::log::warning("Skipping '{}'", "channel")), void>::value,
                  "std-format warning calls compile for the C++20 baseline facade");
    static_assert(std::is_same<decltype(lvr2::log::error("Failed with code {}", 7)), void>::value,
                  "std-format error calls compile for the C++20 baseline facade");
    static_assert(std::is_same<decltype(lvr2::log::here().info("Source-location logging {}", 1)), void>::value,
                  "source-location logging uses a call-style proxy");
    static_assert(std::is_same<decltype(lvr2::log::info("Path {}", std::filesystem::path{"path-like"})), void>::value,
                  "filesystem paths use explicit string summaries under std-format logging");
    static_assert(std::is_same<decltype(lvr2::log::info_runtime("runtime message")), void>::value,
                  "runtime message API remains explicit");
    static_assert(std::is_same<decltype(lvr2::log::info_runtime(std::declval<std::string_view>(), 1)), void>::value,
                  "runtime format strings use explicit runtime APIs");

    std::format_string<int> checked_format{"{}"};
    (void)checked_format;

    lvr2::log::info("Loaded {} vertices", 12);
    lvr2::log::warning("Skipping '{}'", "channel");
    lvr2::log::error("Failed with code {}", 7);
    lvr2::log::here().info("Source-location proxy keeps {} formatting", "std-format");
    lvr2::log::info("Path {}", std::filesystem::path{"path-like"});
    lvr2::log::info_runtime("preformatted runtime message");
    lvr2::log::info_runtime("runtime {}", "format");
    lvr2::log::flush();
    return 0;
}
