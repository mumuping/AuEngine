#include "Core/Common/SingletonSystem.h"

#include <utility>

namespace Au {
    SingletonSystem& SingletonSystem::Get() {
        static SingletonSystem s_Instance;
        return s_Instance;
    }

    SingletonSystem::~SingletonSystem() {
        Shutdown();
    }

    void SingletonSystem::Initialize() {
        for (Entity& entry : m_Entities) {
            if (entry.Initialized || entry.CreateFn == nullptr) {
                continue;
            }

            entry.CreateFn(entry.Context.get());
            entry.Context.reset();
            entry.Initialized = true;
        }
    }

    void SingletonSystem::Shutdown() {
        for (auto it = m_Entities.rbegin(); it != m_Entities.rend(); ++it) {
            Entity& entry = *it;
            if (entry.Initialized) {
                if (entry.Owned) {
                    if (entry.DestroyFn != nullptr) {
                        entry.DestroyFn();
                    }
                } else {
                    ExchangeInstance(entry.TypeKey, nullptr);
                }
                entry.Initialized = false;
            }

            entry.Context.reset();
        }

        m_Entities.clear();
        m_Instances.clear();
    }

    void* SingletonSystem::GetInstance(TypeKeyID id) const {
        const auto it = m_Instances.find(id);
        return it != m_Instances.end() ? it->second : nullptr;
    }

    void* SingletonSystem::ExchangeInstance(TypeKeyID id, void* instance) {
        if (instance == nullptr) {
            const auto it = m_Instances.find(id);
            if (it == m_Instances.end()) {
                return nullptr;
            }
            void* const previous = it->second;
            m_Instances.erase(it);
            return previous;
        }

        const auto [it, inserted] = m_Instances.try_emplace(id, instance);
        if (inserted) {
            return nullptr;
        }
        return std::exchange(it->second, instance);
    }

    void SingletonSystem::AssertTypeNotRegistered(TypeKeyID id) const {
        assert(GetInstance(id) == nullptr && "Singleton type already has a live instance");
        for (const Entity& entry : m_Entities) {
            assert(entry.TypeKey != id && "Singleton type already registered");
        }
    }
}  // namespace Au
