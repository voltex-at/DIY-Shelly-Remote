idf_component_register(SRCS "dsr_ota_manifest.c" "dsr_ota_trust.c"
    INCLUDE_DIRS "include" PRIV_INCLUDE_DIRS "."
    REQUIRES dsr_core dsr_crypto json)
# This backend-only stage has no downloader/UI caller yet. Retain the public
# verifier in target ELF so target builds and binary audits cover real code.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_manifest_verify")
