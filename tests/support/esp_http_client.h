#ifndef TEST_ESP_HTTP_CLIENT_H
#define TEST_ESP_HTTP_CLIENT_H
#include "esp_err.h"
#include <stdbool.h>
typedef enum {HTTP_EVENT_HEADERS_SENT, HTTP_EVENT_ON_DATA} esp_http_client_event_id_t;
typedef struct {esp_http_client_event_id_t event_id; void *user_data; void *data; int data_len;} esp_http_client_event_t;
typedef enum {HTTP_AUTH_TYPE_NONE, HTTP_AUTH_TYPE_BASIC, HTTP_AUTH_TYPE_DIGEST} esp_http_client_auth_type_t;
typedef enum {HTTP_METHOD_GET, HTTP_METHOD_POST} esp_http_client_method_t;
typedef struct {
    const char *url; int timeout_ms;
    esp_err_t (*event_handler)(esp_http_client_event_t *);
    void *user_data; bool disable_auto_redirect;
    const char *username, *password;
    esp_http_client_auth_type_t auth_type;
    int max_authorization_retries;
} esp_http_client_config_t;
typedef struct test_client *esp_http_client_handle_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, esp_http_client_method_t method);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value);
esp_err_t esp_http_client_set_post_field(esp_http_client_handle_t c, const char *body, int length);
esp_err_t esp_http_client_perform(esp_http_client_handle_t c);
int esp_http_client_get_status_code(esp_http_client_handle_t c);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c);
/* Reuse of one client between requests (IDF 5.5.4 public API). */
esp_err_t esp_http_client_set_url(esp_http_client_handle_t c, const char *url);
esp_err_t esp_http_client_set_user_data(esp_http_client_handle_t c, void *data);
esp_err_t esp_http_client_set_timeout_ms(esp_http_client_handle_t c, int timeout_ms);
esp_err_t esp_http_client_set_authtype(esp_http_client_handle_t c, esp_http_client_auth_type_t auth_type);
esp_err_t esp_http_client_delete_header(esp_http_client_handle_t c, const char *key);
esp_err_t esp_http_client_prepare(esp_http_client_handle_t c);
#endif
