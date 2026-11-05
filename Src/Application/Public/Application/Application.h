#pragma once

#include "Application/ApplicationAPI.h"

#include "Asset/AssetManager.h"
#include "Renderer/Renderer.h"

#include <memory>
#include <string>

namespace Au
{
    struct ApplicationDesc
    {
        std::string Name = "AuApplication";
    };

    // Command-line options understood by Run():
    //   --content=<dir>   override the Content directory
    //   --rhi=<backend>   Vulkan or D3D12, overrides Config/Engine.ini
    class AU_APPLICATION_API Application
    {
    public:
        explicit Application(ApplicationDesc desc);
        virtual ~Application();

        Application(const Application&)            = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&)                 = delete;
        Application& operator=(Application&&)      = delete;

        int Run(int argc, char** argv);

    protected:
        virtual bool OnInitialize() { return true; }
        virtual void OnShutdown() {}

        [[nodiscard]] const ApplicationDesc& GetDesc() const { return m_Desc; }
        [[nodiscard]] Renderer&              GetRenderer() { return *m_Renderer; }
        [[nodiscard]] AssetManager&          GetAssetManager() { return *m_AssetManager; }

    private:
        ApplicationDesc               m_Desc;
        std::unique_ptr<AssetManager> m_AssetManager;
        std::unique_ptr<Renderer>     m_Renderer;
    };
} // namespace Au
