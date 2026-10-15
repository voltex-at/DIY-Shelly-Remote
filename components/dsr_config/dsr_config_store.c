#include "dsr_config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint32_t dsr_config_crc32(const void *data,size_t size)
{const unsigned char *p=data;uint32_t crc=UINT32_MAX;while(size--){crc^=*p++;for(unsigned b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));}return ~crc;}
static void path(char out[192],const char *dir,unsigned slot){snprintf(out,192,"%s/config%u.dat",dir,slot);}
static char *read_slot(const char *dir,unsigned slot,bool *exists)
{
 char name[192];path(name,dir,slot);FILE *f=fopen(name,"rb");if(!f)return NULL;*exists=true;
 uint32_t header[4];char *json=NULL;
 if(fread(header,sizeof(header),1,f)!=1||header[0]!=0x44535231||header[1]>DSR_CONFIG_MAX_JSON||!header[1])goto done;
 json=malloc((size_t)header[1]+1);if(!json)goto done;
 if(fread(json,1,header[1],f)!=header[1]||fgetc(f)!=EOF||dsr_config_crc32(json,header[1])!=header[2]){free(json);json=NULL;goto done;}
 json[header[1]]=0;if(!dsr_config_json_depth(json)){free(json);json=NULL;goto done;}char error[96];cJSON *r=cJSON_Parse(json);
 if(!r||!dsr_config_validate(r,error)||dsr_config_revision(json)!=header[3]){free(json);json=NULL;}cJSON_Delete(r);
done:fclose(f);return json;
}
char *dsr_config_store_load(const char *dir,bool *found,char error[96])
{
 bool exists=false;char *a=read_slot(dir,0,&exists),*b=read_slot(dir,1,&exists),*best=NULL;
 if(a&&(!b||dsr_config_revision(a)>=dsr_config_revision(b))){best=a;free(b);}else{best=b;free(a);}
 *found=best!=NULL;error[0]=0;if(!best&&exists)snprintf(error,96,"Both configuration checkpoints invalid; files preserved");return best;
}
bool dsr_config_store_save(const char *dir,const char *json,char error[96])
{
 cJSON *r=cJSON_Parse(json);if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return false;}cJSON_Delete(r);
 size_t length=strlen(json);if(length>DSR_CONFIG_MAX_JSON)return false;
 bool exists=false;char *a=read_slot(dir,0,&exists),*b=read_slot(dir,1,&exists);
 uint32_t ar=a?dsr_config_revision(a):0,br=b?dsr_config_revision(b):0;free(a);free(b);
 unsigned slot=ar<=br?0:1;char name[192];path(name,dir,slot);
 FILE *f=fopen(name,"wb");if(!f){snprintf(error,96,"Cannot open configuration checkpoint");return false;}
 uint32_t h[]={0x44535231,(uint32_t)length,dsr_config_crc32(json,length),dsr_config_revision(json)};
 bool ok=fwrite(h,sizeof(h),1,f)==1&&fwrite(json,1,length,f)==length&&fflush(f)==0&&fsync(fileno(f))==0;
 if(fclose(f)!=0)ok=false;
 char *verify=read_slot(dir,slot,&exists);ok=ok&&verify&&!strcmp(json,verify);free(verify);
 if(!ok)snprintf(error,96,"Checkpoint write/readback failed; previous configuration retained");
 return ok;
}
