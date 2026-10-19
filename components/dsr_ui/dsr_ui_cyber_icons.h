#ifndef DSR_UI_CYBER_ICONS_H
#define DSR_UI_CYBER_ICONS_H
#include "dsr_ui_style.h"

/* Borrowed const Flash descriptors; NULL keeps an existing specialist glyph
 * whose geometry is absent from the supplied pack (e.g. radiator valve). */
const lv_image_dsc_t *dsr_ui_cyber_icon(dsr_ui_icon_t kind,unsigned size);
#endif
