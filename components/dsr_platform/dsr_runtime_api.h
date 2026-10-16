#ifndef DSR_RUNTIME_API_H
#define DSR_RUNTIME_API_H
#include "dsr_workspace.h"
char *dsr_runtime_json(uint32_t boot_id);
bool dsr_output_parse(const char *json,dsr_device_t *device,bool *on);
#endif
