/* Actual build-artifact fixture only. Metadata is NOT from a signed manifest.
 * This exercises image streaming and cannot authorize an OTA installation. */
#include "dsr_ota_image.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
int main(int argc,char **argv)
{
 CHECK(argc==3);FILE *file=fopen(argv[1],"rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long size=ftell(file);
 CHECK(size>=512&&size<=0x600000);rewind(file);unsigned char *bytes=malloc((size_t)size);CHECK(bytes);
 CHECK(fread(bytes,1,(size_t)size,file)==(size_t)size&&fgetc(file)==EOF);fclose(file);
 dsr_ota_verified_t v={.size=(uint32_t)size};file=fopen(argv[2],"rb");CHECK(file);CHECK(fread(v.sha256,1,32,file)==32&&fgetc(file)==EOF);fclose(file);
 const size_t chunks[]={1,511,512,4096};
 for(unsigned i=0;i<sizeof(chunks)/sizeof(chunks[0]);i++){
  dsr_ota_image_t *stream=NULL;CHECK(dsr_ota_image_create(&v,&stream)==DSR_IMAGE_OK&&stream);
  for(size_t offset=0;offset<(size_t)size;){size_t length=(size_t)size-offset;if(length>chunks[i])length=chunks[i];CHECK(dsr_ota_image_update(stream,bytes+offset,length)==DSR_IMAGE_OK);offset+=length;}
  CHECK(dsr_ota_image_received(stream)==(size_t)size&&dsr_ota_image_header_ready(stream));CHECK(dsr_ota_image_finish(stream)==DSR_IMAGE_OK);dsr_ota_image_destroy(stream);CHECK(!dsr_test_sha_live());
  printf("PASS: ARTIFACT FIXTURE ONLY bytes=%ld chunk=%lu actual helper/OpenSSL SHA256/header/board marker; signature/trust/Flash NOT tested\n",size,(unsigned long)chunks[i]);
 }
 free(bytes);printf("PASS: %u actual artifact image checks; no installation authorization\n",checks);return 0;
}
