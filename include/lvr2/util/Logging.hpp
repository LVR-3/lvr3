#ifndef LVR2_UTIL_LOGGING_HPP
#define LVR2_UTIL_LOGGING_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef SPDLOG_USE_STD_FORMAT
#define SPDLOG_USE_STD_FORMAT
#endif
#include <spdlog/spdlog.h>

namespace lvr2
{

struct MonitorState;

namespace log
{

enum class Level : std::uint8_t
{
    trace,
    debug,
    info,
    warning,
    error
};

using SourceLocation = std::source_location;

void set_level(Level level);
void flush();
void write(Level level, std::string_view message);

namespace detail
{

void* logger_handle();

inline spdlog::logger& logger()
{
    return *static_cast<spdlog::logger*>(logger_handle());
}

inline spdlog::level::level_enum to_spdlog_level(Level level) noexcept
{
    switch(level)
    {
        case Level::trace: return spdlog::level::trace;
        case Level::debug: return spdlog::level::debug;
        case Level::info: return spdlog::level::info;
        case Level::warning: return spdlog::level::warn;
        case Level::error: return spdlog::level::err;
    }

    return spdlog::level::info;
}

inline spdlog::source_loc to_spdlog_source_location(SourceLocation location) noexcept
{
    return spdlog::source_loc{
        location.file_name() ? location.file_name() : "",
        static_cast<int>(location.line()),
        location.function_name() ? location.function_name() : ""};
}

inline spdlog::source_loc no_spdlog_source_location() noexcept
{
    return spdlog::source_loc{};
}

// Narrow bridge for path values already used by legacy call sites. Other domain
// objects need std::formatter support or an explicit summary at the call site.
template<typename T>
struct IsPathArgument : std::false_type
{
};

template<>
struct IsPathArgument<std::filesystem::path> : std::true_type
{
};

template<typename T>
concept PathArgument = IsPathArgument<std::remove_cvref_t<T>>::value;

template<PathArgument T>
std::string format_argument(const T& value)
{
    return value.string();
}

template<typename T>
requires(!PathArgument<T>)
decltype(auto) format_argument(T&& value)
{
    return std::forward<T>(value);
}

template<typename T>
using FormatArgument = decltype(format_argument(std::declval<T>()));

template<typename... Args>
void log_to_spdlog(spdlog::source_loc location,
                   Level level,
                   std::format_string<FormatArgument<Args>...> format,
                   Args&&... args)
{
#if defined(__cpp_lib_format) && __cpp_lib_format < 202207L
    logger().log(location, to_spdlog_level(level), format.get(), format_argument(std::forward<Args>(args))...);
#else
    logger().log(location, to_spdlog_level(level), format, format_argument(std::forward<Args>(args))...);
#endif
}

template<typename... Args>
void write_at(Level level, SourceLocation location, std::format_string<FormatArgument<Args>...> format, Args&&... args)
{
    log_to_spdlog(to_spdlog_source_location(location), level, format, std::forward<Args>(args)...);
}

template<typename... Args>
void write_without_location(Level level, std::format_string<FormatArgument<Args>...> format, Args&&... args)
{
    log_to_spdlog(no_spdlog_source_location(), level, format, std::forward<Args>(args)...);
}

template<typename... Args>
std::string format_runtime(std::string_view format, Args&&... args)
{
    auto normalized_args = std::make_tuple(format_argument(std::forward<Args>(args))...);
    return std::apply(
        [format](auto&... values) {
            return std::vformat(std::string(format), std::make_format_args(values...));
        },
        normalized_args);
}

template<typename... Args>
void write_runtime_at(Level level, SourceLocation location, std::string_view format, Args&&... args)
{
    logger().log(to_spdlog_source_location(location),
                 to_spdlog_level(level),
                 "{}",
                 format_runtime(format, std::forward<Args>(args)...));
}

template<typename... Args>
void write_runtime_without_location(Level level, std::string_view format, Args&&... args)
{
    logger().log(no_spdlog_source_location(),
                 to_spdlog_level(level),
                 "{}",
                 format_runtime(format, std::forward<Args>(args)...));
}

} // namespace detail

class SourceLogger
{
public:
    explicit SourceLogger(SourceLocation location = SourceLocation::current()) noexcept
    : m_location(location)
    {
    }

    template<typename... Args>
    void write(Level level, std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        detail::write_at(level, m_location, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void trace(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        write(Level::trace, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        write(Level::debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        write(Level::info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        write(Level::warning, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        warning(format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args) const
    {
        write(Level::error, format, std::forward<Args>(args)...);
    }

    void write_runtime(Level level, std::string_view message) const
    {
        detail::write_at(level, m_location, "{}", message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void write_runtime(Level level, std::string_view format, Args&&... args) const
    {
        detail::write_runtime_at(level, m_location, format, std::forward<Args>(args)...);
    }

    void trace_runtime(std::string_view message) const
    {
        write_runtime(Level::trace, message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void trace_runtime(std::string_view format, Args&&... args) const
    {
        write_runtime(Level::trace, format, std::forward<Args>(args)...);
    }

    void debug_runtime(std::string_view message) const
    {
        write_runtime(Level::debug, message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void debug_runtime(std::string_view format, Args&&... args) const
    {
        write_runtime(Level::debug, format, std::forward<Args>(args)...);
    }

    void info_runtime(std::string_view message) const
    {
        write_runtime(Level::info, message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void info_runtime(std::string_view format, Args&&... args) const
    {
        write_runtime(Level::info, format, std::forward<Args>(args)...);
    }

    void warning_runtime(std::string_view message) const
    {
        write_runtime(Level::warning, message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void warning_runtime(std::string_view format, Args&&... args) const
    {
        write_runtime(Level::warning, format, std::forward<Args>(args)...);
    }

    void warn_runtime(std::string_view message) const
    {
        warning_runtime(message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void warn_runtime(std::string_view format, Args&&... args) const
    {
        warning_runtime(format, std::forward<Args>(args)...);
    }

    void error_runtime(std::string_view message) const
    {
        write_runtime(Level::error, message);
    }

    template<typename... Args>
    requires(sizeof...(Args) > 0)
    void error_runtime(std::string_view format, Args&&... args) const
    {
        write_runtime(Level::error, format, std::forward<Args>(args)...);
    }

private:
    SourceLocation m_location;
};

inline SourceLogger here(SourceLocation location = SourceLocation::current()) noexcept
{
    return SourceLogger{location};
}

inline void write_runtime(Level level, std::string_view message)
{
    write(level, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void write_runtime(Level level, std::string_view format, Args&&... args)
{
    detail::write_runtime_without_location(level, format, std::forward<Args>(args)...);
}

template<typename... Args>
void write(Level level, std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    detail::write_without_location(level, format, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    write(Level::trace, format, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    write(Level::debug, format, std::forward<Args>(args)...);
}

template<typename... Args>
void info(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    write(Level::info, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warning(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    write(Level::warning, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    warning(format, std::forward<Args>(args)...);
}

template<typename... Args>
void error(std::format_string<detail::FormatArgument<Args>...> format, Args&&... args)
{
    write(Level::error, format, std::forward<Args>(args)...);
}

inline void trace_runtime(std::string_view message)
{
    write_runtime(Level::trace, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void trace_runtime(std::string_view format, Args&&... args)
{
    write_runtime(Level::trace, format, std::forward<Args>(args)...);
}

inline void debug_runtime(std::string_view message)
{
    write_runtime(Level::debug, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void debug_runtime(std::string_view format, Args&&... args)
{
    write_runtime(Level::debug, format, std::forward<Args>(args)...);
}

inline void info_runtime(std::string_view message)
{
    write_runtime(Level::info, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void info_runtime(std::string_view format, Args&&... args)
{
    write_runtime(Level::info, format, std::forward<Args>(args)...);
}

inline void warning_runtime(std::string_view message)
{
    write_runtime(Level::warning, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void warning_runtime(std::string_view format, Args&&... args)
{
    write_runtime(Level::warning, format, std::forward<Args>(args)...);
}

inline void warn_runtime(std::string_view message)
{
    warning_runtime(message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void warn_runtime(std::string_view format, Args&&... args)
{
    warning_runtime(format, std::forward<Args>(args)...);
}

inline void error_runtime(std::string_view message)
{
    write_runtime(Level::error, message);
}

template<typename... Args>
requires(sizeof...(Args) > 0)
void error_runtime(std::string_view format, Args&&... args)
{
    write_runtime(Level::error, format, std::forward<Args>(args)...);
}

} // namespace log

using LogLevel = log::Level;

static_assert(sizeof(log::Level) == sizeof(std::uint8_t),
              "logging level remains a compact public enum");
static_assert(static_cast<std::uint8_t>(log::Level::error) == 4,
              "logging level ordering is part of the sink mapping contract");
static_assert(std::is_copy_constructible<log::SourceLocation>::value,
              "source-location values remain cheap to copy into logging proxies");
static_assert(std::is_copy_constructible<log::SourceLogger>::value,
              "source-location logger proxy remains copyable for call-style logging");

/**
 * @brief A class to monitor progress.
 */
class Monitor
{
public:
    /**
     * @brief Constructs a new Monitor object.
     *
     * @param level     Log level associated with the progress output.
     * @param text      Prefix text for progress bar.
     * @param max       Number of expected iterations.
     * @param width     Width of the progress bar. Currently buggy,
     *                  leave it at default.
     */
    Monitor(const LogLevel& level, const std::string& text, const size_t& max, size_t width = 0);

    /// Increment progress by one.
    void operator++();

    /// Destructor.
    ~Monitor()
    {
        this->terminate();
    }

    /**
     * @brief Removes the progress bar from the terminal. Call this function once
     *        if the monitor object is still alive and log output should follow.
     */
    void terminate();

private:
    /// @brief Package-free monitor state.
    std::shared_ptr<MonitorState> m_monitor;

    /// @brief Prefix text.
    std::string m_prefixText;
};

} // namespace lvr2

#endif // LVR2_UTIL_LOGGING_HPP
