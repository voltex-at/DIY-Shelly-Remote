#ifndef DSR_STYLE_HINT_H
#define DSR_STYLE_HINT_H
#include <stdint.h>
/* Last applied UI style kept in NVS so the boot splash can match it before the
 * SPIFFS workspace is mounted. Stable ids: 0 graphite, 2 light, 3 Cyber Blue.
 * Id 1 was the retired "midnight" skin (removed in 0.7.68-dev): it reads as
 * graphite and is rewritten once by the next applied style. Only a hint: the
 * workspace configuration remains the source of truth. */
#define DSR_STYLE_HINT_RETIRED 1u
uint8_t dsr_style_hint_get(void);
void dsr_style_hint_set(uint8_t style);
#endif
