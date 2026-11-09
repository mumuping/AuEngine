au_add_module(Core
    PUBLIC_DEPS  fmt::fmt
    PRIVATE_DEPS spdlog::spdlog 
    ThirdParty::mimalloc $<$<PLATFORM_ID:Windows>:shell32>
)

au_target_use_source_content(AuCore)
