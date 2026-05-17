#include <lvr2/util/Logging.hpp>

#include <cstdint>
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

template<typename... Args>
using info_expression = decltype(
    lvr2::log::info(std::declval<std::string_view>(), std::declval<Args>()...));

template<typename... Args>
using warning_expression = decltype(
    lvr2::log::warning(std::declval<std::string_view>(), std::declval<Args>()...));

template<typename... Args>
using error_expression = decltype(
    lvr2::log::error(std::declval<std::string_view>(), std::declval<Args>()...));

int main()
{
    static_assert(std::is_same<lvr2::LogLevel, lvr2::log::Level>::value,
                  "Monitor log level compatibility aliases the format facade level");
    static_assert(sizeof(lvr2::log::Level) == sizeof(std::uint8_t),
                  "log level stays compact");
    static_assert(std::is_same<decltype(lvr2::log::Level::warning), lvr2::log::Level>::value,
                  "warning level is exposed through lvr2::log");
    static_assert(std::is_same<info_expression<int>, void>::value,
                  "format-style info calls compile for the C++17 facade");
    static_assert(std::is_same<warning_expression<const char*>, void>::value,
                  "format-style warning calls compile for the C++17 facade");
    static_assert(std::is_same<error_expression<int, const char*>, void>::value,
                  "format-style error calls compile for the C++17 facade");
    static_assert(std::is_same<decltype(lvr2::log::info_runtime("runtime message")), void>::value,
                  "runtime message API remains explicit");

    lvr2::log::info("Loaded {} vertices", 12);
    lvr2::log::warning("Skipping '{}'", "channel");
    lvr2::log::error("Failed with code {}", 7);
    LVR2_LOG_INFO("Source-location bridge keeps {} formatting", "spdlog");
    lvr2::log::info_runtime("preformatted runtime message");
    lvr2::log::flush();
    return 0;
}
