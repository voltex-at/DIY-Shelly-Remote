#ifndef DSR_BACKUP_API_H
#define DSR_BACKUP_API_H
#include "esp_http_server.h"
#include <stdbool.h>
#include <stdint.h>
/* Called only after unchanged local-request/boot/cookie gates. Session hash is
 * derived by the trusted handler, never accepted in JSON or query parameters. */
esp_err_t dsr_backup_api(httpd_req_t *,const uint8_t authenticated_session[32]);
bool dsr_panel_backup_session_valid(const uint8_t session[32]);
#endif
