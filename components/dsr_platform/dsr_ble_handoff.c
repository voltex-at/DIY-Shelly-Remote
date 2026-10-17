#include "dsr_ble.h"
#include "cJSON.h"
#include <math.h>
#include <string.h>
bool dsr_ble_handoff_parse(const char *json,uint32_t *serial,unsigned *index)
{
 if(!json||!serial||!index||strlen(json)>128||strstr(json,"\\u0000"))return false;
 cJSON *root=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(root)&&cJSON_GetArraySize(root)==2;
 const cJSON *a=cJSON_GetObjectItemCaseSensitive(root,"serial"),*b=cJSON_GetObjectItemCaseSensitive(root,"index");
 if(valid)for(const cJSON *v=root->child;v;v=v->next)if(strcmp(v->string,"serial")&&strcmp(v->string,"index"))valid=false;
 valid=valid&&cJSON_IsNumber(a)&&isfinite(a->valuedouble)&&a->valuedouble>=0&&a->valuedouble<=UINT32_MAX&&floor(a->valuedouble)==a->valuedouble&&cJSON_IsNumber(b)&&b->valuedouble>=0&&b->valuedouble<DSR_BLE_MAX_DEVICES&&floor(b->valuedouble)==b->valuedouble;
 if(valid){*serial=(uint32_t)a->valuedouble;*index=(unsigned)b->valuedouble;}
 cJSON_Delete(root);return valid;
}
