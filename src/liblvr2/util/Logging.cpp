#include <lvr2/util/Logging.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdio>
#include <utility>

// This file is compiled with -fvisibility=hidden to prevent spdlog symbols
// to be public in the dynamic library
#ifdef LVR2_BUILDING_SHARED
    #define LVR2_API __attribute__ ((visibility ("default")))
#else
    #define LVR2_API
#endif

namespace lvr2
{

LVR2_API Logger::Logger()
{
    m_logger = spdlog::stdout_color_mt("lvr2logger");
    m_logger->set_pattern("[%H:%M:%S:%e]%^[%-7l]%$ %v");
    m_level = LogLevel::info;
}

LVR2_API void Logger::print()
{
    spdlog::level::level_enum level;
    
    switch(m_level)
    {
        case LogLevel::trace: level = spdlog::level::trace; break;
        case LogLevel::debug: level = spdlog::level::debug; break;
        case LogLevel::info: level = spdlog::level::info; break;
        case LogLevel::warning: level = spdlog::level::warn; break;
        case LogLevel::error: level = spdlog::level::err; break;

    }

    m_logger->log(level, m_buffer.str());
    m_buffer.str("");
    m_buffer.clear();
}

LVR2_API void Logger::flush()
{
    m_logger->flush();
}

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
