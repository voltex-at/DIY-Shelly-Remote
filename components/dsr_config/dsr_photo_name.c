#include "dsr_photo_name.h"
#include <stdint.h>
#include <string.h>

static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static int hex(char value)
{return value>='0'&&value<='9'?value-'0':value>='a'&&value<='f'?value-'a'+10:-1;}
bool dsr_photo_filename(const char *id,char out[38])
{
 if(!id||!out||strlen(id)!=32)return false;
 uint8_t bytes[16];for(unsigned i=0;i<16;i++){
  int hi=hex(id[i*2]),lo=hex(id[i*2+1]);if(hi<0||lo<0)return false;
  bytes[i]=(uint8_t)((hi<<4)|lo);
 }
 char name[38]={'p'};unsigned at=1,bits=0;uint32_t value=0;
 for(unsigned i=0;i<16;i++){
  value=(value<<8)|bytes[i];bits+=8;
  while(bits>=6){bits-=6;name[at++]=alphabet[(value>>bits)&63];}
 }
 if(bits)name[at++]=alphabet[(value<<(6-bits))&63];
 memcpy(name+at,".rgb",5);memcpy(out,name,at+5);return true;
}
bool dsr_photo_filename_id(const char *name,char id[33])
{
 if(!name||!id||name[0]!='p')return false;
 size_t length=strlen(name);char result[33];
 if(length==37&&!strcmp(name+33,".rgb")){
  for(unsigned i=0;i<32;i++)if(hex(name[i+1])<0)return false;
  memcpy(result,name+1,32);result[32]=0;
 }else if(length==27&&!strcmp(name+23,".rgb")){
  uint8_t bytes[16];unsigned at=0,bits=0;uint32_t value=0;
  for(unsigned i=1;i<=22;i++){
   const char *found=strchr(alphabet,name[i]);if(!found||!name[i])return false;
   value=(value<<6)|(uint32_t)(found-alphabet);bits+=6;
   if(bits>=8){bits-=8;if(at>=16)return false;bytes[at++]=(uint8_t)(value>>bits);}
  }
  if(at!=16||bits!=4||(value&15))return false; /* Canonical base64url only. */
  static const char digits[]="0123456789abcdef";
  for(unsigned i=0;i<16;i++){result[2*i]=digits[bytes[i]>>4];result[2*i+1]=digits[bytes[i]&15];}result[32]=0;
 }else return false;
 memcpy(id,result,33);return true;
}
