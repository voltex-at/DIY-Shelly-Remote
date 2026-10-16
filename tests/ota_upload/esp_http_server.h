#ifndef TEST_OTA_HTTP_H
#define TEST_OTA_HTTP_H
#include <stddef.h>
#include "esp_err.h"
typedef struct {size_t content_len;} httpd_req_t;
#define HTTPD_SOCK_ERR_TIMEOUT -3
int httpd_req_recv(httpd_req_t *request,char *buffer,size_t length);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *request,const char *name,char *out,size_t capacity);
#endif
