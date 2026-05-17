#ifndef LVR2_UTIL_LOGGING_HPP
#define LVR2_UTIL_LOGGING_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/format.h>
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

struct SourceLocation
{
    const char* file = "";
    int line = 0;
    const char* function = "";
};

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
        location.file ? location.file : "",
        location.line,
        location.function ? location.function : ""};
}

inline fmt::string_view to_fmt_string_view(std::string_view value) noexcept
{
    return fmt::string_view(value.data(), value.size());
}

template<typename... Args>
void write_at(Level level, SourceLocation location, std::string_view format, Args&&... args)
{
    logger().log(to_spdlog_source_location(location),
                 to_spdlog_level(level),
                 fmt::runtime(to_fmt_string_view(format)),
                 std::forward<Args>(args)...);
}

template<typename... Args>
void trace_at(SourceLocation location, std::string_view format, Args&&... args)
{
    write_at(Level::trace, location, format, std::forward<Args>(args)...);
}

template<typename... Args>
void debug_at(SourceLocation location, std::string_view format, Args&&... args)
{
    write_at(Level::debug, location, format, std::forward<Args>(args)...);
}

template<typename... Args>
void info_at(SourceLocation location, std::string_view format, Args&&... args)
{
    write_at(Level::info, location, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warning_at(SourceLocation location, std::string_view format, Args&&... args)
{
    write_at(Level::warning, location, format, std::forward<Args>(args)...);
}

template<typename... Args>
void error_at(SourceLocation location, std::string_view format, Args&&... args)
{
    write_at(Level::error, location, format, std::forward<Args>(args)...);
}

} // namespace detail

inline void write_runtime(Level level, std::string_view message)
{
    write(level, message);
}

template<typename... Args>
void write(Level level, std::string_view format, Args&&... args)
{
    detail::write_at(level, SourceLocation{}, format, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(std::string_view format, Args&&... args)
{
    write(Level::trace, format, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(std::string_view format, Args&&... args)
{
    write(Level::debug, format, std::forward<Args>(args)...);
}

template<typename... Args>
void info(std::string_view format, Args&&... args)
{
    write(Level::info, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warning(std::string_view format, Args&&... args)
{
    write(Level::warning, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(std::string_view format, Args&&... args)
{
    warning(format, std::forward<Args>(args)...);
}

template<typename... Args>
void error(std::string_view format, Args&&... args)
{
    write(Level::error, format, std::forward<Args>(args)...);
}

inline void trace_runtime(std::string_view message)
{
    write_runtime(Level::trace, message);
}

inline void debug_runtime(std::string_view message)
{
    write_runtime(Level::debug, message);
}

inline void info_runtime(std::string_view message)
{
    write_runtime(Level::info, message);
}

inline void warning_runtime(std::string_view message)
{
    write_runtime(Level::warning, message);
}

inline void warn_runtime(std::string_view message)
{
    warning_runtime(message);
}

inline void error_runtime(std::string_view message)
{
    write_runtime(Level::error, message);
}

} // namespace log

#ifndef LVR2_LOG_TRACE
#define LVR2_LOG_TRACE(...) \
    ::lvr2::log::detail::trace_at(::lvr2::log::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#endif
#ifndef LVR2_LOG_DEBUG
#define LVR2_LOG_DEBUG(...) \
    ::lvr2::log::detail::debug_at(::lvr2::log::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#endif
#ifndef LVR2_LOG_INFO
#define LVR2_LOG_INFO(...) \
    ::lvr2::log::detail::info_at(::lvr2::log::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#endif
#ifndef LVR2_LOG_WARNING
#define LVR2_LOG_WARNING(...) \
    ::lvr2::log::detail::warning_at(::lvr2::log::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#endif
#ifndef LVR2_LOG_WARN
#define LVR2_LOG_WARN(...) LVR2_LOG_WARNING(__VA_ARGS__)
#endif
#ifndef LVR2_LOG_ERROR
#define LVR2_LOG_ERROR(...) \
    ::lvr2::log::detail::error_at(::lvr2::log::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#endif

using LogLevel = log::Level;

static_assert(sizeof(log::Level) == sizeof(std::uint8_t),
              "logging level remains a compact public enum");
static_assert(static_cast<std::uint8_t>(log::Level::error) == 4,
              "logging level ordering is part of the sink mapping contract");
static_assert(std::is_trivially_copyable<log::SourceLocation>::value,
              "source-location bridge remains cheap to pass by value");

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
