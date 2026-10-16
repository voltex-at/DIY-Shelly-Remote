#ifndef DSR_OTA_STAGE_H
#define DSR_OTA_STAGE_H
/* Private linear ownership, shared by the native installer and legacy endpoint.
 * The opaque token must never be copied, retained after release, or reused.
 * Activity prevents restart; this separate reservation serializes OTA writers.
 * No application mutex is held across SDK I/O or shutdown. */
typedef struct dsr_ota_stage dsr_ota_stage_t;
typedef enum {DSR_OTA_STAGE_IDLE,DSR_OTA_STAGE_HELD,DSR_OTA_STAGE_STAGED,DSR_OTA_STAGE_UNCERTAIN} dsr_ota_stage_state_t;
dsr_ota_stage_t *dsr_ota_stage_acquire(void);
void dsr_ota_stage_release(dsr_ota_stage_t *stage);
void dsr_ota_stage_commit(dsr_ota_stage_t *stage);
/* Boot selection was attempted but SDK did not confirm success. Physical
 * otadata may have changed: block writers until reboot, release activity. */
void dsr_ota_stage_uncertain(dsr_ota_stage_t *stage);
dsr_ota_stage_state_t dsr_ota_stage_status(void);
#endif
