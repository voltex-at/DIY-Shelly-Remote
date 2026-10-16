idf_component_register(SRCS "dsr_ota_manifest.c" "dsr_ota_trust.c" "dsr_ota_image.c" "dsr_ota_fetch.c" "dsr_ota_release.c" "dsr_ota_release_fetch.c" "dsr_ota_release_chain.c" "dsr_ota_discovery_fetch.c"
    INCLUDE_DIRS "include" PRIV_INCLUDE_DIRS "."
    REQUIRES dsr_core dsr_crypto json mbedtls esp_http_client esp_timer)
# Native UI/worker now calls the genuine verifier/stream chain. Retain the
# standalone backend entry points too so target audits cover their contracts.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_manifest_verify")
if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    # This helper carries the upload board marker. Never retain it in an
    # unconfigured diagnostic image, which must remain ineligible for upload.
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_image_create" "-Wl,--undefined=dsr_ota_image_update" "-Wl,--undefined=dsr_ota_image_finish" "-Wl,--undefined=dsr_ota_image_context_size")
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_fetch_manifest" "-Wl,--undefined=dsr_ota_fetch_app")
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_release_candidate_parse")
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_fetch_release")
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=dsr_ota_fetch_release_app")
endif()
target_compile_options(${COMPONENT_LIB} PRIVATE -fstack-usage)
