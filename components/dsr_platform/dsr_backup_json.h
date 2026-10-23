#ifndef DSR_BACKUP_JSON_H
#define DSR_BACKUP_JSON_H
#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
bool dsr_backup_json_keys(const cJSON *object,const char *const *fields,size_t count);
bool dsr_backup_json_number(const cJSON *object,const char *field,uint32_t max,uint32_t *out);
bool dsr_backup_json_string(const cJSON *object,const char *field,char *out,size_t size,bool hex);
bool dsr_backup_json_add_string(cJSON *object,const char *field,const char *value,size_t capacity,bool hex);
bool dsr_backup_json_replace(cJSON *object,const char *field,cJSON *owned);
#endif
