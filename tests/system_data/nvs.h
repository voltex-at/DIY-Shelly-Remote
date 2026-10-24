#ifndef DSR_SYSTEM_DATA_NVS_H
#define DSR_SYSTEM_DATA_NVS_H
#include "../support/nvs.h"
typedef enum {NVS_TYPE_ANY=0xff,NVS_TYPE_BLOB=0x42} nvs_type_t;
typedef struct nvs_iterator *nvs_iterator_t;
typedef struct {char namespace_name[16];char key[16];nvs_type_t type;} nvs_entry_info_t;
esp_err_t nvs_entry_find(const char *partition,const char *ns,nvs_type_t type,nvs_iterator_t *out);
esp_err_t nvs_entry_next(nvs_iterator_t *it);
void nvs_entry_info(nvs_iterator_t it,nvs_entry_info_t *out);
void nvs_release_iterator(nvs_iterator_t it);
esp_err_t nvs_erase_all(nvs_handle_t h);
esp_err_t nvs_erase_key(nvs_handle_t h,const char *key);
#endif
