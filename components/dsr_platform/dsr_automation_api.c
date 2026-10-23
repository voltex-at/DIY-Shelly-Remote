#include "dsr_automation.h"
#include "dsr_workspace.h"
#include <stdlib.h>

static cJSON *row(cJSON *array)
{cJSON *v=cJSON_CreateObject();if(!v||!cJSON_AddItemToArray(array,v)){cJSON_Delete(v);return NULL;}return v;}
char *dsr_automation_status_json(void)
{
 dsr_auto_snapshot_t s;dsr_automation_snapshot(&s);uint32_t revision=dsr_workspace_revision();
 if(!revision||s.revision!=revision||s.count>DSR_AUTO_RULES||s.target_count>DSR_AUTO_PARTICIPANTS)return NULL;
 dsr_runtime_device_t *devices=calloc(DSR_MAX_DEVICES,sizeof(*devices));if(!devices)return NULL;
 size_t count=dsr_workspace_runtime_snapshot(devices,DSR_MAX_DEVICES);
 cJSON *root=cJSON_CreateObject(),*rules=root?cJSON_AddArrayToObject(root,"rules"):NULL,
  *targets=root?cJSON_AddArrayToObject(root,"targets"):NULL,*eligible=root?cJSON_AddArrayToObject(root,"devices"):NULL;
 bool ok=root&&rules&&targets&&eligible&&count<=DSR_MAX_DEVICES&&cJSON_AddNumberToObject(root,"revision",revision);
 for(unsigned i=0;ok&&i<s.count;i++){
  const dsr_auto_rule_status_t *r=&s.rules[i];cJSON *v=row(rules);
  ok=v&&cJSON_AddNumberToObject(v,"id",r->rule_id)&&cJSON_AddNumberToObject(v,"state",r->state)&&
   cJSON_AddNumberToObject(v,"result",r->result)&&cJSON_AddNumberToObject(v,"cycle",r->cycle)&&cJSON_AddNumberToObject(v,"missing_id",r->missing_id);
 }
 for(unsigned i=0;ok&&i<s.target_count;i++){
  cJSON *v=row(targets);ok=v&&cJSON_AddNumberToObject(v,"id",s.targets[i].layout_id)&&
   cJSON_AddNumberToObject(v,"baseline_half",s.targets[i].baseline)&&cJSON_AddNumberToObject(v,"requested_half",s.targets[i].requested)&&
   cJSON_AddNumberToObject(v,"read_half",s.targets[i].read_half)&&cJSON_AddNumberToObject(v,"phase",s.targets[i].phase)&&cJSON_AddBoolToObject(v,"overridden",s.targets[i].overridden);
 }
 for(size_t i=0;ok&&i<count;i++){
  const dsr_runtime_device_t *r=&devices[i];const dsr_blu_status_t *b=&r->telemetry.blu;
  if(!r->layout_id||!r->device.device_id[0]||r->device.generation!=DSR_GEN2_PLUS||r->device.endpoint_kind!=DSR_ENDPOINT_BLU||!(b->flags&DSR_BLU_VALID))continue;
  unsigned role=(b->errors&DSR_BLU_TRV)?DSR_AUTO_TRV:(b->flags&DSR_BLU_HAS_OPEN)?DSR_AUTO_CONTACT:0;if(!role)continue;
  cJSON *v=row(eligible);ok=v&&cJSON_AddNumberToObject(v,"id",r->layout_id)&&cJSON_AddNumberToObject(v,"role",role);
 }
 char *out=ok&&dsr_workspace_revision()==revision?cJSON_PrintUnformatted(root):NULL;
 cJSON_Delete(root);free(devices);return out;
}
