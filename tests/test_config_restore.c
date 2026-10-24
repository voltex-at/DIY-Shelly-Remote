#include "dsr_config_restore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"restore-config:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
 char dir[]="/tmp/dsr-restore-XXXXXX",error[96];CHECK(mkdtemp(dir));uint32_t next=0;
 CHECK(dsr_config_store_restore_revision(dir,3,&next)&&next==4);
 char *base=dsr_config_default();cJSON *root=cJSON_Parse(base);free(base);CHECK(root);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(root,"revision"),700);char *a=cJSON_PrintUnformatted(root);CHECK(dsr_config_store_save(dir,a,error));free(a);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(root,"revision"),900);a=cJSON_PrintUnformatted(root);CHECK(dsr_config_store_save(dir,a,error));free(a);
 CHECK(dsr_config_store_restore_revision(dir,3,&next)&&next==901);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(root,"revision"),next);a=cJSON_PrintUnformatted(root);CHECK(a);
 CHECK(!dsr_config_store_restore_slot(dir,2,a));CHECK(dsr_config_store_restore_slot(dir,0,a));CHECK(dsr_config_store_restore_slot(dir,1,a));
 bool found=false;char *loaded=dsr_config_store_load(dir,&found,error);CHECK(found&&loaded&&!strcmp(loaded,a));free(loaded);
 char path[128];snprintf(path,sizeof(path),"%s/config0.dat",dir);CHECK(unlink(path)==0);loaded=dsr_config_store_load(dir,&found,error);CHECK(found&&loaded&&dsr_config_revision(loaded)==901);free(loaded);
 CHECK(!dsr_config_store_restore_revision(dir,UINT32_MAX-1,&next));CHECK(!dsr_config_store_restore_slot(dir,0,"{}"));
 free(a);cJSON_Delete(root);snprintf(path,sizeof(path),"%s/config1.dat",dir);CHECK(unlink(path)==0);CHECK(rmdir(dir)==0);
 printf("PASS: %u real checkpoint restore checks; temporary filesystem, no firmware data\n",checks);return 0;
}
