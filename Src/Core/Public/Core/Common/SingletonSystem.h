#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/TypeKey.h"
#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Au {

    enum class ESingletonInitMode : u8 {
        Deferred,
        Immediate,
    };

    // Process-wide singleton locator with explicit Initialize / Shutdown ordering.
    // Register/Init/Shutdown are single-threaded; Get is safe for concurrent readers after Init.
    class AU_CORE_API SingletonSystem {
      public:
        AU_DISABLE_COPY_MOVE(SingletonSystem);

        static SingletonSystem& Get();

        ~SingletonSystem();

        template <typename T>
        T& Get() {
            void* instance = GetInstance(TypeKeyTag<T>());
            assert(instance != nullptr && "Singleton not registered or not initialized");
            return *static_cast<T*>(instance);
        }

        template <typename T>
        T* TryGet() noexcept {
            return static_cast<T*>(GetInstance(TypeKeyTag<T>()));
        }

        template <typename T, typename... Args>
            requires std::constructible_from<T, Args...>
        void Register(ESingletonInitMode mode, Args&&... args) {
            if (mode == ESingletonInitMode::Immediate) {
                RegisterImmediate<T>(std::forward<Args>(args)...);
            } else {
                RegisterDeferred<T>(std::forward<Args>(args)...);
            }
        }

        template <typename T, typename... Args>
            requires std::constructible_from<T, Args...>
        void RegisterImmediate(Args&&... args) {
            const TypeKeyID typeKey = TypeKeyTag<T>();
            AssertTypeNotRegistered(typeKey);

            using Context = std::tuple<std::decay_t<Args>...>;

            Entity entry;
            entry.TypeKey   = typeKey;
            entry.Owned     = true;
            entry.Context   = MakeContext<Context>(std::forward<Args>(args)...);
            entry.CreateFn  = &SingletonSystem::CreateOwnedSingleton<T, Context>;
            entry.DestroyFn = &SingletonSystem::DestroyOwnedSingleton<T>;

            entry.CreateFn(entry.Context.get());
            entry.Context.reset();
            entry.Initialized = true;

            m_Entities.push_back(std::move(entry));
        }

        template <typename T, typename... Args>
            requires std::constructible_from<T, Args...>
        void RegisterDeferred(Args&&... args) {
            const TypeKeyID typeKey = TypeKeyTag<T>();
            AssertTypeNotRegistered(typeKey);

            using Context = std::tuple<std::decay_t<Args>...>;

            Entity entry;
            entry.TypeKey     = typeKey;
            entry.Owned       = true;
            entry.Initialized = false;
            entry.Context     = MakeContext<Context>(std::forward<Args>(args)...);
            entry.CreateFn    = &SingletonSystem::CreateOwnedSingleton<T, Context>;
            entry.DestroyFn   = &SingletonSystem::DestroyOwnedSingleton<T>;

            m_Entities.push_back(std::move(entry));
        }

        // Hooks an externally owned instance for cross-module lookup. Shutdown will not delete it.
        template <typename T>
        void RegisterInstance(T* instance) {
            assert(instance != nullptr);
            const TypeKeyID typeKey = TypeKeyTag<T>();
            AssertTypeNotRegistered(typeKey);

            Entity entry;
            entry.TypeKey     = typeKey;
            entry.Owned       = false;
            entry.Initialized = true;
            m_Entities.push_back(std::move(entry));

            void* const previous = ExchangeInstance(typeKey, instance);
            assert(previous == nullptr);
        }

        void Initialize();
        void Shutdown();

      private:
        SingletonSystem() = default;

        using ContextPtr = std::unique_ptr<void, void (*)(void*)>;

        static void NoOpContextDeleter(void* /*unused*/) noexcept {}

        template <typename Context, typename... Args>
        static ContextPtr MakeContext(Args&&... args) {
            return ContextPtr(new Context(std::forward<Args>(args)...), [](void* contextPtr) {
                delete static_cast<Context*>(contextPtr);  // NOLINT(cppcoreguidelines-owning-memory)
            });
        }

        struct Entity {
            TypeKeyID TypeKey       = 0;
            void (*CreateFn)(void*) = nullptr;
            void (*DestroyFn)()     = nullptr;
            ContextPtr Context{nullptr, &NoOpContextDeleter};
            bool       Initialized = false;
            bool       Owned       = true;
        };

        [[nodiscard]] void* GetInstance(TypeKeyID id) const;
        void*               ExchangeInstance(TypeKeyID id, void* instance);
        void                AssertTypeNotRegistered(TypeKeyID id) const;

        template <typename T, typename Context>
        static void CreateOwnedSingleton(void* contextPtr) {
            Context& context = *static_cast<Context*>(contextPtr);
            std::apply(
                [](auto&... constructorArgs) {
                    T* instance = new T(std::move(constructorArgs)...);  // NOLINT(cppcoreguidelines-owning-memory)
                    void* const previous = SingletonSystem::Get().ExchangeInstance(TypeKeyTag<T>(), instance);
                    // Duplicate should be impossible after AssertTypeNotRegistered; if it happens,
                    // keep the existing instance and discard the one we just created.
                    assert(previous == nullptr && "Duplicate singleton instance");
                    if (previous != nullptr) {
                        SingletonSystem::Get().ExchangeInstance(TypeKeyTag<T>(), previous);
                        delete instance;  // NOLINT(cppcoreguidelines-owning-memory)
                    }
                },
                context
            );
        }

        template <typename T>
        static void DestroyOwnedSingleton() {
            void* instance = SingletonSystem::Get().ExchangeInstance(TypeKeyTag<T>(), nullptr);
            delete static_cast<T*>(instance);  // NOLINT(cppcoreguidelines-owning-memory)
        }

        std::unordered_map<TypeKeyID, void*> m_Instances;
        std::vector<Entity>                  m_Entities;
    };

}  // namespace Au

#define AU_REGISTER_SINGLETON_DEFERRED(SingletonType, ...) \
    ::Au::SingletonSystem::Get().Register<SingletonType>(::Au::ESingletonInitMode::Deferred, ##__VA_ARGS__)

#define AU_REGISTER_SINGLETON_IMMEDIATE(SingletonType, ...) \
    ::Au::SingletonSystem::Get().Register<SingletonType>(::Au::ESingletonInitMode::Immediate, ##__VA_ARGS__)

#define AU_INITIALIZE_SINGLETONS() ::Au::SingletonSystem::Get().Initialize()

#define AU_SHUTDOWN_SINGLETONS() ::Au::SingletonSystem::Get().Shutdown()

#define AU_GET_SINGLETON(SingletonType) (::Au::SingletonSystem::Get().Get<SingletonType>())
