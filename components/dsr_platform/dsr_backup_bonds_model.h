#ifndef DSR_BACKUP_BONDS_MODEL_H
#define DSR_BACKUP_BONDS_MODEL_H
#include "dsr_core.h"
#include "cJSON.h"
#include <stdint.h>

/* Logical fields, never an on-disk or NimBLE ABI structure. */
enum {
 DSR_BACKUP_BOND_OUR_SEC=1, DSR_BACKUP_BOND_PEER_SEC=2,
 DSR_BACKUP_BOND_CCCD=3, DSR_BACKUP_BOND_PEER_RECORD=4,
 DSR_BACKUP_BOND_CSFC=8, DSR_BACKUP_BOND_RPA=6,
 DSR_BACKUP_BOND_LOCAL_IRK=7
};
typedef struct { uint8_t type,bytes[6]; } dsr_backup_addr_t;
typedef struct {
 uint8_t family,index;
 union {
  struct { dsr_backup_addr_t peer; uint16_t bond_count; uint8_t key_size;
   uint16_t ediv; uint64_t rand_num; uint8_t ltk[16],irk[16],csrk[16];
   uint32_t sign_counter; uint8_t flags; } sec;
  struct { dsr_backup_addr_t peer; uint16_t handle,flags; uint8_t changed; } cccd;
  struct { dsr_backup_addr_t peer; uint8_t features; } csfc;
  struct { dsr_backup_addr_t addr; uint8_t key[16]; } local_irk;
  struct { dsr_backup_addr_t random,peer; } rpa;
  struct { uint8_t used,rand_addr_type,pseudo[6],random[6],identity[6];
   dsr_backup_addr_t peer; uint8_t irk[16],irk_present; } peer_record;
 } value;
} dsr_backup_bond_t;
dsr_result_t dsr_backup_bond_encode(const dsr_backup_bond_t *,cJSON **out);
dsr_result_t dsr_backup_bond_check(const dsr_backup_bond_t *);
dsr_result_t dsr_backup_bond_decode(const cJSON *,dsr_backup_bond_t *out);
dsr_result_t dsr_backup_bonds_validate(const cJSON *);
#endif
