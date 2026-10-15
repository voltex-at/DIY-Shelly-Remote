#include "dsr_workspace.h"
#include <stdlib.h>
#include <string.h>
/* Disabled for the original Wi-Fi fixture until explicitly enabled. */
static char *stored;
void test_workspace_enable(void){stored=dsr_config_default();}
void test_workspace_disable(void){free(stored);stored=NULL;}
char *dsr_workspace_snapshot(void){if(!stored)return NULL;char *copy=malloc(strlen(stored)+1);if(copy)strcpy(copy,stored);return copy;}
uint32_t dsr_workspace_revision(void){return stored?dsr_config_revision(stored):0;}
bool dsr_workspace_submit(const char *json,uint32_t revision)
{char error[96];char *next=stored?dsr_config_replace(stored,json,revision,error):NULL;if(!next)return false;free(stored);stored=next;return true;}
void dsr_workspace_status(bool *busy,char message[96]){*busy=false;message[0]=0;}
void dsr_panel_pair_begin(char code[17]){strcpy(code,"0123456789abcdef");}
void dsr_panel_pair_cancel(void){}
void dsr_panel_allow_ota(void){}
const void *dsr_photo_pixels(const char *id){(void)id;return NULL;}
const char *dsr_photo_id(unsigned index){(void)index;return NULL;}
const char *dsr_tls_fingerprint(void){return "TEST FIXTURE";}
