#include "dsr_backup_format.h"
#include "dsr_config.h"
#include "mbedtls/sha256.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {HEADER=24,PHOTO_HEADER=56,MAX_DEPTH=20,MAX_NODES=4096,MAX_OBJECT_FIELDS=64};
struct dsr_backup_image {
 uint8_t *owned;size_t size;cJSON *manifest;unsigned count;
 dsr_backup_photo_t photos[DSR_BACKUP_MAX_PHOTOS];
};
void dsr_backup_wipe(void *data,size_t size)
{volatile uint8_t *p=data;while(p&&size--)*p++=0;}
static void scrub(cJSON *value)
{for(cJSON *v=value;v;v=v->next){if(v->valuestring)dsr_backup_wipe(v->valuestring,strlen(v->valuestring));if(v->string)dsr_backup_wipe(v->string,strlen(v->string));if(v->child)scrub(v->child);}}
static void discard(cJSON *value){scrub(value);cJSON_Delete(value);}
void dsr_backup_json_discard(cJSON *value){discard(value);}
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static const cJSON *get(const cJSON *v,const char *key){return cJSON_GetObjectItemCaseSensitive(v,key);}
static bool integer(const cJSON *v,double min,double max)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&v->valuedouble>=min&&v->valuedouble<=max&&floor(v->valuedouble)==v->valuedouble;}
static bool keys(const cJSON *v,const char *const *names,size_t count)
{
 if(!cJSON_IsObject(v)||(size_t)cJSON_GetArraySize(v)!=count)return false;
 for(size_t i=0;i<count;i++)if(!get(v,names[i]))return false;
 return true;
}
/* Bounds the object walk before duplicates can turn it into unbounded work. */
static bool tree(const cJSON *v,unsigned depth,unsigned *nodes)
{
 if(depth>MAX_DEPTH)return false;
 for(const cJSON *p=v;p;p=p->next){
  if(++*nodes>MAX_NODES)return false;
  if(cJSON_IsObject(p)){
   if(cJSON_GetArraySize(p)>MAX_OBJECT_FIELDS)return false;
   for(const cJSON *a=p->child;a;a=a->next){if(!a->string)return false;for(const cJSON *b=a->next;b;b=b->next)if(!b->string||!strcmp(a->string,b->string))return false;}
  }
  if(p->child&&!tree(p->child,depth+1,nodes))return false;
 }
 return true;
}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static bool unhex(const cJSON *v,uint8_t *out,size_t bytes)
{
 if(!cJSON_IsString(v)||strlen(v->valuestring)!=bytes*2)return false;
 for(size_t i=0;i<bytes;i++){int a=digit(v->valuestring[i*2]),b=digit(v->valuestring[i*2+1]);if(a<0||b<0)return false;if(out)out[i]=(uint8_t)(a*16+b);}
 return true;
}
static bool array_objects(const cJSON *v,unsigned max)
{if(!cJSON_IsArray(v)||(unsigned)cJSON_GetArraySize(v)>max)return false;const cJSON *p;cJSON_ArrayForEach(p,v)if(!cJSON_IsObject(p))return false;return true;}
/* Strict UTF-8 and JSON string controls, including escaped NUL. cJSON uses C
 * strings, so accepting U+0000 would silently change passwords and field keys. */
static bool json_bytes(const uint8_t *p,size_t size)
{
 bool quoted=false;unsigned depth=0,complexity=0;
 for(size_t i=0;i<size;i++){
  uint8_t c=p[i];if(!c)return false;
  if(c=='"'){quoted=!quoted;continue;}
  if(quoted&&c=='\\'){
   if(++i>=size)return false;
   if(p[i]=='u'){if(size-i<=4)return false;if(!memcmp(p+i+1,"0000",4))return false;i+=4;}
   continue;
  }
  if(!quoted){
   if(c=='{'||c=='['){if(++depth>MAX_DEPTH)return false;}
   else if(c=='}'||c==']'){if(!depth)return false;depth--;}
   if((c=='{'||c=='['||c==','||c==':')&&++complexity>MAX_NODES*2)return false;
  }
  if(c<32){if(quoted||(c!='\t'&&c!='\r'&&c!='\n'))return false;continue;}
  if(c<128)continue;
  unsigned n=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;
  if(!n||size-i<n)return false;
  uint32_t cp=c&((1u<<(7-n))-1u);
  for(unsigned j=1;j<n;j++){if((p[i+j]&0xc0)!=0x80)return false;cp=(cp<<6)|(p[i+j]&0x3f);}
  if((n==3&&cp<0x800)||(n==4&&cp<0x10000)||(cp>=0xd800&&cp<=0xdfff)||cp>0x10ffff)return false;
  i+=n-1;
 }
 return !quoted&&!depth;
}
dsr_result_t dsr_backup_manifest_validate(const cJSON *root,const uint8_t device_id[32])
{
 static const char *const fields[]={"format","hardware","device","firmware","created","config","settings","connections","battery","style","bonds","enrollment","photos"};
 unsigned nodes=0;if(!device_id||!tree(root,0,&nodes)||!keys(root,fields,sizeof(fields)/sizeof(fields[0])))return DSR_INVALID;
 if(!integer(get(root,"format"),1,1))return integer(get(root,"format"),2,65535)?DSR_UNSUPPORTED:DSR_INVALID;
 const cJSON *hw=get(root,"hardware"),*fw=get(root,"firmware");uint8_t identity[32];
 if(!cJSON_IsString(hw)||!unhex(get(root,"device"),identity,32))return DSR_INVALID;
 if(strcmp(hw->valuestring,DSR_BACKUP_HARDWARE)||memcmp(identity,device_id,32))return DSR_IDENTITY_MISMATCH;
 if(!cJSON_IsString(fw)||!fw->valuestring[0]||strlen(fw->valuestring)>63)return DSR_INVALID;
 if(!cJSON_IsNull(get(root,"created"))&&!integer(get(root,"created"),1735689600,UINT32_MAX))return DSR_INVALID;
 const cJSON *config=get(root,"config"),*schema=get(config,"schema");
 if(!integer(schema,1,3))return integer(schema,4,65535)?DSR_UNSUPPORTED:DSR_INVALID;
 char error[96];if(!dsr_config_validate_stored(config,error))return DSR_INVALID;
 char *serialized=cJSON_PrintUnformatted(config);if(!serialized)return DSR_IO;bool fits=strlen(serialized)<=DSR_CONFIG_MAX_JSON;free(serialized);if(!fits)return DSR_INVALID;
 if(!cJSON_IsObject(get(root,"settings"))||!cJSON_IsObject(get(root,"battery"))||!array_objects(get(root,"connections"),DSR_MAX_DEVICES)||!integer(get(root,"style"),0,3)||get(root,"style")->valueint==1)return DSR_INVALID;
 const cJSON *bonds=get(root,"bonds"),*enroll=get(root,"enrollment");
 static const char *const bond_fields[]={"version","records"},*const enroll_fields[]={"version","keys","rules"};
 if(!keys(bonds,bond_fields,2)||!integer(get(bonds,"version"),1,1)||!array_objects(get(bonds,"records"),256))return DSR_INVALID;
 if(!keys(enroll,enroll_fields,3)||!integer(get(enroll,"version"),1,1)||!array_objects(get(enroll,"keys"),16)||!array_objects(get(enroll,"rules"),8))return DSR_INVALID;
 const cJSON *photos=get(root,"photos");if(!array_objects(photos,DSR_BACKUP_MAX_PHOTOS))return DSR_INVALID;
 static const char *const photo_fields[]={"id","sha256","bytes"};
 const cJSON *photo;cJSON_ArrayForEach(photo,photos){uint8_t id[16],sha[32];
  if(!keys(photo,photo_fields,3)||!unhex(get(photo,"id"),id,16)||!unhex(get(photo,"sha256"),sha,32)||memcmp(id,sha,16)||!integer(get(photo,"bytes"),DSR_BACKUP_PHOTO_BYTES,DSR_BACKUP_PHOTO_BYTES))return DSR_INVALID;
  for(const cJSON *other=photo->next;other;other=other->next){const cJSON *id2=get(other,"id");if(cJSON_IsString(id2)&&!strcmp(get(photo,"id")->valuestring,id2->valuestring))return DSR_INVALID;}
 }
 return DSR_OK;
}
static dsr_result_t decode(dsr_backup_image_t *image,const uint8_t device_id[32])
{
 const uint8_t *data=image->owned;size_t size=image->size;
 if(size<HEADER||memcmp(data,"DSRSNP1",8)||u16(data+10)!=HEADER||u16(data+18)||u32(data+20)!=size)return DSR_INVALID;
 if(u16(data+8)!=1)return DSR_UNSUPPORTED;
 uint32_t metadata=u32(data+12);unsigned count=u16(data+16);
 if(!metadata||metadata>DSR_BACKUP_METADATA_BYTES||metadata>size-HEADER||count>DSR_BACKUP_MAX_PHOTOS||size-HEADER-metadata!=(size_t)count*(PHOTO_HEADER+DSR_BACKUP_PHOTO_BYTES))return DSR_INVALID;
 if(!json_bytes(data+HEADER,metadata))return DSR_INVALID;
 const char *end=NULL;image->manifest=cJSON_ParseWithLengthOpts((const char *)data+HEADER,metadata,&end,false);
 if(!image->manifest)return DSR_INVALID;
 while(end<(const char *)data+HEADER+metadata&&(*end==' '||*end=='\t'||*end=='\r'||*end=='\n'))end++;
 if(end!=(const char *)data+HEADER+metadata)return DSR_INVALID;
 dsr_result_t result=dsr_backup_manifest_validate(image->manifest,device_id);if(result!=DSR_OK)return result;
 const cJSON *photos=get(image->manifest,"photos");if((unsigned)cJSON_GetArraySize(photos)!=count)return DSR_INVALID;
 size_t at=HEADER+metadata;image->count=count;
 for(unsigned i=0;i<count;i++,at+=PHOTO_HEADER+DSR_BACKUP_PHOTO_BYTES){
  const uint8_t *record=data+at;const cJSON *photo=cJSON_GetArrayItem(photos,(int)i);uint8_t sha[32];
  if(u32(record+48)!=DSR_BACKUP_PHOTO_BYTES||u32(record+52))return DSR_INVALID;
  dsr_backup_photo_t *p=&image->photos[i];
  if(!unhex(get(photo,"sha256"),p->sha256,32)||memcmp(record,p->sha256,16)||memcmp(record+16,p->sha256,32))return DSR_INVALID;
  if(mbedtls_sha256(record+PHOTO_HEADER,DSR_BACKUP_PHOTO_BYTES,sha,0)||memcmp(sha,p->sha256,32))return DSR_INVALID;
  memcpy(p->id,get(photo,"id")->valuestring,33);p->pixels=record+PHOTO_HEADER;
 }
 const cJSON *config=get(image->manifest,"config");if(get(config,"schema")->valueint!=DSR_CONFIG_SCHEMA){
  char error[96];char *old=cJSON_PrintUnformatted(config);bool legacy=false;char *next=old?dsr_config_upgrade(old,&legacy,error):NULL;free(old);
  if(!next)return DSR_INVALID;
  cJSON *upgraded=cJSON_Parse(next);free(next);if(!upgraded)return DSR_IO;
  if(!cJSON_ReplaceItemInObjectCaseSensitive(image->manifest,"config",upgraded)){cJSON_Delete(upgraded);return DSR_IO;}
 }
 return DSR_OK;
}
dsr_result_t dsr_backup_image_open(uint8_t *owned,size_t size,const uint8_t device_id[32],dsr_backup_image_t **out)
{
 if(out)*out=NULL;
 dsr_result_t result=!out||!owned||!device_id?DSR_INVALID:size>DSR_BACKUP_MAX_BYTES?DSR_FULL:DSR_OK;
 dsr_backup_image_t *image=NULL;if(result==DSR_OK){image=calloc(1,sizeof(*image));if(!image)result=DSR_IO;}
 if(image){image->owned=owned;image->size=size;result=decode(image,device_id);if(result==DSR_OK){*out=image;return result;}dsr_backup_image_free(image);}
 else if(owned){dsr_backup_wipe(owned,size);free(owned);}
 return result;
}
void dsr_backup_image_free(dsr_backup_image_t *image)
{if(image){discard(image->manifest);dsr_backup_wipe(image->owned,image->size);free(image->owned);dsr_backup_wipe(image,sizeof(*image));free(image);}}
const uint8_t *dsr_backup_image_bytes(const dsr_backup_image_t *image,size_t *size)
{if(size)*size=image?image->size:0;return image?image->owned:NULL;}
const cJSON *dsr_backup_image_manifest(const dsr_backup_image_t *image){return image?image->manifest:NULL;}
const dsr_backup_photo_t *dsr_backup_image_photo(const dsr_backup_image_t *image,unsigned index)
{return image&&index<image->count?&image->photos[index]:NULL;}
