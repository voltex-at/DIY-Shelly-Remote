/* Genuine typed HTTP transaction + genuine actor model. Only SDK transport
 * and clock are providers reused from the platform fixture. */
#define main platform_fixture_main
#include "test_platform.c"
#undef main
#include "../components/dsr_platform/dsr_actions_private.h"

static dsr_actions_model_t actor;
static dsr_action_key_t frozen;
static unsigned permissions, deny_after_ack;
static uint32_t actor_request=101;
static dsr_result_t actor_permission(void)
{
    permissions++;
    if (deny_after_ack && permissions == deny_after_ack)
        return DSR_IDENTITY_MISMATCH; /* exact workspace mapping removed */
    return dsr_actions_model_worker_allowed(&actor, actor_request, &frozen, 11)
        ? DSR_OK : DSR_BUSY;
}
static void actor_write(void){CHECK(dsr_actions_model_write_attempted(&actor,101));}
int main(void)
{
    unsigned wrong = 0;
    for (unsigned mode = 0; mode < 4; mode++) {
        bool cover = mode >= 2;
        dsr_connection_t c = connection();
        c.device.generation = DSR_GEN2_PLUS;
        c.device.has_switch = !cover;
        c.device.endpoint_kind = cover ? DSR_ENDPOINT_COVER : DSR_ENDPOINT_SWITCH;
        strcpy(c.device.device_id, "shellyplus1-AABBCCDDEEFF");
        frozen = (dsr_action_key_t){.layout_id=1, .generation=2,
            .kind=c.device.endpoint_kind, .channel=0};
        strcpy(frozen.device_id, c.device.device_id);
        strcpy(frozen.host, c.device.host);
        dsr_action_step_t plan[3] = {
            {.key=frozen, .command=cover ? DSR_ACTION_OPEN : DSR_ACTION_ON},
            {.key=frozen, .command=cover ? DSR_ACTION_CLOSE : DSR_ACTION_OFF},
            {.key=frozen, .command=cover ? DSR_ACTION_OPEN : DSR_ACTION_ON}};
        plan[2].key.layout_id=2;strcpy(plan[2].key.device_id,"shelly-B");
        memset(&actor, 0, sizeof(actor));
        CHECK(dsr_actions_model_begin(&actor, plan, 3, 1, 0, 17, 1, 1,
              "0123456789abcdef", 0, 1, 10));
        CHECK(dsr_actions_model_check(&actor,0,&frozen,DSR_OK,true,true,10));
        CHECK(dsr_actions_model_queued(&actor,0,101,10));
        actor_request=101;permissions=0;deny_after_ack=mode%2 ? (cover ? 4 : 3) : 0;
        CHECK(dsr_http_job_guard(actor_permission)==NULL);
        CHECK(dsr_http_job_write_hook(actor_write)==NULL);
        step_t relay[]={{"/shelly",gen2,200,false,false},
            {"/rpc/Switch.Set","{\"was_on\":false}",200,true,false},
            {"/rpc/Switch.GetStatus?id=0","{}",401,false,false}};
        step_t covering[]={{"/shelly",gen2,200,false,false},
            {"/rpc/Cover.GetStatus?id=0",cover_ready,200,false,false},
            {"/rpc/Cover.Open","null",200,true,false},
            {"/rpc/Cover.GetStatus?id=0","{}",401,false,false}};
        cover_payload=cover ? "{\"id\":0}" : NULL;
        fixture(cover ? covering : relay, cover ? 4 : 3, HTTP_AUTH_TYPE_DIGEST);
        dsr_result_t result=cover
            ? dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_OPEN})
            : dsr_http_set_output(&c,true);
        CHECK(dsr_http_job_guard(NULL)==actor_permission);
        CHECK(dsr_http_job_write_hook(NULL)==actor_write);
        CHECK(used==(cover ? 3u : 3u-(mode%2))); /* POST ACK occurred once */
        CHECK(result==(cover ? DSR_OK : mode%2 ? DSR_IDENTITY_MISMATCH : DSR_AUTH_REQUIRED));
        CHECK(dsr_actions_model_complete(&actor,101,result,12));
        int next=dsr_actions_model_current(&actor,12);
        if(cover){
            /* ACK completes this scene step. A separately failed read gates
             * the next step; it cannot retroactively turn ACK into failure. */
            CHECK(actor.steps[0].outcome==DSR_ACTION_SUCCESS&&next==1);
            CHECK(dsr_actions_model_check(&actor,1,&frozen,DSR_OK,false,true,12));
            CHECK(dsr_actions_model_queued(&actor,1,102,12));actor_request=102;
            permissions=0;deny_after_ack=mode%2?1:0;
            CHECK(dsr_http_job_guard(actor_permission)==NULL);
            step_t read[]={{"/shelly",gen2,200,false,false},{"/rpc/Cover.GetStatus?id=0","{}",401,false,false}};
            fixture(read,2,HTTP_AUTH_TYPE_DIGEST);
            dsr_result_t gate=dsr_http_refresh(&c);
            CHECK(gate==(mode%2?DSR_IDENTITY_MISMATCH:DSR_AUTH_REQUIRED)&&used==(mode%2?0u:2u));
            CHECK(dsr_http_job_guard(NULL)==actor_permission);
            CHECK(dsr_actions_model_complete(&actor,102,gate,12));
            CHECK(actor.steps[0].outcome==DSR_ACTION_SUCCESS&&actor.steps[1].outcome==DSR_ACTION_FAILED&&dsr_actions_model_current(&actor,12)==2);
            continue;
        }
        fprintf(stderr,"%s POST ACK -> %s: result=%u outcome=%u same-endpoint=%u next=%d\n",
            cover?"Cover":"Switch",mode%2?"mapping guard":"readback401",result,
            actor.steps[0].outcome,actor.steps[1].outcome,next);
        if(actor.steps[0].outcome!=DSR_ACTION_UNCERTAIN ||
           actor.steps[1].outcome!=DSR_ACTION_CANCELLED || next!=2)wrong++;
    }
    cover_payload=NULL;
    CHECK(wrong==0);
    printf("PASS: %u genuine HTTP/model checks; relay uncertainty, independent Cover ACK and later read gate\n",checks);
    return 0;
}
