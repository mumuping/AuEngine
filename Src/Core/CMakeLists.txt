au_add_module(Core
    PUBLIC_DEPS  spdlog::spdlog
    PRIVATE_DEPS ThirdParty::mimalloc $<$<PLATFORM_ID:Windows>:shell32>
)

au_target_use_source_content(AuCore)
