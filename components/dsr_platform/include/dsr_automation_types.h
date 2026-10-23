#ifndef DSR_AUTOMATION_TYPES_H
#define DSR_AUTOMATION_TYPES_H
#include "dsr_core.h"

#define DSR_AUTO_RULES 8u
#define DSR_AUTO_PARTICIPANTS 16u
#define DSR_AUTO_RECORD_MAX 3072u
#define DSR_AUTO_CONTACT_AGE_S 120u
#define DSR_AUTO_FRESH_MS 45000u
#define DSR_AUTO_READ_INTERVAL_MS 15000u
enum { DSR_AUTO_CONTACT=1, DSR_AUTO_TRV=2 };
/* Private enrollment. No credentials; never serialize through the public API. */
typedef struct {
 uint32_t layout_id;
 char device_id[64],host[16];
 uint8_t generation,channel,kind,reserved;
} dsr_auto_key_t;
typedef struct {
 uint32_t id,signature;
 uint16_t sensors,targets;
 uint8_t target_half,enabled;
 uint16_t reserved;
} dsr_auto_rule_t;
typedef struct {
 dsr_auto_key_t keys[DSR_AUTO_PARTICIPANTS];
 dsr_auto_rule_t items[DSR_AUTO_RULES];
 uint8_t key_count,count;
 /* Trusted explicit save only. Not part of the durable wire encoding. */
 bool enroll;
} dsr_auto_rules_t;
typedef struct {
 dsr_auto_key_t key;
 uint32_t sample_ms,source_utc,read_id,started_ms;
 double target_c;
 bool valid,open;
} dsr_auto_observation_t;
typedef enum {DSR_AUTO_EFFECT_NONE,DSR_AUTO_EFFECT_JOURNAL,
 DSR_AUTO_EFFECT_READ_TRV,DSR_AUTO_EFFECT_SET_TRV} dsr_auto_effect_kind_t;
typedef struct {
 uint32_t id,generation,rule_id,cycle,intent_id,layout_id;
 uint8_t kind,target_half;
} dsr_auto_effect_t;
enum {DSR_AUTO_IDLE,DSR_AUTO_PREPARED,DSR_AUTO_VERIFY,DSR_AUTO_OWNED,
 DSR_AUTO_RESTORE_PREPARED,DSR_AUTO_RESTORE_VERIFY,DSR_AUTO_DONE,DSR_AUTO_UNCERTAIN};
typedef struct {
 uint32_t manual_intent;
 uint8_t baseline,desired,phase,override;
} dsr_auto_target_t;
typedef struct {uint32_t cycle;uint8_t active;uint8_t reserved[3];} dsr_auto_cycle_t;
typedef struct {
 dsr_auto_rules_t rules;
 dsr_auto_cycle_t cycles[DSR_AUTO_RULES];
 dsr_auto_target_t targets[DSR_AUTO_PARTICIPANTS];
 uint32_t generation;
} dsr_auto_record_t;
_Static_assert(sizeof(dsr_auto_record_t)<DSR_AUTO_RECORD_MAX,"bounded recovery record");
enum {DSR_AUTO_DISABLED,DSR_AUTO_WAITING,DSR_AUTO_ACTIVE,DSR_AUTO_MANUAL,
 DSR_AUTO_CONFIRMING,DSR_AUTO_ERROR};
typedef struct {
 uint32_t rule_id,cycle,missing_id;
 uint8_t state;
 dsr_result_t result;
 uint16_t overridden;
} dsr_auto_rule_status_t;
typedef struct {
 dsr_auto_rule_status_t rules[DSR_AUTO_RULES];
 struct {uint32_t layout_id;uint8_t baseline,requested,read_half,phase;} targets[DSR_AUTO_PARTICIPANTS];
 uint8_t count,target_count;
} dsr_auto_snapshot_t;
#endif
