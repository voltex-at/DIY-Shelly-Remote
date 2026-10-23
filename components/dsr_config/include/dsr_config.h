#ifndef DSR_CONFIG_H
#define DSR_CONFIG_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DSR_CONFIG_SCHEMA 3
#define DSR_CONFIG_MAX_JSON 24576
#define DSR_CONFIG_MAX_DASHBOARDS 4
#define DSR_CONFIG_MAX_TILES 24
/* Optional device "icon" (0.7.51): 0 = by type, 1..20 = LCD icon gallery.
 * Written only when non-zero, so unchanged configs stay valid for 0.7.50. */
#define DSR_CONFIG_DEVICE_ICONS 20
/* Configuration contains no passwords and no fabricated telemetry. */
char *dsr_config_default(void);
bool dsr_config_validate(const cJSON *root, char error[96]);
/* Return a validated independent draft, removing only this stable card ID and
 * its tile/member/action references. Rooms, scenes and sibling channels stay.
 * Automation references must be resolved explicitly before deletion.
 * No write or revision change; the caller must use the normal durable CAS. */
cJSON *dsr_config_remove_device(const cJSON *root,uint32_t id,char error[96]);
/* Frozen strict schema1/2 paths; current saves require canonical3. */
bool dsr_config_validate_stored(const cJSON *root,char error[96]);
/* RAM-only, no revision increment, no write. Caller owns canonical3 or NULL.
 * Also migrates the retired "midnight" theme to "graphite". */
char *dsr_config_upgrade(const char *json,bool *legacy,char error[96]);
/* Exactly three skins remain: graphite, light, cyber_blue. Replaces a retired
 * "midnight" theme in place by "graphite"; true only when it changed. Stored
 * documents may still hold it (validate_stored); dsr_config_validate never. */
bool dsr_config_retire_theme(cJSON *root);
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
