#ifndef DSR_AP_H
#define DSR_AP_H
#include <stdbool.h>
#include <stdint.h>
#include "dsr_core.h"
typedef struct {char id[64],mac[13],model[40],ssid[33];uint8_t generation;bool auth_required;} dsr_ap_identity_t;
/* Only factory/open APs with a matching public API identity are supported.
 * SSID/identity comparisons are consistency checks, not cryptographic trust. */
/* Shared identity guard for a previously inspected Wi-Fi Shelly. */
bool dsr_station_identity_valid(const dsr_ap_identity_t *identity);
bool dsr_ap_candidate(const char *ssid);
bool dsr_ap_identity_matches(const char *ssid,const dsr_ap_identity_t *identity);
void dsr_ap_cancel(void);
void dsr_ap_prepare(void);
bool dsr_ap_cancelled(void);
bool dsr_ap_sensitive(void);
void dsr_ap_visibility(bool visible);
#endif
