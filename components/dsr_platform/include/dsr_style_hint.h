#ifndef DSR_STYLE_HINT_H
#define DSR_STYLE_HINT_H
#include <stdint.h>
/* Last applied UI style (0 graphite, 1 midnight, 2 light) kept in NVS so the
 * boot splash can match it before the SPIFFS workspace is mounted. Only a
 * hint: the workspace configuration remains the source of truth. */
uint8_t dsr_style_hint_get(void);
void dsr_style_hint_set(uint8_t style);
#endif
