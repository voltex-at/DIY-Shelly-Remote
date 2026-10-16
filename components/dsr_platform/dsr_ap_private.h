#ifndef DSR_AP_PRIVATE_H
#define DSR_AP_PRIVATE_H
#include "dsr_platform.h"
dsr_result_t dsr_http_ap_inspect(const char *ssid,dsr_ap_identity_t *out);
dsr_result_t dsr_http_ap_provision(const dsr_ap_identity_t *expected,const dsr_settings_t *home);
dsr_result_t dsr_http_ap_verify_lan(const dsr_ap_identity_t *expected,const char *host,const dsr_settings_t *home);
dsr_result_t dsr_http_lan_verify_station(const dsr_ap_identity_t *expected,const char *host,const dsr_settings_t *home);
/* Worker-only orchestration. Event data never contains home credentials. */
void dsr_ap_execute(const dsr_job_t *job,dsr_event_t *event);
#endif
