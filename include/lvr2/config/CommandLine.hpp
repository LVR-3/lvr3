#pragma once

#include <algorithm>
#include <any>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace lvr2::cli
{

class error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class variable_value
{
public:
    variable_value() = default;

    template<typename T>
    explicit variable_value(T value)
        : m_value(std::move(value))
    {}

    template<typename T>
    const T& as() const
    {
        if (!m_value.has_value())
        {
            throw error("option has no value");
        }
        return std::any_cast<const T&>(m_value);
    }

    bool empty() const
    {
        return !m_value.has_value();
    }

private:
    std::any m_value;
};

class variables_map
{
public:
    // Returns explicit command-line presence. Defaulted values remain readable
    // through operator[] but do not count as user-provided options.
    std::size_t count(std::string_view name) const
    {
        return m_explicit.contains(std::string(name)) ? 1u : 0u;
    }

    const variable_value& operator[](std::string_view name) const
    {
        const auto it = m_values.find(std::string(name));
        if (it == m_values.end())
        {
            throw error("unknown option value: " + std::string(name));
        }
        return it->second;
    }

    variable_value& operator[](std::string_view name)
    {
        return m_values[std::string(name)];
    }

    void clear()
    {
        m_values.clear();
        m_explicit.clear();
    }

    void set(std::span<const std::string> names, variable_value value, bool explicit_value)
    {
        for (const auto& name : names)
        {
            m_values[name] = value;
            if (explicit_value)
            {
                m_explicit.insert(name);
            }
        }
    }

    bool has_value(std::string_view name) const
    {
        return m_values.contains(std::string(name));
    }

private:
    std::map<std::string, variable_value> m_values;
    std::set<std::string> m_explicit;
};

namespace detail
{

template<typename T>
struct is_vector : std::false_type
{};

template<typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type
{
    using value_type = T;
};

template<typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

inline std::string trim(std::string_view text)
{
    auto first = text.begin();
    auto last = text.end();
    while (first != last && std::isspace(static_cast<unsigned char>(*first)))
    {
        ++first;
    }
    while (first != last && std::isspace(static_cast<unsigned char>(*(last - 1))))
    {
        --last;
    }
    return std::string(first, last);
}

inline std::string lower(std::string_view text)
{
    std::string result(text);
    std::ranges::transform(result, result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

template<typename T>
T lexical_value(std::string_view token)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return std::string(token);
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        const auto lowered = lower(token);
        if (lowered == "1" || lowered == "true" || lowered == "on" || lowered == "yes")
        {
            return true;
        }
        if (lowered == "0" || lowered == "false" || lowered == "off" || lowered == "no")
        {
            return false;
        }
        throw error("invalid boolean value: " + std::string(token));
    }
    else if constexpr (std::is_constructible_v<T, std::string>)
    {
        return T(std::string(token));
    }
    else
    {
        T value{};
        std::istringstream input{std::string(token)};
        input >> value;
        if (!input || !input.eof())
        {
            throw error("invalid option value: " + std::string(token));
        }
        return value;
    }
}

inline std::vector<std::string> split_names(std::string_view spec)
{
    std::vector<std::string> names;
    std::size_t begin = 0;
    while (begin <= spec.size())
    {
        const auto comma = spec.find(',', begin);
        const auto end = comma == std::string_view::npos ? spec.size() : comma;
        auto name = trim(spec.substr(begin, end - begin));
        if (!name.empty())
        {
            names.push_back(std::move(name));
        }
        if (comma == std::string_view::npos)
        {
            break;
        }
        begin = comma + 1;
    }
    if (names.empty())
    {
        throw error("option has no name");
    }
    return names;
}

} // namespace detail

class value_semantic
{
public:
    virtual ~value_semantic() = default;
    virtual bool is_flag() const = 0;
    virtual bool is_multitoken() const = 0;
    virtual bool is_required() const = 0;
    virtual bool has_implicit() const = 0;
    virtual std::string value_hint() const = 0;
    virtual std::string default_hint() const = 0;
    virtual void apply_default(const std::vector<std::string>& names, variables_map& variables) const = 0;
    virtual void parse(const std::vector<std::string>& names,
                       std::span<const std::string_view> tokens,
                       variables_map& variables) const = 0;
    virtual void parse_implicit(const std::vector<std::string>& names, variables_map& variables) const = 0;
};

template<typename T>
class typed_value : public value_semantic
{
public:
    explicit typed_value(T* target = nullptr)
        : m_target(target)
    {}

    typed_value* default_value(T value)
    {
        m_default = std::move(value);
        return this;
    }

    typed_value* implicit_value(T value)
    {
        m_implicit = std::move(value);
        return this;
    }

    typed_value* multitoken()
    {
        m_multitoken = true;
        return this;
    }

    typed_value* required()
    {
        m_required = true;
        return this;
    }

    bool is_flag() const override
    {
        return m_flag;
    }

    bool is_multitoken() const override
    {
        return m_multitoken || detail::is_vector_v<T>;
    }

    bool is_required() const override
    {
        return m_required;
    }

    bool has_implicit() const override
    {
        return m_implicit.has_value();
    }

    std::string value_hint() const override
    {
        if (m_flag)
        {
            return {};
        }
        if constexpr (detail::is_vector_v<T>)
        {
            return "arg...";
        }
        else
        {
            return "arg";
        }
    }

    std::string default_hint() const override
    {
        if (!m_default.has_value() || m_flag)
        {
            return {};
        }
        if constexpr (requires(std::ostream& out, const T& value) { out << value; })
        {
            std::ostringstream out;
            if constexpr (std::is_same_v<T, bool>)
            {
                out << std::boolalpha;
            }
            out << *m_default;
            return out.str();
        }
        else
        {
            return {};
        }
    }

    void make_flag(T value)
    {
        m_flag = true;
        m_implicit = value;
        m_default = T{};
    }

    void apply_default(const std::vector<std::string>& names, variables_map& variables) const override
    {
        if (!m_default.has_value())
        {
            return;
        }
        assign(names, *m_default, variables, false);
    }

    void parse(const std::vector<std::string>& names,
               std::span<const std::string_view> tokens,
               variables_map& variables) const override
    {
        if constexpr (detail::is_vector_v<T>)
        {
            using Element = typename detail::is_vector<T>::value_type;
            T parsed;
            parsed.reserve(tokens.size());
            for (const auto token : tokens)
            {
                parsed.push_back(detail::lexical_value<Element>(token));
            }
            assign(names, std::move(parsed), variables, true);
        }
        else
        {
            if (tokens.empty())
            {
                throw error("missing value for option --" + names.front());
            }
            if (tokens.size() > 1)
            {
                throw error("too many values for option --" + names.front());
            }
            assign(names, detail::lexical_value<T>(tokens.front()), variables, true);
        }
    }

    void parse_implicit(const std::vector<std::string>& names, variables_map& variables) const override
    {
        if (!m_implicit.has_value())
        {
            throw error("missing value for option --" + names.front());
        }
        assign(names, *m_implicit, variables, true);
    }

private:
    void assign(const std::vector<std::string>& names, T value, variables_map& variables, bool explicit_value) const
    {
        if (m_target)
        {
            *m_target = value;
        }
        variables.set(std::span<const std::string>(names.data(), names.size()), variable_value(std::move(value)), explicit_value);
    }

    T* m_target = nullptr;
    std::optional<T> m_default;
    std::optional<T> m_implicit;
    bool m_multitoken = false;
    bool m_required = false;
    bool m_flag = false;
};

template<typename T>
typed_value<T>* value(T* target = nullptr)
{
    return new typed_value<T>(target);
}

inline typed_value<bool>* bool_switch(bool* target = nullptr)
{
    auto* semantic = new typed_value<bool>(target);
    semantic->make_flag(true);
    return semantic;
}

struct option_spec
{
    std::vector<std::string> names;
    std::shared_ptr<value_semantic> semantic;
    std::string description;
};

class options_description
{
public:
    explicit options_description(std::string caption = {})
        : m_caption(std::move(caption))
    {}

    class options_adder
    {
    public:
        explicit options_adder(options_description& owner)
            : m_owner(owner)
        {}

        options_adder& operator()(const char* spec, const char* description)
        {
            m_owner.add_option(spec, nullptr, description);
            return *this;
        }

        options_adder& operator()(const char* spec, value_semantic* semantic, const char* description)
        {
            m_owner.add_option(spec, std::shared_ptr<value_semantic>(semantic), description);
            return *this;
        }

    private:
        options_description& m_owner;
    };

    options_adder add_options()
    {
        return options_adder(*this);
    }

    options_description& add(const options_description& other)
    {
        m_options.insert(m_options.end(), other.m_options.begin(), other.m_options.end());
        return *this;
    }

    const option_spec* find(std::string_view name) const
    {
        for (const auto& option : m_options)
        {
            if (std::ranges::find(option.names, name) != option.names.end())
            {
                return &option;
            }
        }
        return nullptr;
    }

    const std::vector<option_spec>& options() const
    {
        return m_options;
    }

    void print(std::ostream& out) const
    {
        out << *this;
    }

    friend std::ostream& operator<<(std::ostream& out, const options_description& description)
    {
        if (!description.m_caption.empty())
        {
            out << description.m_caption << ":\n";
        }
        for (const auto& option : description.m_options)
        {
            out << "  " << description.format_names(option);
            if (option.semantic)
            {
                const auto value_hint = option.semantic->value_hint();
                if (!value_hint.empty())
                {
                    out << ' ' << value_hint;
                }
                const auto default_hint = option.semantic->default_hint();
                if (!default_hint.empty())
                {
                    out << " (=" << default_hint << ')';
                }
            }
            if (!option.description.empty())
            {
                out << "\n      " << option.description;
            }
            out << '\n';
        }
        return out;
    }

private:
    void add_option(std::string_view spec, std::shared_ptr<value_semantic> semantic, std::string_view description)
    {
        m_options.push_back(option_spec{detail::split_names(spec), std::move(semantic), std::string(description)});
    }

    std::string format_names(const option_spec& option) const
    {
        auto short_name = std::string{};
        auto long_name = option.names.front();
        for (const auto& name : option.names)
        {
            if (name.size() == 1 && short_name.empty())
            {
                short_name = name;
            }
            else if (name.size() > 1)
            {
                long_name = name;
            }
        }

        if (!short_name.empty() && !long_name.empty() && short_name != long_name)
        {
            return "-" + short_name + " [ --" + long_name + " ]";
        }
        if (!long_name.empty())
        {
            return "--" + long_name;
        }
        return "-" + short_name;
    }

    std::string m_caption;
    std::vector<option_spec> m_options;
};

class positional_options_description
{
public:
    positional_options_description& add(const char* name, int max_count)
    {
        m_entries.push_back({name, max_count});
        return *this;
    }

    struct entry
    {
        std::string name;
        int max_count;
    };

    const std::vector<entry>& entries() const
    {
        return m_entries;
    }

private:
    std::vector<entry> m_entries;
};

class parsed_options
{
public:
    std::vector<std::string_view> tokens;
    const options_description* options = nullptr;
    const positional_options_description* positional = nullptr;
};

class command_line_parser
{
public:
    command_line_parser(int argc, char** argv)
    {
        m_argv.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0u);
        for (int i = 1; i < argc; ++i)
        {
            m_argv.emplace_back(argv[i]);
        }
    }

    explicit command_line_parser(std::span<char const* const> argv)
    {
        m_argv.reserve(argv.size());
        for (const auto* arg : argv)
        {
            m_argv.emplace_back(arg);
        }
    }

    command_line_parser& options(const options_description& description)
    {
        m_options = &description;
        return *this;
    }

    command_line_parser& positional(const positional_options_description& description)
    {
        m_positional = &description;
        return *this;
    }

    parsed_options run() const
    {
        return parsed_options{m_argv, m_options, m_positional};
    }

private:
    std::vector<std::string_view> m_argv;
    const options_description* m_options = nullptr;
    const positional_options_description* m_positional = nullptr;
};

namespace detail
{

inline bool is_long_option(std::string_view token)
{
    return token.starts_with("--") && token.size() > 2;
}

inline bool is_short_option(std::string_view token)
{
    return token.starts_with('-') && !token.starts_with("--") && token.size() > 1;
}

inline bool looks_like_negative_number(std::string_view token)
{
    if (!token.starts_with('-') || token.size() < 2)
    {
        return false;
    }
    const auto first = static_cast<unsigned char>(token[1]);
    return std::isdigit(first) || token[1] == '.';
}

inline bool is_known_option(std::string_view token, const options_description& description)
{
    if (is_long_option(token))
    {
        auto name = token.substr(2);
        if (const auto equals = name.find('='); equals != std::string_view::npos)
        {
            name = name.substr(0, equals);
        }
        return description.find(name) != nullptr;
    }
    if (is_short_option(token))
    {
        auto name = token.substr(1);
        if (description.find(name) != nullptr)
        {
            return true;
        }
        if (name.size() > 1)
        {
            return description.find(name.substr(0, 1)) != nullptr;
        }
    }
    return false;
}

inline void parse_option_value(const option_spec& spec,
                               std::optional<std::string_view> attached_value,
                               const std::vector<std::string_view>& tokens,
                               std::size_t& index,
                               const options_description& description,
                               variables_map& variables)
{
    if (!spec.semantic)
    {
        if (attached_value.has_value())
        {
            throw error("option --" + spec.names.front() + " does not take a value");
        }
        variables.set(std::span<const std::string>(spec.names.data(), spec.names.size()), variable_value(true), true);
        return;
    }

    std::vector<std::string_view> values;
    if (attached_value.has_value())
    {
        values.push_back(*attached_value);
    }
    else if (spec.semantic->is_flag())
    {
        spec.semantic->parse_implicit(spec.names, variables);
        return;
    }
    else
    {
        const auto first = index + 1;
        if (first >= tokens.size())
        {
            if (spec.semantic->has_implicit())
            {
                spec.semantic->parse_implicit(spec.names, variables);
                return;
            }
            throw error("missing value for option --" + spec.names.front());
        }

        if (spec.semantic->is_multitoken())
        {
            for (auto cursor = first; cursor < tokens.size(); ++cursor)
            {
                if (is_known_option(tokens[cursor], description))
                {
                    break;
                }
                values.push_back(tokens[cursor]);
            }
            if (!values.empty())
            {
                index += values.size();
            }
        }
        else
        {
            if (is_known_option(tokens[first], description))
            {
                if (spec.semantic->has_implicit())
                {
                    spec.semantic->parse_implicit(spec.names, variables);
                    return;
                }
                throw error("missing value for option --" + spec.names.front());
            }
            values.push_back(tokens[first]);
            ++index;
        }
    }

    if (values.empty())
    {
        if (spec.semantic->has_implicit())
        {
            spec.semantic->parse_implicit(spec.names, variables);
            return;
        }
        throw error("missing value for option --" + spec.names.front());
    }
    spec.semantic->parse(spec.names, std::span<const std::string_view>(values.data(), values.size()), variables);
}

} // namespace detail

inline void store(const parsed_options& parsed, variables_map& variables)
{
    if (!parsed.options)
    {
        throw error("no option description was supplied");
    }

    variables.clear();
    for (const auto& option : parsed.options->options())
    {
        if (option.semantic)
        {
            option.semantic->apply_default(option.names, variables);
        }
    }

    std::vector<std::string_view> positional_tokens;
    const auto& tokens = parsed.tokens;
    for (std::size_t index = 0; index < tokens.size(); ++index)
    {
        auto token = tokens[index];
        if (token == "--")
        {
            positional_tokens.insert(positional_tokens.end(), tokens.begin() + static_cast<std::ptrdiff_t>(index + 1), tokens.end());
            break;
        }

        if (detail::is_long_option(token))
        {
            auto name = token.substr(2);
            std::optional<std::string_view> value;
            if (const auto equals = name.find('='); equals != std::string_view::npos)
            {
                value = name.substr(equals + 1);
                name = name.substr(0, equals);
            }
            const auto* spec = parsed.options->find(name);
            if (!spec)
            {
                throw error("unrecognized option --" + std::string(name));
            }
            detail::parse_option_value(*spec, value, tokens, index, *parsed.options, variables);
            continue;
        }

        if (detail::is_short_option(token))
        {
            auto name = token.substr(1);
            if (!detail::is_known_option(token, *parsed.options))
            {
                if (detail::looks_like_negative_number(token))
                {
                    positional_tokens.push_back(token);
                    continue;
                }
                throw error("unrecognized option -" + std::string(name));
            }

            std::optional<std::string_view> value;
            const option_spec* spec = parsed.options->find(name);
            if (!spec && name.size() > 1)
            {
                spec = parsed.options->find(name.substr(0, 1));
                value = name.substr(1);
            }
            if (!spec)
            {
                throw error("unrecognized option -" + std::string(name));
            }
            detail::parse_option_value(*spec, value, tokens, index, *parsed.options, variables);
            continue;
        }

        positional_tokens.push_back(token);
    }

    if (parsed.positional && !positional_tokens.empty())
    {
        std::map<std::string, std::vector<std::string_view>> assigned;
        std::size_t cursor = 0;
        for (const auto& entry : parsed.positional->entries())
        {
            if (cursor >= positional_tokens.size())
            {
                break;
            }
            const auto remaining = positional_tokens.size() - cursor;
            const auto take = entry.max_count < 0
                ? remaining
                : std::min<std::size_t>(remaining, static_cast<std::size_t>(entry.max_count));
            auto& values = assigned[entry.name];
            values.insert(values.end(), positional_tokens.begin() + static_cast<std::ptrdiff_t>(cursor),
                          positional_tokens.begin() + static_cast<std::ptrdiff_t>(cursor + take));
            cursor += take;
        }
        if (cursor != positional_tokens.size())
        {
            throw error("too many positional arguments");
        }

        for (const auto& [name, values] : assigned)
        {
            const auto* spec = parsed.options->find(name);
            if (!spec || !spec->semantic)
            {
                throw error("unknown positional option: " + name);
            }
            spec->semantic->parse(spec->names, std::span<const std::string_view>(values.data(), values.size()), variables);
        }
    }
    else if (!positional_tokens.empty())
    {
        throw error("too many positional arguments");
    }

    for (const auto& option : parsed.options->options())
    {
        if (option.semantic && option.semantic->is_required() && variables.count(option.names.front()) == 0)
        {
            throw error("required option is missing: --" + option.names.front());
        }
    }
}

inline void notify(variables_map&)
{}

} // namespace lvr2::cli
