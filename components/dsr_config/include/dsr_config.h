#ifndef DSR_CONFIG_H
#define DSR_CONFIG_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DSR_CONFIG_SCHEMA 2
#define DSR_CONFIG_MAX_JSON 24576
#define DSR_CONFIG_MAX_DASHBOARDS 4
#define DSR_CONFIG_MAX_TILES 24
/* Configuration contains no passwords and no fabricated telemetry. */
char *dsr_config_default(void);
bool dsr_config_validate(const cJSON *root, char error[96]);
/* Frozen strict schema1 and separate strict schema2 paths. Only stored legacy
 * documents may use1; all new explicit saves require canonical2. */
bool dsr_config_validate_stored(const cJSON *root,char error[96]);
/* RAM-only, no revision increment, no write. Caller owns canonical2 or NULL. */
char *dsr_config_upgrade(const char *json,bool *legacy,char error[96]);
/* CAS revision is mandatory. Caller owns returned canonical JSON. */
char *dsr_config_replace(const char *current, const char *candidate,
                         uint32_t expected_revision, char error[96]);
uint32_t dsr_config_revision(const char *json);
/* Two independent checksummed checkpoints; never overwrite the newest valid
 * record. No format/erase on corruption. Paths are fixed, not user supplied. */
char *dsr_config_store_load(const char *directory, bool *found, char error[96]);
bool dsr_config_store_save(const char *directory, const char *json, char error[96]);
/* Boot only, before any LVGL photo pointers exist. Keep the union of both
 * checkpoints. Any present invalid/future checkpoint disables deletion. */
bool dsr_config_store_collect_photos(const char *directory,unsigned *removed,char error[96]);
uint32_t dsr_config_crc32(const void *data, size_t size);
bool dsr_config_json_depth(const char *json);
#endif
