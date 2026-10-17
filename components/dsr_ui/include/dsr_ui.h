#pragma once
#include "dsr_storage_progress.h"
void dsr_ui_init(void);
/* Register with dsr_storage_set_observer() before storage starts (#28). */
const dsr_storage_observer_t *dsr_ui_storage_observer(void);
/* Boot splash (owner artwork): begin right after dsr_board_init, end before
 * dsr_ui_init. Opens/closes the early UI gate; no-op stays dark on failure. */
void dsr_ui_splash_begin(void);
void dsr_ui_splash_end(void);
