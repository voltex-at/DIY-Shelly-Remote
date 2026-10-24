#include "dsr_maintenance_grant.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"grants:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
 dsr_maint_grant_t g={0};dsr_maint_request_t r={.op=DSR_MAINT_EXPORT,.nonce={1},.session={2}},bad;
 CHECK(dsr_maint_grant_begin(&g,&r,7,100)==DSR_OK);CHECK(g.state==DSR_GRANT_WAIT);
 CHECK(!dsr_maint_grant_take(&g,&r,7,101)); /* no physical confirmation */
 CHECK(dsr_maint_grant_begin(&g,&r,7,102)==DSR_BUSY);
 uint8_t wrong[16]={9};CHECK(!dsr_maint_grant_confirm(&g,wrong,7,102,false,DSR_OTA_POWER_READY));
 CHECK(dsr_maint_grant_confirm(&g,r.nonce,7,103,false,DSR_OTA_POWER_UNCHECKED));
 bad=r;bad.op=DSR_MAINT_IMPORT_UPLOAD;CHECK(!dsr_maint_grant_take(&g,&bad,7,104));
 bad=r;bad.session[31]=8;CHECK(!dsr_maint_grant_take(&g,&bad,7,104));
 bad=r;bad.candidate_sha256[0]=1;CHECK(!dsr_maint_grant_take(&g,&bad,7,104));
 CHECK(dsr_maint_grant_take(&g,&r,7,104));CHECK(!dsr_maint_grant_take(&g,&r,7,105));
 dsr_maint_grant_finish(&g,DSR_OK);CHECK(dsr_maint_grant_begin(&g,&r,7,106)==DSR_CONFLICT);
 r.nonce[0]++;r.op=DSR_MAINT_IMPORT_UPLOAD;CHECK(dsr_maint_grant_begin(&g,&r,7,UINT32_MAX-1000)==DSR_OK);
 CHECK(dsr_maint_grant_confirm(&g,r.nonce,7,40,false,DSR_OTA_POWER_READY));
 CHECK(dsr_maint_grant_live(&g,7,118998));CHECK(!dsr_maint_grant_live(&g,7,119000)); /* elapsed120001 */
 r.nonce[0]++;CHECK(dsr_maint_grant_begin(&g,&r,7,10)==DSR_OK);
 CHECK(!dsr_maint_grant_confirm(&g,r.nonce,8,11,false,DSR_OTA_POWER_READY));CHECK(g.state==DSR_GRANT_DONE);
 r.nonce[0]++;r.op=DSR_MAINT_IMPORT_COMMIT;r.candidate_sha256[0]=3;
 CHECK(dsr_maint_grant_begin(&g,&r,8,200)==DSR_OK);
 CHECK(!dsr_maint_grant_confirm(&g,r.nonce,8,201,true,DSR_OTA_POWER_LOW));CHECK(g.result==DSR_OFFLINE);
 CHECK(!dsr_maint_grant_confirm(&g,r.nonce,8,202,true,DSR_OTA_POWER_BUSY));CHECK(g.result==DSR_BUSY);
 CHECK(!dsr_maint_grant_confirm(&g,r.nonce,8,203,false,DSR_OTA_POWER_UNKNOWN));CHECK(g.usb_required);
 CHECK(dsr_maint_grant_confirm(&g,r.nonce,8,204,true,DSR_OTA_POWER_UNKNOWN));
 CHECK(dsr_maint_grant_take(&g,&r,8,205));dsr_maint_grant_finish(&g,DSR_OK);
 r.nonce[0]++;r.op=DSR_MAINT_RESET;memset(r.candidate_sha256,0,32);memset(r.session,0,32);
 CHECK(dsr_maint_grant_begin(&g,&r,8,300)==DSR_OK);CHECK(dsr_maint_grant_confirm(&g,r.nonce,8,301,false,DSR_OTA_POWER_READY));
 CHECK(dsr_maint_grant_cancel(&g,r.nonce));CHECK(!dsr_maint_grant_take(&g,&r,8,302));
 CHECK(!dsr_maint_grant_cancel(&g,wrong));r.nonce[0]++;
 CHECK(dsr_maint_grant_begin(&g,&r,8,400)==DSR_OK);dsr_maint_grant_invalidate_session(&g,(uint8_t[32]){9});CHECK(g.state==DSR_GRANT_WAIT);
 dsr_maint_grant_invalidate_session(&g,r.session);CHECK(g.state==DSR_GRANT_DONE);
 memset(&g,0,sizeof(g));r.op=0;CHECK(dsr_maint_grant_begin(&g,&r,8,1)==DSR_INVALID);
 r.op=DSR_MAINT_EXPORT;CHECK(dsr_maint_grant_begin(&g,&r,8,1)==DSR_INVALID); /* missing authenticated session */
 r.session[0]=1;memset(r.nonce,0,16);CHECK(dsr_maint_grant_begin(&g,&r,8,1)==DSR_INVALID);
 printf("PASS: %u typed physical grant checks; no IO\n",checks);return 0;
}
