idf_component_register(
    SRCS "vendor/monocypher.c" "vendor/optional/monocypher-ed25519.c"
    INCLUDE_DIRS "vendor" "vendor/optional")
# Keep the upstream files intact. The final link only needs Ed25519 verify;
# unused signing, key generation and unrelated primitives are garbage-collected.
target_compile_options(${COMPONENT_LIB} PRIVATE -ffunction-sections -fdata-sections)
