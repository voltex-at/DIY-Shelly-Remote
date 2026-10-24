#ifndef DSR_BACKUP_BONDS_SDK_H
#define DSR_BACKUP_BONDS_SDK_H
#include "dsr_backup_bonds_model.h"
#include <stddef.h>
/* Called only by the store owner or boot recovery. Buffer contains one SDK NVS
 * value, never copied to an archive; every field is mapped explicitly. */
dsr_result_t dsr_backup_bond_from_sdk(const char *key,const void *value,size_t size,dsr_backup_bond_t *out);
dsr_result_t dsr_backup_bond_to_sdk(const dsr_backup_bond_t *,char key[16],void *out,size_t capacity,size_t *size);
#endif
