#ifndef DSR_CONFIG_RESTORE_H
#define DSR_CONFIG_RESTORE_H
#include "dsr_config.h"
/* Private maintenance adapters. Sole boot owner before workspace/photo GC;
 * do not use for normal CAS saves. Selection happens before import commit and
 * is persisted with that intent, so retries always write the same revision. */
bool dsr_config_store_restore_revision(const char *directory,uint32_t imported,uint32_t *out);
bool dsr_config_store_restore_slot(const char *directory,unsigned slot,const char *json);
#endif
