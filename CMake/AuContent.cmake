include_guard(GLOBAL)

set(AU_CONTENT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/Content")
set(AU_SAVED_SOURCE_DIR   "${PROJECT_SOURCE_DIR}/Saved")
set(AU_TEST_DATA_DIR      "${PROJECT_SOURCE_DIR}/Test/Data")

# Release builds are self-contained: Content/ is copied next to the executables.
# Other configurations read Content/ straight from the source tree, so nothing is copied.
add_custom_target(AuCopyContent
    COMMAND "$<IF:$<CONFIG:Release,MinSizeRel>,${CMAKE_COMMAND};-E;copy_directory_if_different;${AU_CONTENT_SOURCE_DIR};${AU_RUNTIME_OUTPUT_DIR}/Content,${CMAKE_COMMAND};-E;true>"
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Syncing Content/"
)
set_target_properties(AuCopyContent PROPERTIES FOLDER "Utility")

# Compile definitions that point a target at the source-tree Content/ and Saved/ directories in
# non-Release configurations. Only the module that resolves paths (Core) should need this.
function(au_target_use_source_content target)
    target_compile_definitions(${target} PRIVATE
        "$<$<NOT:$<CONFIG:Release,MinSizeRel>>:AU_SOURCE_CONTENT_DIR=\"${AU_CONTENT_SOURCE_DIR}\">"
        "$<$<NOT:$<CONFIG:Release,MinSizeRel>>:AU_SOURCE_SAVED_DIR=\"${AU_SAVED_SOURCE_DIR}\">"
    )
endfunction()
