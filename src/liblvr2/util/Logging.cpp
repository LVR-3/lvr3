#include <lvr2/util/Logging.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <utility>

// This file is compiled with -fvisibility=hidden to prevent spdlog symbols
// from becoming public in the dynamic library.
#ifdef LVR2_BUILDING_SHARED
    #define LVR2_API __attribute__ ((visibility ("default")))
#else
    #define LVR2_API
#endif

namespace lvr2
{

namespace
{

std::shared_ptr<spdlog::logger> logger_instance()
{
    static std::shared_ptr<spdlog::logger> logger = [] {
        if(auto existing = spdlog::get("lvr2logger"))
        {
            return existing;
        }

        auto created = spdlog::stdout_color_mt("lvr2logger");
        created->set_pattern("[%H:%M:%S:%e]%^[%-7l]%$ %v");
        created->set_level(spdlog::level::info);
        return created;
    }();

    return logger;
}

} // namespace

namespace log
{

namespace detail
{

LVR2_API void* logger_handle()
{
    return logger_instance().get();
}

} // namespace detail

LVR2_API void set_level(Level level)
{
    logger_instance()->set_level(detail::to_spdlog_level(level));
}

LVR2_API void flush()
{
    logger_instance()->flush();
}

LVR2_API void write(Level level, std::string_view message)
{
    logger_instance()->log(detail::to_spdlog_level(level), "{}", message);
}

} // namespace log

struct MonitorState
{
    MonitorState(LogLevel level, std::string text, size_t max, size_t width)
    : level(level)
    , text(std::move(text))
    , max(max)
    , width(width)
    {
    }

    void render()
    {
        if(terminated)
        {
            return;
        }

        const auto percent = max == 0 ? 100 : static_cast<unsigned>((current * 100) / max);
        (void)level;
        (void)width;
        std::fprintf(stderr, "\r%s %zu/%zu (%u%%)", text.c_str(), current, max, percent);
        std::fflush(stderr);
    }

    void finish()
    {
        if(!terminated)
        {
            render();
            std::fprintf(stderr, "\n");
            std::fflush(stderr);
            terminated = true;
        }
    }

    LogLevel level;
    std::string text;
    size_t max = 0;
    size_t width = 0;
    size_t current = 0;
    bool terminated = false;
};

LVR2_API Monitor::Monitor(const LogLevel& level, const std::string& text, const size_t& max, size_t width)
: m_monitor(std::make_shared<MonitorState>(level, text, max, width))
, m_prefixText(text)
{
    m_monitor->render();
}

LVR2_API void Monitor::terminate()
{
    if(m_monitor)
    {
        m_monitor->finish();
    }
}

LVR2_API void Monitor::operator++()
{
    if(m_monitor)
    {
        ++m_monitor->current;
        m_monitor->render();
    }
}

} // namespace lvr2
