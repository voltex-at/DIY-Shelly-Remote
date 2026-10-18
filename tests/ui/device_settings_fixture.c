#include "dsr_device_settings.h"
/* UI suite links the real editor/model/actor; SDK HTTP is a separate suite. */
dsr_result_t dsr_http_settings_identity(const dsr_connection_t *c){(void)c;return DSR_OK;}
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *c,const char *method,const cJSON *p,cJSON **out,bool write,bool *attempted)
{(void)c;(void)method;(void)p;(void)write;if(attempted)*attempted=false;*out=NULL;return DSR_OFFLINE;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t guard){(void)guard;return NULL;}

bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return c&&c->layout_id!=0;}
