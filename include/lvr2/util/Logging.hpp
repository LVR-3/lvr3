#ifndef LVR2_UTIL_LOGGING_HPP
#define LVR2_UTIL_LOGGING_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>

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

void set_level(Level level);
void flush();
void write(Level level, std::string_view message);

inline void write_runtime(Level level, std::string_view message)
{
    write(level, message);
}

template<typename... Args>
void write(Level level, fmt::format_string<Args...> format, Args&&... args)
{
    write(level, fmt::format(format, std::forward<Args>(args)...));
}

template<typename... Args>
void trace(fmt::format_string<Args...> format, Args&&... args)
{
    write(Level::trace, format, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(fmt::format_string<Args...> format, Args&&... args)
{
    write(Level::debug, format, std::forward<Args>(args)...);
}

template<typename... Args>
void info(fmt::format_string<Args...> format, Args&&... args)
{
    write(Level::info, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warning(fmt::format_string<Args...> format, Args&&... args)
{
    write(Level::warning, format, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(fmt::format_string<Args...> format, Args&&... args)
{
    warning(format, std::forward<Args>(args)...);
}

template<typename... Args>
void error(fmt::format_string<Args...> format, Args&&... args)
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

using LogLevel = log::Level;

static_assert(sizeof(log::Level) == sizeof(std::uint8_t),
              "logging level remains a compact public enum");
static_assert(static_cast<std::uint8_t>(log::Level::error) == 4,
              "logging level ordering is part of the sink mapping contract");

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
