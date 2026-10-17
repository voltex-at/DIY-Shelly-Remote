#ifndef DSR_RUNTIME_API_H
#define DSR_RUNTIME_API_H
#include "dsr_workspace.h"
bool dsr_ht_refresh_parse(const char *json,dsr_device_t *device);
bool dsr_rgb_parse(const char *json,dsr_device_t *device,dsr_rgb_intent_t *intent);
bool dsr_light_parse(const char *json,dsr_device_t *device,dsr_light_intent_t *intent);
char *dsr_runtime_json(uint32_t boot_id);
bool dsr_output_parse(const char *json,dsr_device_t *device,bool *on);
bool dsr_cover_parse(const char *json,dsr_device_t *device,dsr_cover_intent_t *intent);
#endif
