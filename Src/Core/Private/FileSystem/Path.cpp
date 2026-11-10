#include "Core/FileSystem/Path.h"

#include "Core/Common/Error.h"
#include "Core/Common/StringUtil.h"
#include "Core/Common/Types.h"

#include <utility>
#include <vector>

namespace Au::Fs {

    Path::Path(std::string alreadyNormalizedUtf8) : m_Value(std::move(alreadyNormalizedUtf8)) {
    }

    Result<Path> Path::Normalize(std::string_view utf8) {
        if (utf8.empty()) {
            return Unexpected(Error::Make("virtual path must not be empty"));
        }
        if (utf8.front() != '/') {
            return Unexpected(Error::Format({}, "virtual path must be absolute: '{}'", utf8));
        }
        for (const char character : utf8) {
            if (character == '\\' || character == '\0') {
                return Unexpected(Error::Format({}, "virtual path contains illegal character: '{}'", utf8));
            }
        }

        std::vector<std::string_view> segments;
        usize                         index = 1;
        while (index <= utf8.size()) {
            const usize begin = index;
            while (index < utf8.size() && utf8[index] != '/') {
                ++index;
            }
            const std::string_view segment = utf8.substr(begin, index - begin);
            if (segment.empty() || segment == ".") {
                // Skip empty segments from repeated '/' and current-dir references.
            } else if (segment == "..") {
                if (segments.empty()) {
                    return Unexpected(Error::Format({}, "virtual path escapes root: '{}'", utf8));
                }
                segments.pop_back();
            } else {
                segments.push_back(segment);
            }
            if (index < utf8.size() && utf8[index] == '/') {
                ++index;
            } else {
                break;
            }
        }

        if (segments.empty()) {
            return Path{"/"};
        }

        std::string normalized;
        normalized.reserve(utf8.size());
        for (const std::string_view segment : segments) {
            normalized.push_back('/');
            normalized.append(segment);
        }
        return Path{std::move(normalized)};
    }

    bool Path::IsAbsolute() const noexcept {
        return !m_Value.empty() && m_Value.front() == '/';
    }

    Path Path::Parent() const {
        if (m_Value.empty() || m_Value == "/") {
            return Path{"/"};
        }
        const usize slash = m_Value.find_last_of('/');
        if (slash == 0) {
            return Path{"/"};
        }
        return Path{m_Value.substr(0, slash)};
    }

    Path Path::Filename() const {
        if (m_Value.empty() || m_Value == "/") {
            return Path{};
        }
        const usize slash = m_Value.find_last_of('/');
        if (slash == std::string::npos) {
            return Path{m_Value};
        }
        return Path{m_Value.substr(slash + 1)};
    }

    Result<Path> Path::Join(std::string_view child) const {
        if (child.empty()) {
            return *this;
        }
        if (child.front() == '/') {
            return Normalize(child);
        }
        if (m_Value.empty()) {
            return Unexpected(Error::Make("cannot join onto an empty path"));
        }

        std::string combined;
        if (IsRoot()) {
            combined.reserve(1 + child.size());
            combined.push_back('/');
            combined.append(child);
        } else {
            combined.reserve(m_Value.size() + 1 + child.size());
            combined = m_Value;
            combined.push_back('/');
            combined.append(child);
        }
        return Normalize(combined);
    }

    bool Path::EqualsIgnoreCase(const Path& other) const noexcept {
        return AsciiEqualsIgnoreCase(m_Value, other.m_Value);
    }

    bool Path::IsMountPrefixOf(const Path& other) const noexcept {
        if (m_Value.empty() || other.m_Value.empty()) {
            return false;
        }
        if (m_Value == "/") {
            return other.IsAbsolute();
        }
        if (!AsciiStartsWithIgnoreCase(other.m_Value, m_Value)) {
            return false;
        }
        return other.m_Value.size() == m_Value.size() || other.m_Value[m_Value.size()] == '/';
    }

    Result<Path> Path::StripMountPrefix(const Path& other) const {
        if (!IsMountPrefixOf(other)) {
            return Unexpected(Error::Format({}, "path '{}' is not under mount '{}'", other.View(), View()));
        }
        if (m_Value == "/") {
            return other;
        }
        if (AsciiEqualsIgnoreCase(m_Value, other.m_Value)) {
            return Path{"/"};
        }
        std::string remainder = "/";
        remainder.append(other.m_Value.substr(m_Value.size() + 1));
        return Normalize(remainder);
    }

}  // namespace Au::Fs
