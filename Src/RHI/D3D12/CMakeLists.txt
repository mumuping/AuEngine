au_add_module(RHID3D12
    PUBLIC_DEPS  Au::RHI
    PRIVATE_DEPS GPUOpen::D3D12MemoryAllocator
                 Microsoft::DirectX-Headers
                 Microsoft::DirectX-Guids
                 d3d12
                 dxgi
    FOLDER       Engine/RHI
)
