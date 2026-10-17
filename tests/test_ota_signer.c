/* Host-only: real dsr_ota_manifest_verify + REAL production trust provider.
 * argv: manifest-file expected-result current-version channel(0/1) [version size sha256-hex] */
#include "dsr_ota_manifest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
 if(argc!=5&&argc!=8){fprintf(stderr,"usage\n");return 2;}
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;unsigned char json[4096];size_t n=fread(json,1,sizeof(json),f);fclose(f);
 dsr_ota_policy_t policy={argv[3],(dsr_ota_channel_t)atoi(argv[4]),0x600000};dsr_ota_verified_t out;memset(&out,0xa5,sizeof(out));
 dsr_manifest_result_t r=dsr_ota_manifest_verify(json,n,&policy,&out);
 if((int)r!=atoi(argv[2])){fprintf(stderr,"expected %s got %d\n",argv[2],(int)r);return 1;}
 if(argc==8){
  if(strcmp(out.version,argv[5])||out.size!=strtoul(argv[6],NULL,10))return 1;
  char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",out.sha256[i]);if(strcmp(hex,argv[7]))return 1;
 }
 return 0;
}
