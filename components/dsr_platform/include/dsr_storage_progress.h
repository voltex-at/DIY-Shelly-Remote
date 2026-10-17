#ifndef DSR_STORAGE_PROGRESS_H
#define DSR_STORAGE_PROGRESS_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Optional first-format observer (#28). begin/end run on the caller of
 * dsr_storage_mount() (app_main), strictly around the single synchronous
 * format pass and never for an existing, refused or unreadable filesystem.
 * begin() returns before the format starts; end() must return only after
 * every other task has stopped calling dsr_storage_format_progress(). */
typedef struct {
    void (*begin)(bool resume, uint32_t total_blocks, uint32_t remnant_blocks);
    void (*end)(esp_err_t result);
} dsr_storage_observer_t;

/* Set once by app_main before storage starts; NULL disables the hooks. */
void dsr_storage_set_observer(const dsr_storage_observer_t *observer);

/* Between begin and end only (else 0): number of leading blocks that already
 * carry the exact formatter header, read from Flash (<=11 x 4-byte reads).
 * During a resumed format the old remnant prefix is still present, so values
 * <= remnant_blocks do not yet prove new progress. Safe from another task. */
uint32_t dsr_storage_format_progress(void);
#endif
