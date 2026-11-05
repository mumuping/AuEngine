#include "RHID3D12/D3D12RHI.h"

#include "Core/Log.h"

#include <Windows.h>

#include <directx/d3d12.h>
#include <dxguids/dxguids.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <D3D12MemAlloc.h>

#include <string>

namespace Au::RHI
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        std::string WideToUtf8(const wchar_t* text)
        {
            const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
            if (size <= 1)
            {
                return {};
            }
            std::string result(static_cast<std::size_t>(size - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
            return result;
        }

        class D3D12Device final : public IRHIDevice
        {
        public:
            D3D12Device(ComPtr<ID3D12Device> device, ComPtr<D3D12MA::Allocator> allocator, std::string adapterName)
                : m_Device(std::move(device))
                , m_Allocator(std::move(allocator))
                , m_AdapterName(std::move(adapterName))
            {
            }

            [[nodiscard]] ERHIBackend GetBackend() const override
            {
                return ERHIBackend::D3D12;
            }

            [[nodiscard]] std::string_view GetAdapterName() const override
            {
                return m_AdapterName;
            }

        private:
            ComPtr<ID3D12Device>       m_Device;
            ComPtr<D3D12MA::Allocator> m_Allocator;
            std::string                m_AdapterName;
        };

        ComPtr<IDXGIAdapter1> PickAdapter(IDXGIFactory6* factory)
        {
            ComPtr<IDXGIAdapter1> adapter;
            for (UINT index = 0;
                 factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                     IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
                 ++index)
            {
                DXGI_ADAPTER_DESC1 desc{};
                adapter->GetDesc1(&desc);
                if ((desc.Flags & static_cast<UINT>(DXGI_ADAPTER_FLAG_SOFTWARE)) != 0)
                {
                    continue;
                }
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
                {
                    return adapter;
                }
            }
            return nullptr;
        }
    } // namespace

    std::unique_ptr<IRHIDevice> CreateD3D12Device()
    {
        UINT factoryFlags = 0;
#if AU_DEBUG
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        ComPtr<IDXGIFactory6> factory;
        if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory))))
        {
            // The DXGI debug layer is optional (Graphics Tools feature); retry without it.
            if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
            {
                AU_LOG_ERROR("D3D12: CreateDXGIFactory2 failed");
                return nullptr;
            }
        }

        const ComPtr<IDXGIAdapter1> adapter = PickAdapter(factory.Get());
        if (!adapter)
        {
            AU_LOG_ERROR("D3D12: no hardware adapter supports feature level 12_0");
            return nullptr;
        }

        ComPtr<ID3D12Device> device;
        if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
        {
            AU_LOG_ERROR("D3D12: D3D12CreateDevice failed");
            return nullptr;
        }

        D3D12MA::ALLOCATOR_DESC allocatorDesc{};
        allocatorDesc.pDevice  = device.Get();
        allocatorDesc.pAdapter = adapter.Get();
        ComPtr<D3D12MA::Allocator> allocator;
        if (FAILED(D3D12MA::CreateAllocator(&allocatorDesc, &allocator)))
        {
            AU_LOG_ERROR("D3D12: failed to create D3D12MA allocator");
            return nullptr;
        }

        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        return std::make_unique<D3D12Device>(std::move(device), std::move(allocator),
                                             WideToUtf8(std::data(desc.Description)));
    }
} // namespace Au::RHI
