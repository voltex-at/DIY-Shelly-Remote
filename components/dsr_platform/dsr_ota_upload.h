#ifndef DSR_OTA_UPLOAD_H
#define DSR_OTA_UPLOAD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_http_server.h"
#include "esp_ota_ops.h"
/* Private legacy-development receiver. No signature/trust decision here.
 * Caller retains local/cookie/boot-ready gates, one-shot physical consent,
 * inactive slot selection, shared staging reservation and HTTP responses.
 * Never selects boot or reboots. */
esp_err_t dsr_ota_upload_receive(httpd_req_t *request,const esp_partition_t *slot,
 const uint8_t expected[32]);
/* Preserve prior successful-upload contract: exactly 64 lowercase hex bytes.
 * Uppercase previously failed final comparison; now fails BEFORE Flash. */
bool dsr_ota_upload_sha256(const char *text,size_t length,uint8_t out[32]);
#endif
