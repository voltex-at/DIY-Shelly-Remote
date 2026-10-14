#ifndef DSR_CORE_H
#define DSR_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DSR_MAX_DEVICES 16
#define DSR_MAX_CHANNEL 15
#define DSR_HOST_SIZE 16
#define DSR_NAME_SIZE 48

typedef enum {
    DSR_OK = 0, DSR_INVALID, DSR_FULL, DSR_UNSUPPORTED, DSR_AUTH_REQUIRED,
    DSR_OFFLINE, DSR_BAD_RESPONSE, DSR_IO, DSR_BUSY
} dsr_result_t;

typedef enum { DSR_GEN1 = 1, DSR_GEN2_PLUS = 2 } dsr_generation_t;
typedef struct {
    char host[DSR_HOST_SIZE];
    char name[DSR_NAME_SIZE];
    dsr_generation_t generation;
    uint8_t channel;
    bool online;
    bool has_switch;
    bool has_power;
    bool output;
    double power_w;
} dsr_device_t;

typedef struct { dsr_device_t items[DSR_MAX_DEVICES]; size_t count; } dsr_registry_t;
typedef enum { DSR_GET_STATUS, DSR_SET_OUTPUT } dsr_command_t;
typedef struct {
    char path[96];
    char body[64];
    bool post;
} dsr_request_t;

bool dsr_ipv4_valid(const char *host);
dsr_result_t dsr_request_build(dsr_generation_t gen, uint8_t channel,
                             dsr_command_t cmd, bool on, dsr_request_t *out);
/* Registry is owned by one task; callers synchronize or copy snapshots. */
dsr_result_t dsr_registry_upsert(dsr_registry_t *registry, const dsr_device_t *device);
/* Stable releases only: optional 'v', exactly MAJOR.MINOR.PATCH, no suffix. */
bool dsr_version_parse(const char *text, uint32_t parts[3]);
bool dsr_version_is_newer(const char *current, const char *candidate);

typedef struct {
    const char *hardware_id;
    const char *version;
    const char *url;
    const char *sha256;
    size_t size;
} dsr_update_metadata_t;
/* Syntax/compatibility policy only. This does NOT authenticate a signature,
 * download an image or authorize writing Flash. Installer is not implemented. */
dsr_result_t dsr_update_metadata_check(const dsr_update_metadata_t *meta,
                                     const char *hardware_id, const char *current,
                                     size_t inactive_slot_size);
#endif
