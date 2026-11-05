#include "ConfigFile.h"

namespace Au
{
    namespace
    {
        std::string_view Trim(std::string_view text)
        {
            constexpr std::string_view whitespace = " \t\r\n";
            const auto first = text.find_first_not_of(whitespace);
            if (first == std::string_view::npos)
            {
                return {};
            }
            const auto last = text.find_last_not_of(whitespace);
            return text.substr(first, last - first + 1);
        }

        std::string MakeKey(std::string_view section, std::string_view key)
        {
            std::string result(section);
            result += '.';
            result += key;
            return result;
        }
    } // namespace

    ConfigFile ConfigFile::Parse(std::string_view text)
    {
        ConfigFile  config;
        std::string section;

        while (!text.empty())
        {
            const auto             lineEnd = text.find('\n');
            const std::string_view line    = Trim(text.substr(0, lineEnd));
            text = lineEnd == std::string_view::npos ? std::string_view() : text.substr(lineEnd + 1);

            if (line.empty() || line.front() == ';' || line.front() == '#')
            {
                continue;
            }
            if (line.front() == '[' && line.back() == ']')
            {
                section = Trim(line.substr(1, line.size() - 2));
                continue;
            }
            if (const auto equals = line.find('='); equals != std::string_view::npos)
            {
                config.m_Values[MakeKey(section, Trim(line.substr(0, equals)))] = Trim(line.substr(equals + 1));
            }
        }
        return config;
    }

    std::optional<std::string> ConfigFile::Get(std::string_view section, std::string_view key) const
    {
        const auto it = m_Values.find(MakeKey(section, key));
        if (it == m_Values.end())
        {
            return std::nullopt;
        }
        return it->second;
    }
} // namespace Au
