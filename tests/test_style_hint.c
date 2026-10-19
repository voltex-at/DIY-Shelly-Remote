/* Real hint implementation, NVS calls MODEL. Each scenario is a fresh process. */
#include "dsr_style_hint.h"
#include "nvs.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t stored,pending;
static unsigned commits;
static bool missing,fail_commit;
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h)
{assert(!strcmp(name,"dsr_ui"));(void)mode;*h=1;return ESP_OK;}
esp_err_t nvs_get_u8(nvs_handle_t h,const char *key,uint8_t *out)
{assert(h==1&&!strcmp(key,"style"));*out=stored;return missing?ESP_FAIL:ESP_OK;}
esp_err_t nvs_set_u8(nvs_handle_t h,const char *key,uint8_t value)
{assert(h==1&&!strcmp(key,"style"));pending=value;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h)
{assert(h==1);commits++;if(fail_commit)return ESP_FAIL;stored=pending;return ESP_OK;}
void nvs_close(nvs_handle_t h){assert(h==1);}
int main(int argc,char **argv)
{
 assert(argc==2);unsigned mode=(unsigned)atoi(argv[1]);stored=(uint8_t)mode;missing=mode==5;
 uint8_t expected=mode>3?0:(uint8_t)mode;assert(dsr_style_hint_get()==expected);
 dsr_style_hint_set(4);assert(commits==0&&dsr_style_hint_get()==expected);
 dsr_style_hint_set(3);assert(dsr_style_hint_get()==3&&commits==(expected==3?0u:1u));
 unsigned count=commits;dsr_style_hint_set(3);assert(commits==count);
 fail_commit=true;dsr_style_hint_set(2);assert(dsr_style_hint_get()==3);
 fail_commit=false;dsr_style_hint_set(2);assert(dsr_style_hint_get()==2&&stored==2);
 puts("PASS: boot skin hint: Cyber Blue, legacy values, invalid fallback, write-on-change, failed commit; NVS MODEL");return 0;
}
