#ifndef DSR_PANEL_PAIRING_H
#define DSR_PANEL_PAIRING_H
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
 /* Effective current-boot has_token, not active clients/session count or
  * evidence of persistent key presence/absence. Startup NVS read may fail. */
 bool paired;
 /* A revoke started but persistent absence has not been verified. After an
  * error the old NVS key may remain OR already be erased; reboot is unknown. */
 bool revocation_unconfirmed;
} dsr_panel_pairing_snapshot_t;
/* Read-only/no secrets/no NVS. Zeroes out; false on NULL, incomplete startup,
 * unavailable panel or lock contention. Bounded trylock, safe for UI polling. */
bool dsr_panel_pairing_snapshot(dsr_panel_pairing_snapshot_t *out);
/* WORKER ONLY, after explicit physical confirmation; never call from LVGL.
 * Activity admission spans lock wait, RAM invalidation, NVS and close. Once
 * locked, deny the current credential and invalidate pairing/OTA windows before
 * any NVS operation. Erases ONLY dsr_panel/token_hash; NOT_FOUND is idempotent.
 * Success requires erase, commit and absent-key readback. NVS is not a rollback
 * transaction. Errors never restore the old RAM credential; unconfirmed stays
 * true until a verified revoke or genuinely committed subsequent pairing.
 * Startup/admission/lock errors before invalidation leave RAM unchanged.
 * Already authorized requests may finish; this does not abort active writes,
 * rotate TLS identity, wipe settings, reboot or add a remote revoke endpoint. */
esp_err_t dsr_panel_pairing_revoke(void);
#endif
