#include "dsr_backup_format.h"
#include "dsr_config.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static const char *fixture_path;
static unsigned parser_calls;
cJSON *__real_cJSON_ParseWithLengthOpts(const char *,size_t,const char **,cJSON_bool);
cJSON *__wrap_cJSON_ParseWithLengthOpts(const char *v,size_t n,const char **end,cJSON_bool terminated)
{parser_calls++;return __real_cJSON_ParseWithLengthOpts(v,n,end,terminated);}
static const uint8_t device[32]={1};
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"format:%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void hex(char *out,const uint8_t *data,size_t size)
{for(size_t i=0;i<size;i++)sprintf(out+2*i,"%02x",data[i]);}
static cJSON *manifest(void)
{
 char data[2048];FILE *file=fopen(fixture_path,"rb");CHECK(file);size_t size=fread(data,1,sizeof(data)-1,file);CHECK(feof(file));CHECK(!fclose(file));data[size]=0;
 cJSON *root=cJSON_Parse(data);CHECK(root);return root;
}
static uint8_t *pack(cJSON *root,unsigned photos,size_t *size)
{
 unsigned char *pixels=calloc(1,115200);CHECK(pixels);uint8_t hashes[16][32];
 for(unsigned i=0;i<photos&&i<16;i++){
  pixels[0]=(uint8_t)i;CHECK(!mbedtls_sha256(pixels,115200,hashes[i],0));char id[33]={0},sha[65]={0};hex(id,hashes[i],16);hex(sha,hashes[i],32);
  cJSON *row=cJSON_CreateObject();cJSON_AddStringToObject(row,"id",id);cJSON_AddStringToObject(row,"sha256",sha);cJSON_AddNumberToObject(row,"bytes",115200);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(root,"photos"),row);
 }
 char *json=cJSON_PrintUnformatted(root);CHECK(json);size_t length=strlen(json);*size=24+length+photos*(56+115200);
 uint8_t *data=calloc(1,*size+1);CHECK(data);memcpy(data,"DSRSNP1",7);put16(data+8,1);put16(data+10,24);put32(data+12,(uint32_t)length);put16(data+16,photos);put32(data+20,(uint32_t)*size);memcpy(data+24,json,length);free(json);
 for(unsigned i=0;i<photos&&i<16;i++){uint8_t *at=data+24+length+i*(56+115200);memcpy(at,hashes[i],16);memcpy(at+16,hashes[i],32);put32(at+48,115200);pixels[0]=(uint8_t)i;memcpy(at+56,pixels,115200);}
 free(pixels);return data;
}
static void rejected(uint8_t *data,size_t size,dsr_result_t reason)
{dsr_backup_image_t *image=(void *)1;CHECK(dsr_backup_image_open(data,size,device,&image)==reason);CHECK(image==NULL);}
int main(int argc,char **argv)
{
 CHECK(argc==2);fixture_path=argv[1];
 size_t size;dsr_backup_image_t *image=NULL;cJSON *root=manifest();uint8_t *data=pack(root,0,&size);
 CHECK(dsr_backup_image_open(data,size,device,&image)==DSR_OK);CHECK(image);size_t received=0;CHECK(dsr_backup_image_bytes(image,&received)==data&&received==size);dsr_backup_image_free(image);cJSON_Delete(root);
 root=manifest();data=pack(root,16,&size);CHECK(size<2097152);CHECK(dsr_backup_image_open(data,size,device,&image)==DSR_OK);CHECK(dsr_backup_image_photo(image,15)&&!dsr_backup_image_photo(image,16));dsr_backup_image_free(image);cJSON_Delete(root);
 root=manifest();data=pack(root,0,&size);data[8]=2;rejected(data,size,DSR_UNSUPPORTED);cJSON_Delete(root);
 root=manifest();data=pack(root,0,&size);data[18]=1;rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,0,&size);rejected(data,size+1,DSR_INVALID);cJSON_Delete(root);
 root=manifest();cJSON_ReplaceItemInObjectCaseSensitive(root,"device",cJSON_CreateString("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));data=pack(root,0,&size);rejected(data,size,DSR_IDENTITY_MISMATCH);cJSON_Delete(root);
 root=manifest();cJSON_AddStringToObject(root,"unknown","forbidden");data=pack(root,0,&size);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();cJSON_AddNumberToObject(root,"format",1);data=pack(root,0,&size);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();cJSON_AddNumberToObject(cJSON_GetObjectItemCaseSensitive(root,"settings"),"brightness",70);data=pack(root,0,&size);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,1,&size);data[size-1]^=1;rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,1,&size);rejected(data,size-1,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,2,&size);put32(data+12,65537);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();cJSON *cfg=cJSON_GetObjectItemCaseSensitive(root,"config");cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cfg,"schema"),99);data=pack(root,0,&size);rejected(data,size,DSR_UNSUPPORTED);cJSON_Delete(root);
 root=manifest();cfg=cJSON_GetObjectItemCaseSensitive(root,"config");cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cfg,"schema"),1);cJSON_DeleteItemFromObjectCaseSensitive(cfg,"groups");cJSON_DeleteItemFromObjectCaseSensitive(cfg,"automations");data=pack(root,0,&size);CHECK(dsr_backup_image_open(data,size,device,&image)==DSR_OK);CHECK(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(dsr_backup_image_manifest(image),"config"),"schema")->valueint==3);dsr_backup_image_free(image);cJSON_Delete(root);
 root=manifest();cJSON_ReplaceItemInObjectCaseSensitive(root,"firmware",cJSON_CreateString("\xc0\xaf"));data=pack(root,0,&size);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 data=calloc(1,2097153);CHECK(data);rejected(data,2097153,DSR_FULL);
 /* A malformed nesting bomb must be rejected before cJSON recursion on ESP. */
 size=24+129;data=calloc(1,size);CHECK(data);memcpy(data,"DSRSNP1",7);put16(data+8,1);put16(data+10,24);put32(data+12,129);put32(data+20,(uint32_t)size);
 memset(data+24,'[',64);data[24+64]='0';memset(data+24+65,']',64);parser_calls=0;rejected(data,size,DSR_INVALID);CHECK(parser_calls==0);
 for(size_t tiny=0;tiny<24;tiny++){data=calloc(1,24);CHECK(data);rejected(data,tiny,DSR_INVALID);}
 root=manifest();data=pack(root,0,&size);put16(data+16,17);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,0,&size);char *fw=strstr((char *)data+24,"0.7.70-dev");CHECK(fw);memcpy(fw,"\\u0000xxxx",10);rejected(data,size,DSR_INVALID);cJSON_Delete(root);
 root=manifest();data=pack(root,2,&size);cJSON *photos=cJSON_GetObjectItemCaseSensitive(root,"photos");cJSON *first=cJSON_GetArrayItem(photos,0),*second=cJSON_GetArrayItem(photos,1);
 cJSON_ReplaceItemInObjectCaseSensitive(second,"id",cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(first,"id"),true));cJSON_ReplaceItemInObjectCaseSensitive(second,"sha256",cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(first,"sha256"),true));CHECK(dsr_backup_manifest_validate(root,device)==DSR_INVALID);free(data);cJSON_Delete(root);
 printf("PASS: %u system-data format checks; real parser/config/SHA-256, no storage or radio\n",checks);return 0;
}
