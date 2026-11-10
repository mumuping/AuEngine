#include "Core/FileSystem/MemoryFileSystem.h"

#include "Core/Common/Assert.h"
#include "Core/Common/Error.h"
#include "Core/Common/StringUtil.h"

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace Au::Fs {
    namespace {

        struct MemoryNode {
            enum class EKind : u8 { Directory, File };

            EKind                                    Kind = EKind::Directory;
            std::string                              Name;
            std::vector<std::unique_ptr<MemoryNode>> Children;
            std::vector<u8>                          Data;
        };

        [[nodiscard]] const MemoryNode* FindChild(const MemoryNode& parent, std::string_view name) {
            for (const std::unique_ptr<MemoryNode>& child : parent.Children) {
                if (AsciiEqualsIgnoreCase(child->Name, name)) {
                    return child.get();
                }
            }
            return nullptr;
        }

        [[nodiscard]] MemoryNode* FindChild(MemoryNode& parent, std::string_view name) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<MemoryNode*>(FindChild(std::as_const(parent), name));
        }

        [[nodiscard]] Result<std::vector<std::string_view>> SplitAbsolute(const Path& path) {
            if (!path.IsAbsolute()) {
                return Unexpected(Error::Format({}, "memory fs path must be absolute: '{}'", path.View()));
            }
            if (path.IsRoot()) {
                return std::vector<std::string_view>{};
            }

            std::vector<std::string_view> segments;
            const std::string_view        view  = path.View();
            usize                         index = 1;
            while (index < view.size()) {
                const usize begin = index;
                while (index < view.size() && view[index] != '/') {
                    ++index;
                }
                segments.push_back(view.substr(begin, index - begin));
                if (index < view.size() && view[index] == '/') {
                    ++index;
                }
            }
            return segments;
        }

        [[nodiscard]] const MemoryNode* Walk(const MemoryNode& root, std::span<const std::string_view> segments) {
            const MemoryNode* current = &root;
            for (const std::string_view segment : segments) {
                if (current->Kind != MemoryNode::EKind::Directory) {
                    return nullptr;
                }
                current = FindChild(*current, segment);
                if (current == nullptr) {
                    return nullptr;
                }
            }
            return current;
        }

        [[nodiscard]] MemoryNode* Walk(MemoryNode& root, std::span<const std::string_view> segments) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<MemoryNode*>(Walk(std::as_const(root), segments));
        }

        class MemoryFile final : public IFile {
          public:
            MemoryFile(MemoryNode* node, EFileAccess access, bool append)
                : m_Node(node), m_Access(access), m_Position(append ? node->Data.size() : 0) {
                AU_ENSURE(m_Node != nullptr);
                AU_ENSURE(m_Node->Kind == MemoryNode::EKind::File);
            }

            Result<usize> Read(std::span<u8> buffer) override {
                AU_ENSURE(m_Node != nullptr);
                if (m_Access == EFileAccess::Write) {
                    return Unexpected(Error::Make("file not opened for reading"));
                }
                if (m_Position >= m_Node->Data.size()) {
                    return usize{0};
                }
                const usize remaining = m_Node->Data.size() - m_Position;
                const usize toRead    = std::min(remaining, buffer.size());
                std::copy_n(m_Node->Data.begin() + static_cast<std::ptrdiff_t>(m_Position), toRead, buffer.begin());
                m_Position += toRead;
                return toRead;
            }

            Result<usize> Write(std::span<const u8> buffer) override {
                AU_ENSURE(m_Node != nullptr);
                if (m_Access == EFileAccess::Read) {
                    return Unexpected(Error::Make("file not opened for writing"));
                }
                if (m_Position > m_Node->Data.size()) {
                    m_Node->Data.resize(m_Position, 0);
                }
                const usize end = m_Position + buffer.size();
                if (end > m_Node->Data.size()) {
                    m_Node->Data.resize(end);
                }
                std::ranges::copy(buffer, m_Node->Data.begin() + static_cast<std::ptrdiff_t>(m_Position));
                m_Position = end;
                return buffer.size();
            }

            Status Seek(i64 offset, ESeekOrigin origin) override {
                AU_ENSURE(m_Node != nullptr);
                i64 target = 0;
                switch (origin) {
                    case ESeekOrigin::Begin:
                        target = offset;
                        break;
                    case ESeekOrigin::Current:
                        target = static_cast<i64>(m_Position) + offset;
                        break;
                    case ESeekOrigin::End:
                        target = static_cast<i64>(m_Node->Data.size()) + offset;
                        break;
                }
                if (target < 0) {
                    return Unexpected(Error::Make("seek before beginning of file"));
                }
                m_Position = static_cast<usize>(target);
                return {};
            }

            [[nodiscard]] u64 Tell() const override {
                AU_ENSURE(m_Node != nullptr);
                return m_Position;
            }

            [[nodiscard]] u64 Size() const override {
                AU_ENSURE(m_Node != nullptr);
                return m_Node->Data.size();
            }

          private:
            MemoryNode* m_Node     = nullptr;
            EFileAccess m_Access   = EFileAccess::Read;
            usize       m_Position = 0;
        };

    }  // namespace

    struct MemoryFileSystem::Impl {
        MemoryNode Root{};
    };

    MemoryFileSystem::MemoryFileSystem() : m_Impl(std::make_unique<Impl>()) {
        m_Impl->Root.Kind = MemoryNode::EKind::Directory;
        m_Impl->Root.Name = "/";
    }

    MemoryFileSystem::~MemoryFileSystem() = default;

    Result<FileStat> MemoryFileSystem::Stat(const Path& path) const {
        const Result<std::vector<std::string_view>> segmentsResult = SplitAbsolute(path);
        if (!segmentsResult) {
            return Unexpected(segmentsResult.error());
        }
        const MemoryNode* node = Walk(m_Impl->Root, *segmentsResult);
        if (node == nullptr) {
            return FileStat{};
        }
        FileStat stat;
        stat.Exists        = true;
        stat.IsDirectory   = node->Kind == MemoryNode::EKind::Directory;
        stat.IsRegularFile = node->Kind == MemoryNode::EKind::File;
        stat.Size          = node->Kind == MemoryNode::EKind::File ? node->Data.size() : 0;
        return stat;
    }

    Result<std::unique_ptr<IFile>> MemoryFileSystem::Open(const Path& path, EFileAccess access, EFileMode mode) {
        Result<std::vector<std::string_view>> segmentsResult = SplitAbsolute(path);
        if (!segmentsResult) {
            return Unexpected(segmentsResult.error());
        }
        std::vector<std::string_view>& segments = *segmentsResult;
        if (segments.empty()) {
            return Unexpected(Error::Make("cannot open filesystem root as a file"));
        }

        const std::string_view            fileName = segments.back();
        const std::span<std::string_view> parents(segments.data(), segments.size() - 1);

        MemoryNode* parent = &m_Impl->Root;
        if (mode != EFileMode::OpenExisting && mode != EFileMode::Truncate) {
            for (const std::string_view segment : parents) {
                MemoryNode* child = FindChild(*parent, segment);
                if (child == nullptr) {
                    auto created  = std::make_unique<MemoryNode>();
                    created->Kind = MemoryNode::EKind::Directory;
                    created->Name = std::string(segment);
                    child         = created.get();
                    parent->Children.push_back(std::move(created));
                } else if (child->Kind != MemoryNode::EKind::Directory) {
                    return Unexpected(Error::Format({}, "parent path component is a file: '{}'", path.View()));
                }
                parent = child;
            }
        } else {
            parent = Walk(m_Impl->Root, parents);
            if (parent == nullptr || parent->Kind != MemoryNode::EKind::Directory) {
                return Unexpected(Error::Format({}, "parent directory does not exist: '{}'", path.View()));
            }
        }

        MemoryNode* node   = FindChild(*parent, fileName);
        bool        append = false;

        switch (mode) {
            case EFileMode::OpenExisting:
                if (node == nullptr || node->Kind != MemoryNode::EKind::File) {
                    return Unexpected(Error::Format({}, "file not found: '{}'", path.View()));
                }
                break;
            case EFileMode::CreateNew:
                if (node != nullptr) {
                    return Unexpected(Error::Format({}, "file already exists: '{}'", path.View()));
                }
                break;
            case EFileMode::CreateAlways:
                break;
            case EFileMode::Append:
                append = true;
                break;
            case EFileMode::Truncate:
                if (node == nullptr || node->Kind != MemoryNode::EKind::File) {
                    return Unexpected(Error::Format({}, "file not found: '{}'", path.View()));
                }
                break;
        }

        if (node != nullptr && node->Kind != MemoryNode::EKind::File) {
            return Unexpected(Error::Format({}, "path is a directory: '{}'", path.View()));
        }

        const bool needsWrite = access == EFileAccess::Write || access == EFileAccess::ReadWrite ||
                                mode == EFileMode::CreateNew || mode == EFileMode::CreateAlways ||
                                mode == EFileMode::Append || mode == EFileMode::Truncate;
        if (needsWrite && access == EFileAccess::Read &&
            (mode == EFileMode::CreateNew || mode == EFileMode::CreateAlways || mode == EFileMode::Append ||
             mode == EFileMode::Truncate)) {
            return Unexpected(Error::Make("create/truncate/append requires write access"));
        }

        if (node == nullptr) {
            if (mode == EFileMode::OpenExisting || mode == EFileMode::Truncate) {
                return Unexpected(Error::Format({}, "file not found: '{}'", path.View()));
            }
            auto created  = std::make_unique<MemoryNode>();
            created->Kind = MemoryNode::EKind::File;
            created->Name = std::string(fileName);
            node          = created.get();
            parent->Children.push_back(std::move(created));
        } else if (mode == EFileMode::CreateAlways || mode == EFileMode::Truncate) {
            node->Data.clear();
        }

        return std::unique_ptr<IFile>(std::make_unique<MemoryFile>(node, access, append));
    }

    Status MemoryFileSystem::Remove(const Path& path) {
        Result<std::vector<std::string_view>> segmentsResult = SplitAbsolute(path);
        if (!segmentsResult) {
            return Unexpected(segmentsResult.error());
        }
        std::vector<std::string_view>& segments = *segmentsResult;
        if (segments.empty()) {
            return Unexpected(Error::Make("cannot remove filesystem root"));
        }

        const std::string_view            name = segments.back();
        const std::span<std::string_view> parents(segments.data(), segments.size() - 1);
        MemoryNode*                       parent = Walk(m_Impl->Root, parents);
        if (parent == nullptr || parent->Kind != MemoryNode::EKind::Directory) {
            return Unexpected(Error::Format({}, "path not found: '{}'", path.View()));
        }

        const auto iterator = std::ranges::find_if(parent->Children, [&](const std::unique_ptr<MemoryNode>& child) {
            return AsciiEqualsIgnoreCase(child->Name, name);
        });
        if (iterator == parent->Children.end()) {
            return Unexpected(Error::Format({}, "path not found: '{}'", path.View()));
        }
        if ((*iterator)->Kind == MemoryNode::EKind::Directory && !(*iterator)->Children.empty()) {
            return Unexpected(Error::Format({}, "directory not empty: '{}'", path.View()));
        }
        parent->Children.erase(iterator);
        return {};
    }

    Status MemoryFileSystem::CreateDirectories(const Path& path) {
        Result<std::vector<std::string_view>> segmentsResult = SplitAbsolute(path);
        if (!segmentsResult) {
            return Unexpected(segmentsResult.error());
        }
        MemoryNode* current = &m_Impl->Root;
        for (const std::string_view segment : *segmentsResult) {
            MemoryNode* child = FindChild(*current, segment);
            if (child == nullptr) {
                auto created  = std::make_unique<MemoryNode>();
                created->Kind = MemoryNode::EKind::Directory;
                created->Name = std::string(segment);
                child         = created.get();
                current->Children.push_back(std::move(created));
            } else if (child->Kind != MemoryNode::EKind::Directory) {
                return Unexpected(Error::Format({}, "path component is a file: '{}'", path.View()));
            }
            current = child;
        }
        return {};
    }

    Result<std::vector<Path>> MemoryFileSystem::Enumerate(const Path& directory) const {
        const Result<std::vector<std::string_view>> segmentsResult = SplitAbsolute(directory);
        if (!segmentsResult) {
            return Unexpected(segmentsResult.error());
        }
        const MemoryNode* node = Walk(m_Impl->Root, *segmentsResult);
        if (node == nullptr) {
            return Unexpected(Error::Format({}, "directory not found: '{}'", directory.View()));
        }
        if (node->Kind != MemoryNode::EKind::Directory) {
            return Unexpected(Error::Format({}, "path is not a directory: '{}'", directory.View()));
        }

        std::vector<Path> children;
        children.reserve(node->Children.size());
        for (const std::unique_ptr<MemoryNode>& child : node->Children) {
            Result<Path> joined =
                directory.IsRoot() ? Path::Normalize(std::string("/") + child->Name) : directory.Join(child->Name);
            if (!joined) {
                return Unexpected(joined.error());
            }
            children.push_back(std::move(*joined));
        }
        return children;
    }

}  // namespace Au::Fs
