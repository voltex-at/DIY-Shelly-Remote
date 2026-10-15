#include "dsr_config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

uint32_t dsr_config_crc32(const void *data,size_t size)
{const unsigned char *p=data;uint32_t crc=UINT32_MAX;while(size--){crc^=*p++;for(unsigned b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));}return ~crc;}
static void path(char out[192],const char *dir,unsigned slot){snprintf(out,192,"%s/config%u.dat",dir,slot);}
static char *read_slot(const char *dir,unsigned slot,bool *exists,bool *future)
{
 char name[192];path(name,dir,slot);FILE *f=fopen(name,"rb");if(!f){if(errno!=ENOENT)*exists=true;return NULL;}*exists=true;
 uint32_t header[4];char *json=NULL;
 if(fread(header,sizeof(header),1,f)!=1||header[0]!=0x44535231||header[1]>DSR_CONFIG_MAX_JSON||!header[1])goto done;
 json=malloc((size_t)header[1]+1);if(!json)goto done;
 if(fread(json,1,header[1],f)!=header[1]||fgetc(f)!=EOF||dsr_config_crc32(json,header[1])!=header[2]){free(json);json=NULL;goto done;}
 json[header[1]]=0;if(!dsr_config_json_depth(json)){free(json);json=NULL;goto done;}char error[96];cJSON *r=cJSON_Parse(json);
 const cJSON *schema=cJSON_GetObjectItemCaseSensitive(r,"schema");if(cJSON_IsNumber(schema)&&schema->valuedouble>DSR_CONFIG_SCHEMA)*future=true;
 if(!r||!dsr_config_validate(r,error)||dsr_config_revision(json)!=header[3]){free(json);json=NULL;}cJSON_Delete(r);
done:fclose(f);return json;
}
char *dsr_config_store_load(const char *dir,bool *found,char error[96])
{
 bool exists=false,future=false;char *a=read_slot(dir,0,&exists,&future),*b=read_slot(dir,1,&exists,&future),*best=NULL;
 if(future){free(a);free(b);*found=false;snprintf(error,96,"Newer configuration schema found; both checkpoints preserved");return NULL;}
 if(a&&(!b||dsr_config_revision(a)>=dsr_config_revision(b))){best=a;free(b);}else{best=b;free(a);}
 *found=best!=NULL;error[0]=0;if(!best&&exists)snprintf(error,96,"Both configuration checkpoints invalid; files preserved");return best;
}
bool dsr_config_store_save(const char *dir,const char *json,char error[96])
{
 cJSON *r=cJSON_Parse(json);if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return false;}cJSON_Delete(r);
 size_t length=strlen(json);if(length>DSR_CONFIG_MAX_JSON)return false;
 bool exists=false,future=false;char *a=read_slot(dir,0,&exists,&future),*b=read_slot(dir,1,&exists,&future);
 if(future){free(a);free(b);snprintf(error,96,"Newer schema must not be overwritten by older firmware");return false;}
 uint32_t ar=a?dsr_config_revision(a):0,br=b?dsr_config_revision(b):0;free(a);free(b);
 unsigned slot=ar<=br?0:1;char name[192];path(name,dir,slot);
 FILE *f=fopen(name,"wb");if(!f){snprintf(error,96,"Cannot open configuration checkpoint");return false;}
 uint32_t h[]={0x44535231,(uint32_t)length,dsr_config_crc32(json,length),dsr_config_revision(json)};
 bool ok=fwrite(h,sizeof(h),1,f)==1&&fwrite(json,1,length,f)==length&&fflush(f)==0&&fsync(fileno(f))==0;
 if(fclose(f)!=0)ok=false;
 char *verify=read_slot(dir,slot,&exists,&future);ok=ok&&verify&&!strcmp(json,verify);free(verify);
 if(!ok)snprintf(error,96,"Checkpoint write/readback failed; previous configuration retained");
 return ok;
}
bool dsr_config_store_collect_photos(const char *dir,unsigned *removed,char error[96])
{
 *removed=0;error[0]=0;bool exists[2]={false,false},future=false;
 char *json[2]={read_slot(dir,0,&exists[0],&future),read_slot(dir,1,&exists[1],&future)};
 cJSON *roots[2]={NULL,NULL};bool safe=!future&&(json[0]||json[1]);
 for(unsigned i=0;i<2;i++){if(exists[i]&&!json[i])safe=false;if(json[i])roots[i]=cJSON_Parse(json[i]);if(json[i]&&!roots[i])safe=false;free(json[i]);}
 if(!safe){snprintf(error,96,"Photo cleanup skipped: checkpoints require recovery");goto done;}
 DIR *directory=opendir(dir);if(!directory){safe=false;snprintf(error,96,"Cannot open photo directory");goto done;}
 struct dirent *entry;
 while((entry=readdir(directory))){
  const char *name=entry->d_name;if(strlen(name)!=37||name[0]!='p'||strcmp(name+33,".rgb"))continue;
  char id[33];memcpy(id,name+1,32);id[32]=0;bool valid=true;
  for(unsigned n=0;n<32;n++)if(!((id[n]>='0'&&id[n]<='9')||(id[n]>='a'&&id[n]<='f')))valid=false;
  if(!valid)continue;
  bool referenced=false;const cJSON *room;
  for(unsigned i=0;i<2;i++)cJSON_ArrayForEach(room,cJSON_GetObjectItemCaseSensitive(roots[i],"rooms")){
   const cJSON *photo=cJSON_GetObjectItemCaseSensitive(room,"photo");if(cJSON_IsString(photo)&&!strcmp(photo->valuestring,id))referenced=true;
  }
  if(!referenced){char file[256];int length=snprintf(file,sizeof(file),"%s/%s",dir,name);
   if(length<0||(size_t)length>=sizeof(file)||unlink(file)){safe=false;snprintf(error,96,"Cannot reclaim an unused photo");}else (*removed)++;
  }
 }
 closedir(directory);
done:for(unsigned i=0;i<2;i++)cJSON_Delete(roots[i]);return safe;
}
