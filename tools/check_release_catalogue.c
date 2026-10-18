/* Read-only host check using the exact bounded catalogue parser in firmware. */
#include "dsr_ota_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
 if(argc!=4||(strcmp(argv[2],"dev")&&strcmp(argv[2],"stable")))return 2;
 FILE *file=fopen(argv[1],"rb");if(!file)return 2;
 unsigned char *body=malloc(DSR_OTA_RELEASE_MAX+1);if(!body){fclose(file);return 2;}
 size_t length=fread(body,1,DSR_OTA_RELEASE_MAX+1,file);bool io=ferror(file);fclose(file);
 dsr_ota_discovery_t out;unsigned entries=0;
 dsr_ota_release_result_t result=io?DSR_RELEASE_ARGUMENT:dsr_ota_discovery_parse(body,length,!strcmp(argv[2],"dev")?DSR_OTA_DEV:DSR_OTA_STABLE,&out,&entries);
 free(body);
 if(result!=DSR_RELEASE_OK){fprintf(stderr,"REFUSED: firmware catalogue parser result=%u, bytes=%zu, limit=%u\n",(unsigned)result,length,DSR_OTA_RELEASE_MAX);return 1;}
 if(strcmp(out.candidate.tag,argv[3])){fprintf(stderr,"REFUSED: catalogue selected another tag\n");return 1;}
 printf("PASS: firmware catalogue parser selects %s; %zu/%u bytes; %u entries; metadata only, signature checked separately\n",out.candidate.tag,length,DSR_OTA_RELEASE_MAX,entries);return 0;
}
