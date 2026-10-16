#pragma once
#include "dsr_storage_progress.h"
void dsr_ui_init(void);
/* Register with dsr_storage_set_observer() before storage starts (#28). */
const dsr_storage_observer_t *dsr_ui_storage_observer(void);
