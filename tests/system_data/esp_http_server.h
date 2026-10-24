#ifndef DSR_TEST_MAINT_HTTP_H
#define DSR_TEST_MAINT_HTTP_H
#include "esp_err.h"
#include <stddef.h>
enum {HTTP_GET,HTTP_POST,HTTP_PUT,HTTP_DELETE};
typedef struct {const char *uri;int method;size_t content_len;} httpd_req_t;
#define HTTPD_SOCK_ERR_TIMEOUT -3
int httpd_req_recv(httpd_req_t *,char *,size_t);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *,const char *,char *,size_t);
size_t httpd_req_get_hdr_value_len(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_status(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_type(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *,const char *,const char *);
esp_err_t httpd_resp_sendstr(httpd_req_t *,const char *);
esp_err_t httpd_resp_send_chunk(httpd_req_t *,const char *,size_t);
#endif
