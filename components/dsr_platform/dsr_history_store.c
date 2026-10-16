#define _POSIX_C_SOURCE 200809L
#include "dsr_history_private.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {uint32_t generation;size_t checkpoint,length;bool valid,future,exists;} file_t;
static uint32_t get32(const unsigned char *p){uint32_t x=0;for(unsigned i=0;i<4;i++)x|=(uint32_t)p[i]<<(8*i);return x;}
static bool path(char out[160],const char *dir,unsigned slot)
{int n=snprintf(out,160,"%s/history%u.dat",dir,slot);return n>0&&n<160&&slot<2;}
static uint32_t update_crc(uint32_t crc,const unsigned char *data,size_t length)
{while(length--){crc^=*data++;for(unsigned i=0;i<8;i++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));}return crc;}
static file_t inspect(const char *dir,unsigned slot)
{
 file_t info={0};char name[160];if(!path(name,dir,slot))return info;
 FILE *f=fopen(name,"rb");if(!f){info.exists=errno!=ENOENT;return info;}info.exists=true;
 unsigned char header[32],buffer[512];
 if(fread(header,1,sizeof(header),f)!=sizeof(header)||get32(header)!=0x48525344)goto done;
 if(get32(header+4)>1){info.future=true;goto done;}
 if(get32(header+4)!=1||get32(header+20)!=dsr_history_crc(header,20)||get32(header+24)||get32(header+28))goto done;
 info.generation=get32(header+8);info.checkpoint=get32(header+12);
 if(!info.generation||info.checkpoint<32||info.checkpoint>DSR_HISTORY_SNAPSHOT_MAX)goto done;
 uint32_t crc=UINT32_MAX;size_t left=info.checkpoint-32;
 while(left){size_t n=left>sizeof(buffer)?sizeof(buffer):left;if(fread(buffer,1,n,f)!=n)goto done;crc=update_crc(crc,buffer,n);left-=n;}
 if(~crc!=get32(header+16)||ferror(f))goto done;
 if(fseek(f,0,SEEK_END)||ftell(f)<0)goto done;
 info.length=(size_t)ftell(f);if(info.length>DSR_HISTORY_FILE_MAX||info.length<info.checkpoint)goto done;
 /* A valid future endpoint kind/frame is not corruption to erase. Check
  * all complete frames, including append records after the checkpoint. */
 if(fseek(f,32,SEEK_SET))goto done;
 for(size_t at=32;at<info.length;){
  unsigned char record[80];if(fread(record,1,8,f)!=8)break;size_t size=record[2];
  if((size!=40&&size!=80)||record[3]||at+size>info.length||fread(record+8,1,size-8,f)!=size-8)break;
  uint32_t expected=get32(record+4);memset(record+4,0,4);if(dsr_history_crc(record,size)!=expected)break;
  if(record[0]>2||(size==80&&record[0]==1&&(record[14]||record[15]))){info.future=true;goto done;}
  at+=size;
 }
 info.valid=true;
done:if(fclose(f))info.valid=false;return info;
}
static bool replay(dsr_history_store_t *store,dsr_history_model_t *model,const char *dir,unsigned slot,file_t info)
{
 char name[160];if(!path(name,dir,slot))return false;FILE *f=fopen(name,"rb");if(!f)return false;
 bool ok=fseek(f,32,SEEK_SET)==0;size_t used=32;unsigned char frame[80];
 memset(model,0,sizeof(*model));
 while(ok&&used<info.length){
  if(fread(frame,1,8,f)!=8){if(used<info.checkpoint)ok=false;break;}
  size_t length=frame[2];
  if((length!=40&&length!=80)||frame[3]||used+length>info.length||fread(frame+8,1,length-8,f)!=length-8||!dsr_history_decode(model,frame,length)){
   if(used<info.checkpoint)ok=false;
   break;
  }
  used+=length;if(used>info.checkpoint&&used-length<info.checkpoint){ok=false;break;}
 }
 if(used<info.checkpoint||ferror(f))ok=false;
 if(fclose(f))ok=false;
 if(ok){
  store->slot=slot;store->generation=info.generation;store->length=used;store->repair=used!=info.length;
  for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++){store->known[i]=model->slots[i].used;store->keys[i]=model->slots[i].key;}
 }
 return ok;
}
bool dsr_history_store_load(dsr_history_store_t *store,dsr_history_model_t *model,const char *dir)
{
 if(!store||!model||!dir)return false;
 memset(store,0,sizeof(*store));store->slot=1;
 file_t a=inspect(dir,0),b=inspect(dir,1);
 if(a.future||b.future){store->future=true;return false;}
 if(!a.exists&&!b.exists){memset(model,0,sizeof(*model));store->loaded=true;return true;}
 unsigned first=b.valid&&(!a.valid||b.generation>a.generation)?1:0;
 file_t selected=first?b:a,other=first?a:b;
 if((selected.valid&&replay(store,model,dir,first,selected))||(other.valid&&replay(store,model,dir,1-first,other))){store->loaded=true;return true;}
 /* Both present but invalid are preserved. Do not manufacture a blank store. */
 memset(model,0,sizeof(*model));return false;
}
bool dsr_history_store_append(dsr_history_store_t *store,const void *data,size_t length,const char *dir)
{
 if(!store||!store->loaded||store->future||!store->generation||!data||!length||length>120||store->length+length>DSR_HISTORY_FILE_MAX)return false;
 char name[160];if(!path(name,dir,store->slot))return false;FILE *f=fopen(name,"r+b");if(!f)return false;
 bool ok=true;
 if(store->repair){ok=ftruncate(fileno(f),(off_t)store->length)==0&&fsync(fileno(f))==0;if(ok)store->repair=false;}
 if(ok)ok=!fseek(f,(long)store->length,SEEK_SET)&&fwrite(data,1,length,f)==length&&fflush(f)==0&&fsync(fileno(f))==0;
 unsigned char verify[120];
 if(ok)ok=!fseek(f,(long)store->length,SEEK_SET)&&fread(verify,1,length,f)==length&&!memcmp(data,verify,length)&&fgetc(f)==EOF;
 if(fclose(f))ok=false;
 if(ok)store->length+=length;else store->repair=true;
 return ok;
}
bool dsr_history_store_checkpoint(dsr_history_store_t *store,const void *data,size_t length,const char *dir)
{
 const unsigned char *bytes=data;
 if(!store||!store->loaded||store->future||store->generation==UINT32_MAX||!bytes||length<32||length>DSR_HISTORY_SNAPSHOT_MAX||get32(bytes+8)!=store->generation+1)return false;
 unsigned slot=1-store->slot;char name[160];if(!path(name,dir,slot))return false;
 FILE *f=fopen(name,"wb");if(!f)return false;
 bool ok=fwrite(bytes,1,length,f)==length&&fflush(f)==0&&fsync(fileno(f))==0;if(fclose(f))ok=false;
 file_t info=inspect(dir,slot);ok=ok&&info.valid&&info.generation==store->generation+1&&info.length==length&&info.checkpoint==length;
 /* Exact readback checks the complete immutable snapshot, not only its CRC. */
 if(ok){f=fopen(name,"rb");if(!f)ok=false;else{
  unsigned char buffer[512];size_t at=0;
  while(ok&&at<length){size_t n=length-at>sizeof(buffer)?sizeof(buffer):length-at;ok=fread(buffer,1,n,f)==n&&!memcmp(buffer,bytes+at,n);at+=n;}
  if(fclose(f))ok=false;
 }}
 if(ok){store->slot=slot;store->generation=info.generation;store->length=length;store->repair=false;memset(store->known,0,sizeof(store->known));}
 return ok;
}
