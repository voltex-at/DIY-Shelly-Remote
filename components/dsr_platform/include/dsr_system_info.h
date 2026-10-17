#ifndef DSR_SYSTEM_INFO_H
#define DSR_SYSTEM_INFO_H
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    size_t internal_free;
    size_t internal_largest;
    size_t psram_free;
    /* Available configured Flash size, not unused filesystem space or a
     * separate physical-chip capacity probe. Zero when flash_error != OK. */
    uint32_t flash_bytes;
    esp_err_t flash_error;
} dsr_system_info_t;

/* Read-only, allocation-free diagnostics. No NVS, RF or Flash writes.
 * Heap counters are separate live reads, not an atomic accounting snapshot. */
void dsr_system_info_snapshot(dsr_system_info_t *out);
#endif
