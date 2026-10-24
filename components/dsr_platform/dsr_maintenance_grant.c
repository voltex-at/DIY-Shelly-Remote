#include "dsr_maintenance_grant.h"
#include <string.h>
static bool nonzero(const uint8_t *p,size_t n){uint8_t v=0;while(n--)v|=*p++;return v!=0;}
bool dsr_maint_grant_live(dsr_maint_grant_t *g,uint64_t generation,uint32_t now)
{
 if(!g||(g->state!=DSR_GRANT_WAIT&&g->state!=DSR_GRANT_READY))return false;
 if(g->generation!=generation||(uint32_t)(now-g->started)>=DSR_MAINT_CONSENT_MS){g->state=DSR_GRANT_DONE;g->result=DSR_CONFLICT;return false;}return true;
}
dsr_result_t dsr_maint_grant_begin(dsr_maint_grant_t *g,const dsr_maint_request_t *r,uint64_t generation,uint32_t now)
{
 if(!g||!r||!generation||r->op<DSR_MAINT_EXPORT||r->op>DSR_MAINT_RESET||!nonzero(r->nonce,16)||
  (r->op!=DSR_MAINT_RESET&&!nonzero(r->session,32))||
  (r->op==DSR_MAINT_IMPORT_COMMIT?!nonzero(r->candidate_sha256,32):nonzero(r->candidate_sha256,32)))return DSR_INVALID;
 if(g->state==DSR_GRANT_RUNNING||dsr_maint_grant_live(g,generation,now))return DSR_BUSY;
 if(g->state!=DSR_GRANT_EMPTY&&!memcmp(r->nonce,g->request.nonce,16))return DSR_CONFLICT;
 *g=(dsr_maint_grant_t){.request=*r,.generation=generation,.started=now,.state=DSR_GRANT_WAIT};return DSR_OK;
}
bool dsr_maint_grant_confirm(dsr_maint_grant_t *g,const uint8_t id[16],uint64_t generation,uint32_t now,bool usb,dsr_ota_power_result_t power)
{
 if(!g||!id||memcmp(id,g->request.nonce,16)||!dsr_maint_grant_live(g,generation,now)||g->state!=DSR_GRANT_WAIT)return false;
 bool needs_power=g->request.op==DSR_MAINT_IMPORT_COMMIT||g->request.op==DSR_MAINT_RESET;
 g->usb_required=needs_power&&power==DSR_OTA_POWER_UNKNOWN;
 if(needs_power&&power!=DSR_OTA_POWER_READY){
  g->result=power==DSR_OTA_POWER_LOW?DSR_OFFLINE:power==DSR_OTA_POWER_UNKNOWN?DSR_INVALID:DSR_BUSY;
  if(power!=DSR_OTA_POWER_UNKNOWN||!usb)return false;
 }
 g->state=DSR_GRANT_READY;g->result=DSR_OK;return true;
}
bool dsr_maint_grant_take(dsr_maint_grant_t *g,const dsr_maint_request_t *r,uint64_t generation,uint32_t now)
{
 if(!g||!r||r->op!=g->request.op||memcmp(r->nonce,g->request.nonce,16)||memcmp(r->session,g->request.session,32)||memcmp(r->candidate_sha256,g->request.candidate_sha256,32)||
  !dsr_maint_grant_live(g,generation,now)||g->state!=DSR_GRANT_READY)return false;
 g->state=DSR_GRANT_RUNNING;return true;
}
bool dsr_maint_grant_cancel(dsr_maint_grant_t *g,const uint8_t id[16])
{
 if(!g||!id||memcmp(id,g->request.nonce,16)||(g->state!=DSR_GRANT_WAIT&&g->state!=DSR_GRANT_READY))return false;
 g->state=DSR_GRANT_DONE;g->result=DSR_CONFLICT;return true;
}
void dsr_maint_grant_invalidate_session(dsr_maint_grant_t *g,const uint8_t session[32])
{if(g&&session&&!memcmp(g->request.session,session,32))(void)dsr_maint_grant_cancel(g,g->request.nonce);}
void dsr_maint_grant_finish(dsr_maint_grant_t *g,dsr_result_t r)
{if(g&&g->state==DSR_GRANT_RUNNING){g->state=DSR_GRANT_DONE;g->result=r;}}
