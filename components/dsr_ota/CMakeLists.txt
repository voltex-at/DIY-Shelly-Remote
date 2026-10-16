idf_component_register(SRCS "dsr_ota_manifest.c" "dsr_ota_trust.c" "dsr_ota_image.c" "dsr_ota_fetch.c"
    INCLUDE_DIRS "include" PRIV_INCLUDE_DIRS "."
    REQUIRES dsr_core dsr_crypto json mbedtls esp_http_client esp_timer)
# These backend-only stages have no UI/worker caller yet. Retain the public
# verifier in target ELF so target builds and binary audits cover real code.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_manifest_verify")
if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    # This helper carries the upload board marker. Never retain it in an
    # unconfigured diagnostic image, which must remain ineligible for upload.
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_image_create" "-Wl,--undefined=dsr_ota_image_update" "-Wl,--undefined=dsr_ota_image_finish" "-Wl,--undefined=dsr_ota_image_context_size")
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_fetch_manifest" "-Wl,--undefined=dsr_ota_fetch_app")
endif()
target_compile_options(${COMPONENT_LIB} PRIVATE -fstack-usage)
