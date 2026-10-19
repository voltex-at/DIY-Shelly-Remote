#ifndef DSR_TEST_HTTP_CLIENT_H
#define DSR_TEST_HTTP_CLIENT_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#define ESP_ERR_HTTP_EAGAIN 0x7007 /* ESP-IDF v5.5.4 public constant */
typedef void *esp_http_client_handle_t;
typedef enum {HTTP_EVENT_ERROR,HTTP_EVENT_ON_CONNECTED,HTTP_EVENT_HEADERS_SENT,HTTP_EVENT_ON_HEADER,HTTP_EVENT_ON_DATA,HTTP_EVENT_ON_FINISH,HTTP_EVENT_DISCONNECTED,HTTP_EVENT_REDIRECT} esp_http_client_event_id_t;
typedef struct {esp_http_client_event_id_t event_id;esp_http_client_handle_t client;void *data;int data_len;void *user_data;char *header_key,*header_value;} esp_http_client_event_t;
typedef esp_err_t (*http_event_handle_cb)(esp_http_client_event_t *event);
typedef enum {HTTP_METHOD_GET} esp_http_client_method_t;
typedef enum {HTTP_TRANSPORT_UNKNOWN,HTTP_TRANSPORT_OVER_TCP,HTTP_TRANSPORT_OVER_SSL} esp_http_client_transport_t;
typedef struct {
 const char *url,*username,*password,*user_agent,*cert_pem,*common_name;
 const char *client_cert_pem,*client_key_pem,*client_key_password;
 int client_key_password_len,auth_type;
 bool use_global_ca_store,keep_alive_enable;
 esp_http_client_method_t method;esp_http_client_transport_t transport_type;
 esp_err_t (*crt_bundle_attach)(void *configuration);
 bool skip_cert_common_name_check,disable_auto_redirect;
 int max_authorization_retries,timeout_ms,buffer_size,buffer_size_tx;
 http_event_handle_cb event_handler;void *user_data;
} esp_http_client_config_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t client,const char *key,const char *value);
esp_err_t esp_http_client_open(esp_http_client_handle_t client,int write_length);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
int64_t esp_http_client_get_content_length(esp_http_client_handle_t client);
bool esp_http_client_is_chunked_response(esp_http_client_handle_t client);
int esp_http_client_read(esp_http_client_handle_t client,char *out,int length);
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client);
esp_err_t esp_http_client_close(esp_http_client_handle_t client);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
#endif
