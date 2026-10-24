#include "image.h"
#include "dsr_backup_bonds_model.h"
#include "mbedtls/sha256.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(uint8_t *p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++){p[i]=(uint8_t)v;v>>=8;}}
static void hex(const uint8_t *p,unsigned n,char *out){for(unsigned i=0;i<n;i++)snprintf(out+2*i,3,"%02x",p[i]);}
dsr_backup_image_t *test_backup_image(const char *fixture,unsigned photos,unsigned bonds)
{
 assert(photos<=16&&bonds<=16);FILE *f=fopen(fixture,"rb");assert(f);char input[2048];size_t n=fread(input,1,sizeof(input)-1,f);assert(feof(f)&&!fclose(f));input[n]=0;cJSON *root=cJSON_Parse(input);assert(root);
 cJSON *config=cJSON_GetObjectItemCaseSensitive(root,"config"),*rows=cJSON_GetObjectItemCaseSensitive(root,"photos");
 cJSON *settings=cJSON_GetObjectItemCaseSensitive(root,"settings");assert(cJSON_ReplaceItemInObjectCaseSensitive(settings,"password",cJSON_CreateString("73796e7468657469632d736563726574")));
 uint8_t *pixels=malloc(115200);assert(pixels);
 for(unsigned i=0;i<photos;i++){
  memset(pixels,(int)i,115200);uint8_t sha[32];assert(!mbedtls_sha256(pixels,115200,sha,0));char id[33],digest[65];hex(sha,16,id);hex(sha,32,digest);
  cJSON *p=cJSON_CreateObject();assert(p&&cJSON_AddStringToObject(p,"id",id)&&cJSON_AddStringToObject(p,"sha256",digest)&&cJSON_AddNumberToObject(p,"bytes",115200)&&cJSON_AddItemToArray(rows,p));
  cJSON *v=cJSON_CreateObject();assert(v&&cJSON_AddNumberToObject(v,"id",i%8+1)&&cJSON_AddStringToObject(v,"name","Fixture photo")&&cJSON_AddStringToObject(v,"photo",id));
  assert(i<8?cJSON_AddNumberToObject(v,"icon",0)!=NULL:cJSON_AddArrayToObject(v,"actions")!=NULL);assert(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(config,i<8?"rooms":"scenes"),v));
 }
 cJSON *records=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"bonds"),"records");
 for(unsigned i=0;i<bonds;i++){
  dsr_backup_bond_t b={.family=DSR_BACKUP_BOND_OUR_SEC,.index=(uint8_t)(i+1)};b.value.sec.key_size=16;b.value.sec.flags=3;b.value.sec.ltk[5]=77;b.value.sec.irk[7]=(uint8_t)(i+1);
  cJSON *v=NULL;assert(dsr_backup_bond_encode(&b,&v)==DSR_OK&&v&&cJSON_AddItemToArray(records,v));
 }
 char *metadata=cJSON_PrintUnformatted(root);assert(metadata);size_t metadata_size=strlen(metadata),size=24+metadata_size+photos*(56+115200);uint8_t *raw=calloc(1,size);assert(raw);
 memcpy(raw,"DSRSNP1",8);put(raw+8,1,2);put(raw+10,24,2);put(raw+12,(uint32_t)metadata_size,4);put(raw+16,photos,2);put(raw+20,(uint32_t)size,4);memcpy(raw+24,metadata,metadata_size);
 size_t at=24+metadata_size;for(unsigned i=0;i<photos;i++){memset(pixels,(int)i,115200);uint8_t sha[32];assert(!mbedtls_sha256(pixels,115200,sha,0));memcpy(raw+at,sha,16);memcpy(raw+at+16,sha,32);put(raw+at+48,115200,4);memcpy(raw+at+56,pixels,115200);at+=56+115200;}
 dsr_backup_json_discard(root);dsr_backup_wipe(metadata,metadata_size);free(metadata);free(pixels);dsr_backup_image_t *image=NULL;const uint8_t device[32]={1};assert(dsr_backup_image_open(raw,size,device,&image)==DSR_OK&&image);return image;
}
