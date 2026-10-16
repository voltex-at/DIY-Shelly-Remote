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
    DSR_OFFLINE, DSR_BAD_RESPONSE, DSR_IO, DSR_BUSY, DSR_IDENTITY_MISMATCH, DSR_RATE_LIMITED,
    DSR_CONFLICT
} dsr_result_t;

typedef enum { DSR_GEN1 = 1, DSR_GEN2_PLUS = 2 } dsr_generation_t;
typedef enum { DSR_ENDPOINT_SWITCH=0, DSR_ENDPOINT_COVER=1, DSR_ENDPOINT_LIGHT=2 } dsr_endpoint_kind_t;
typedef enum {DSR_COVER_STATE_UNKNOWN,DSR_COVER_STATE_OPEN,DSR_COVER_STATE_CLOSED,DSR_COVER_STATE_OPENING,DSR_COVER_STATE_CLOSING,DSR_COVER_STATE_STOPPED,DSR_COVER_STATE_CALIBRATING} dsr_cover_state_t;
enum {DSR_COVER_VALID=1,DSR_COVER_POSITION=2,DSR_COVER_TARGET=4,DSR_COVER_POSITION_CONTROL=8};
typedef struct {uint8_t state,current_pos,target_pos,flags,errors;} dsr_cover_status_t;
typedef enum {DSR_COVER_NONE,DSR_COVER_OPEN,DSR_COVER_CLOSE,DSR_COVER_STOP,DSR_COVER_GOTO} dsr_cover_command_t;
typedef struct {uint8_t command,position;} dsr_cover_intent_t;
enum {DSR_LIGHT_VALID=1,DSR_LIGHT_BRIGHTNESS=2,DSR_LIGHT_TARGET=4,DSR_LIGHT_TRANSITION=8,DSR_LIGHT_TARGET_OUTPUT=16,DSR_LIGHT_TARGET_ON=32};
enum {DSR_LIGHT_CALIBRATING=1,DSR_LIGHT_UNCALIBRATED=2,DSR_LIGHT_NO_LOAD=4,DSR_LIGHT_UNKNOWN_CONDITION=128};
typedef struct {uint8_t brightness,target,flags,errors,conditions;} dsr_light_status_t;
typedef enum {DSR_LIGHT_NONE,DSR_LIGHT_ON,DSR_LIGHT_OFF,DSR_LIGHT_SET_BRIGHTNESS} dsr_light_command_t;
typedef struct {uint8_t command,brightness;} dsr_light_intent_t;
typedef struct {
    char host[DSR_HOST_SIZE];
    char name[DSR_NAME_SIZE];
    char device_id[64];
    dsr_generation_t generation;
    uint8_t channel;
    bool online;
    bool has_switch;
    bool has_power;
    bool output;
    uint8_t endpoint_kind;
    double power_w;
} dsr_device_t;
/* Installed raw records use this ABI. Persistent legacy types are frozen
 * separately; their padding is never interpreted as an endpoint kind. */
_Static_assert(sizeof(dsr_generation_t)==4&&sizeof(bool)==1,"installed scalar ABI");
_Static_assert(sizeof(dsr_device_t)==152&&offsetof(dsr_device_t,generation)==128&&offsetof(dsr_device_t,channel)==132&&offsetof(dsr_device_t,output)==136&&offsetof(dsr_device_t,endpoint_kind)==137&&offsetof(dsr_device_t,power_w)==144,"installed device ABI");
bool dsr_cover_intent_valid(dsr_cover_intent_t intent);
bool dsr_cover_can_move(const dsr_cover_status_t *status,dsr_cover_intent_t intent);
bool dsr_light_intent_valid(dsr_light_intent_t intent);
bool dsr_light_can_set(const dsr_light_status_t *status,dsr_light_intent_t intent);
/* Workspace kind is a transport family, never a hint to alias another API. */
uint8_t dsr_endpoint_from_kind(const char *kind);
const char *dsr_endpoint_kind_name(uint8_t kind);

typedef struct { dsr_device_t items[DSR_MAX_DEVICES]; size_t count; } dsr_registry_t;
typedef enum { DSR_GET_STATUS, DSR_SET_OUTPUT } dsr_command_t;
typedef struct {
    char path[96];
    char body[64];
    bool post;
} dsr_request_t;
dsr_result_t dsr_cover_request_build(uint8_t channel,dsr_cover_intent_t intent,dsr_request_t *out);
dsr_result_t dsr_light_request_build(uint8_t channel,dsr_light_intent_t intent,dsr_request_t *out);

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
 * download an image or authorize writing Flash. The separate local installer
 * is implemented by dsr_platform/dsr_panel.c and requires local approval. */
dsr_result_t dsr_update_metadata_check(const dsr_update_metadata_t *meta,
                                     const char *hardware_id, const char *current,
                                     size_t inactive_slot_size);
#endif
