#include "dsr_backup_json.h"
#include "dsr_backup_format.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
bool dsr_backup_json_keys(const cJSON *o,const char *const *fields,size_t count)
{
 if(!cJSON_IsObject(o)||(size_t)cJSON_GetArraySize(o)!=count)return false;
 for(size_t i=0;i<count;i++)if(!cJSON_GetObjectItemCaseSensitive(o,fields[i]))return false;
 return true;
}
bool dsr_backup_json_number(const cJSON *o,const char *field,uint32_t max,uint32_t *out)
{
 const cJSON *v=cJSON_GetObjectItemCaseSensitive(o,field);
 if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<0||v->valuedouble>max||floor(v->valuedouble)!=v->valuedouble)return false;
 if(out)*out=(uint32_t)v->valuedouble;
 return true;
}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
bool dsr_backup_json_string(const cJSON *o,const char *field,char *out,size_t size,bool hex)
{
 const cJSON *v=cJSON_GetObjectItemCaseSensitive(o,field);if(!out||!size||!cJSON_IsString(v))return false;
 size_t n=strlen(v->valuestring);if(hex&&n%2)return false;size_t bytes=hex?n/2:n;if(bytes>=size)return false;
 if(hex){for(size_t i=0;i<bytes;i++){int a=digit(v->valuestring[2*i]),b=digit(v->valuestring[2*i+1]);if(a<0||b<0||a*16+b==0)return false;out[i]=(char)(a*16+b);}}
 else memcpy(out,v->valuestring,bytes);
 out[bytes]=0;return true;
}
bool dsr_backup_json_add_string(cJSON *o,const char *field,const char *value,size_t capacity,bool hex)
{
 if(!value||!memchr(value,0,capacity))return false;
 if(!hex)return cJSON_AddStringToObject(o,field,value)!=NULL;
 size_t size=strlen(value);if(size>64)return false;char encoded[129];static const char digits[]="0123456789abcdef";
 for(size_t i=0;i<size;i++){encoded[2*i]=digits[(uint8_t)value[i]>>4];encoded[2*i+1]=digits[(uint8_t)value[i]&15];}encoded[2*size]=0;
 bool ok=cJSON_AddStringToObject(o,field,encoded)!=NULL;dsr_backup_wipe(encoded,sizeof(encoded));return ok;
}
bool dsr_backup_json_replace(cJSON *o,const char *field,cJSON *owned)
{
 if(!owned)return false;
 bool ok=cJSON_GetObjectItemCaseSensitive(o,field)?cJSON_ReplaceItemInObjectCaseSensitive(o,field,owned):cJSON_AddItemToObject(o,field,owned);
 if(!ok)cJSON_Delete(owned);
 return ok;
}
