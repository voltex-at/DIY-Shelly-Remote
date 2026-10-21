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
 /* Modes 0..3 are the four ids a 0.7.67 device may hold (1 = retired
  * midnight); 4 is an invalid byte, 5 a missing key. */
 assert(argc==2);unsigned mode=(unsigned)atoi(argv[1]);stored=(uint8_t)mode;missing=mode==5;
 uint8_t expected=mode>3||mode==DSR_STYLE_HINT_RETIRED?0:(uint8_t)mode;assert(dsr_style_hint_get()==expected);
 assert(commits==0); /* reading never writes */
 if(mode==DSR_STYLE_HINT_RETIRED){
  /* Midnight migrates losslessly to graphite: the next applied graphite style
   * rewrites the retired byte exactly once; later applies are idempotent. */
  dsr_style_hint_set(0);assert(commits==1&&stored==0&&dsr_style_hint_get()==0);
  dsr_style_hint_set(0);assert(commits==1);
  dsr_style_hint_set(DSR_STYLE_HINT_RETIRED);assert(commits==1&&stored==0&&dsr_style_hint_get()==0); /* never written again */
  puts("PASS: retired midnight hint reads as graphite, rewritten once, never re-selectable; NVS MODEL");return 0;
 }
 dsr_style_hint_set(4);assert(commits==0&&dsr_style_hint_get()==expected);
 dsr_style_hint_set(DSR_STYLE_HINT_RETIRED);assert(commits==0&&dsr_style_hint_get()==expected);
 dsr_style_hint_set(3);assert(dsr_style_hint_get()==3&&commits==(mode==3?0u:1u));
 unsigned count=commits;dsr_style_hint_set(3);assert(commits==count);
 fail_commit=true;dsr_style_hint_set(2);assert(dsr_style_hint_get()==3);
 fail_commit=false;dsr_style_hint_set(2);assert(dsr_style_hint_get()==2&&stored==2);
 puts("PASS: boot skin hint: Cyber Blue, graphite, light, invalid fallback, write-on-change, failed commit; NVS MODEL");return 0;
}
