#ifndef DSR_AUTOMATION_CONFIG_H
#define DSR_AUTOMATION_CONFIG_H
#include "dsr_automation_types.h"
#include "cJSON.h"
/* Public IDs/roles only. Caller must resolve and verify private identities. */
bool dsr_auto_rules_parse(const cJSON *root,dsr_auto_rules_t *out,char error[96]);
bool dsr_auto_rule_same(const dsr_auto_rules_t *a,const dsr_auto_rule_t *ar,
 const dsr_auto_rules_t *b,const dsr_auto_rule_t *br);
#endif
