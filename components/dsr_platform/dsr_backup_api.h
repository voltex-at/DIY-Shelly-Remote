#ifndef DSR_BACKUP_API_H
#define DSR_BACKUP_API_H
#include "esp_http_server.h"
#include <stdbool.h>
#include <stdint.h>
/* Called only after unchanged local-request/boot/cookie gates. Session hash is
 * derived by the trusted handler, never accepted in JSON or query parameters. */
esp_err_t dsr_backup_api(httpd_req_t *,const uint8_t authenticated_session[32]);
bool dsr_panel_backup_session_valid(const uint8_t session[32]);
/* One bounded HTTPS transfer across backup and local OTA. Takes context
 * ownership on every outcome; cleanup runs exactly once. URI admission and
 * authorization stay with the caller. No RPC executes on this transport. */
esp_err_t dsr_panel_transport_start(httpd_req_t *,void (*run)(httpd_req_t *,void *),
 void *context,void (*cleanup)(void *));
#endif
