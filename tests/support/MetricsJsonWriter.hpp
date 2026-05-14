#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <vector>

namespace lvr2
{
namespace testing
{

// Test support only: this helper intentionally lives under tests/support and is
// not a public CLI or installed-library metrics API.
struct MetricEntry
{
    std::string name;
    double value;
};

struct MetricsReport
{
    std::string scenario;
    std::vector<MetricEntry> entries;
};

inline std::string escapeJson(std::string_view input)
{
    std::string output;
    output.reserve(input.size());
    for (char ch : input)
    {
        const auto c = static_cast<unsigned char>(ch);
        switch (c)
        {
            case '\\':
                output += "\\\\";
                break;
            case '"':
                output += "\\\"";
                break;
            case '/':
                output += "\\/";
                break;
            case '\t':
                output += "\\t";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            default:
                if (c < 0x20)
                {
                    const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                               '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                    output += "\\u00";
                    output.push_back(hexDigits[c >> 4]);
                    output.push_back(hexDigits[c & 0x0F]);
                }
                else
                {
                    output += static_cast<char>(c);
                }
                break;
        }
    }
    return output;
}

inline bool writeMetricsJson(std::ostream& out, const MetricsReport& report)
{
    if (!out.good())
    {
        return false;
    }

    out << "{\n";
    out << "  \"scenario\": \"" << escapeJson(report.scenario) << "\"," << '\n';
    out << "  \"metric_count\": " << report.entries.size() << ",\n";
    out << "  \"metrics\": [\n";

    for (std::size_t i = 0; i < report.entries.size(); ++i)
    {
        const auto& entry = report.entries[i];
        out << "    {\"name\": \"" << escapeJson(entry.name) << "\", \"value\": "
            << std::setprecision(10) << entry.value << "}";
        if (i + 1 != report.entries.size())
        {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n";
    out << "}\n";
    return static_cast<bool>(out);
}

inline bool writeMetricsJson(const MetricsReport& report, const std::filesystem::path& path)
{
    std::ofstream out(path);
    if (!out.is_open())
    {
        return false;
    }

    return writeMetricsJson(out, report);
}

} // namespace testing
} // namespace lvr2
