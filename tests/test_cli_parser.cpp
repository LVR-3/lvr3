#include "lvr2/config/CommandLine.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        ++failures;
    }
}

void exercise_defaults_aliases_positionals_and_vectors()
{
    using namespace lvr2::cli;

    bool help = false;
    int count = 7;
    std::string output = "default.out";
    std::vector<float> samples;

    options_description options("Options");
    options.add_options()
        ("help,h", bool_switch(&help), "Show help")
        ("count,c", value<int>(&count)->default_value(count), "Count")
        ("output,o", value<std::string>(&output)->default_value(output), "Output")
        ("samples", value<std::vector<float>>(&samples)->multitoken(), "Samples")
        ("input", value<std::vector<std::string>>(), "Input files");

    positional_options_description positional;
    positional.add("input", -1);

    const char* argv[] = {"scan_a.pts", "scan_b.pts", "--count", "11", "-o", "mesh.ply", "--samples", "-1.5", "0", "2.5"};
    variables_map variables;
    store(command_line_parser(std::span<char const* const>(argv, std::size(argv))).options(options).positional(positional).run(), variables);
    notify(variables);

    check(!help, "bool switch default should be false");
    check(count == 11, "long integer option should update target");
    check(output == "mesh.ply", "short alias should update target");
    check(variables.count("count") == 1, "explicit long option should be counted");
    check(variables.count("c") == 1, "explicit short alias should be counted");
    check(variables.count("output") == 1, "explicit short alias should count the canonical option");
    check(variables.count("help") == 0, "absent switch should not be counted");
    check(samples.size() == 3 && samples[0] == -1.5f && samples[2] == 2.5f, "multitoken values should accept negative numbers");
    const auto& input = variables["input"].as<std::vector<std::string>>();
    check(input.size() == 2 && input[0] == "scan_a.pts" && input[1] == "scan_b.pts", "positional input should collect remaining tokens");

    std::ostringstream help_text;
    options.print(help_text);
    const auto help_output = help_text.str();
    check(help_output.find("-o [ --output ] arg (=default.out)") != std::string::npos, "help should show aliases, value placeholders, and defaults");
    check(help_output.find("--samples arg...") != std::string::npos, "help should mark multitoken values");
}

void exercise_switches_implicit_values_and_required_errors()
{
    using namespace lvr2::cli;

    bool flag = false;
    std::string mode;

    options_description options("Options");
    options.add_options()
        ("flag,f", bool_switch(&flag), "Flag")
        ("mode,m", value<std::string>(&mode)->implicit_value("auto"), "Mode")
        ("required", value<std::string>()->required(), "Required");

    const char* argv[] = {"--flag", "--mode", "--required", "present"};
    variables_map variables;
    store(command_line_parser(std::span<char const* const>(argv, std::size(argv))).options(options).run(), variables);

    check(flag, "bool switch should become true when present");
    check(mode == "auto", "implicit value should be used when no value token follows");
    check(variables.count("f") == 1, "switch alias should be counted");
    check(variables["required"].as<std::string>() == "present", "required value should parse");

    bool threw = false;
    try
    {
        variables_map missing;
        const char* missing_argv[] = {"--flag"};
        store(command_line_parser(std::span<char const* const>(missing_argv, std::size(missing_argv))).options(options).run(), missing);
    }
    catch (const error&)
    {
        threw = true;
    }
    check(threw, "missing required value should throw");
}

void exercise_unknown_options_are_rejected()
{
    using namespace lvr2::cli;

    options_description options("Options");
    options.add_options()
        ("input", value<std::vector<std::string>>(), "Input files");

    positional_options_description positional;
    positional.add("input", -1);

    bool threw = false;
    try
    {
        variables_map variables;
        const char* argv[] = {"-Q"};
        store(command_line_parser(std::span<char const* const>(argv, std::size(argv))).options(options).positional(positional).run(), variables);
    }
    catch (const error&)
    {
        threw = true;
    }
    check(threw, "unknown single-dash options should be rejected instead of treated as positionals");
}

} // namespace

int main()
{
    exercise_defaults_aliases_positionals_and_vectors();
    exercise_switches_implicit_values_and_required_errors();
    exercise_unknown_options_are_rejected();
    return failures == 0 ? 0 : 1;
}
