#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace Au
{
    // Minimal INI reader: "[Section]" headers, "Key=Value" pairs, ';' or '#' comments.
    class ConfigFile
    {
    public:
        static ConfigFile Parse(std::string_view text);

        [[nodiscard]] std::optional<std::string> Get(std::string_view section, std::string_view key) const;

    private:
        std::map<std::string, std::string, std::less<>> m_Values;
    };
} // namespace Au
