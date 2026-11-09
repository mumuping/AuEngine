#include "Core/Common/TypeKey.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace Au {
    namespace {
        std::mutex& TypeKeyMutex() {
            static std::mutex s_Mutex;
            return s_Mutex;
        }

        std::unordered_map<std::string, TypeKeyID>& TypeKeyMap() {
            static std::unordered_map<std::string, TypeKeyID> s_Map;
            return s_Map;
        }

        TypeKeyID& NextTypeKeyID() {
            static TypeKeyID s_Next = g_InvalidTypeKeyID + 1;
            return s_Next;
        }
    }  // namespace

    TypeKeyID InternTypeKey(std::string_view uniqueName) {
        const std::scoped_lock lock(TypeKeyMutex());

        std::unordered_map<std::string, TypeKeyID>& map = TypeKeyMap();
        const std::string                           key(uniqueName);
        if (const auto it = map.find(key); it != map.end()) {
            return it->second;
        }

        const TypeKeyID id = NextTypeKeyID()++;
        map.emplace(key, id);
        return id;
    }
}  // namespace Au
