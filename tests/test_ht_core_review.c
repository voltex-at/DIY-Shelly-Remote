/* Genuine HTTP/parser -> genuine fleet. The existing SDK/NVS fixture is MODEL. */
#define main existing_platform_main
#include "test_platform.c"
#undef main

static const char ht_review_identity[]="{\"gen\":3,\"id\":\"shellyhtg3-AABBCCDDEEFF\",\"model\":\"S3SN-0U12A\"}";
static const char ht_review_good[]="{\"temperature:0\":{\"id\":0,\"tC\":23.5},\"humidity:0\":{\"id\":0,\"rh\":41.5},\"devicepower:0\":{\"id\":0,\"battery\":{\"percent\":82.5},\"external\":{\"present\":false}}}";

static void lost_bundle_preserves_last_good(const char *response,dsr_result_t expected)
{
 dsr_connection_t c=connection();c.device.endpoint_kind=DSR_ENDPOINT_HT;c.device.generation=DSR_GEN2_PLUS;c.layout_id=44;
 strcpy(c.device.device_id,"shellyhtg3-AABBCCDDEEFF");
 step_t data[]={{"/shelly",ht_review_identity,200,false,false},{"/rpc/Shelly.GetStatus",ht_review_good,200,false,false}};
 fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&c)==DSR_OK&&used==2);
 CHECK(c.device.online&&c.telemetry.has_temperature&&c.telemetry.has_humidity&&c.telemetry.has_battery);
 dsr_telemetry_t last=c.telemetry;dsr_fleet_t fleet={0};CHECK(dsr_fleet_upsert(&fleet,&c,100)==DSR_OK);
 dsr_job_t job;unsigned at;CHECK(dsr_fleet_next(&fleet,60100,true,&job,&at)&&at==0&&job.kind==DSR_JOB_PROBE);
 job.id=900;dsr_fleet_started(&fleet,at,&job);
 data[1].body=response;fixture(data,2,HTTP_AUTH_TYPE_DIGEST);dsr_result_t result=dsr_http_refresh(&job.connection);CHECK(result==expected&&used==2);
 dsr_event_t event={.id=job.id,.result=result,.device=job.connection.device,.telemetry=job.connection.telemetry};
 CHECK(dsr_fleet_complete(&fleet,at,&event,70100));
 CHECK(!fleet.slots[at].connection.device.online&&fleet.slots[at].result==expected);
 fprintf(stderr,"H&T result=%u sample_valid=%u sample_ms=%u temperature=%.1f RH=%.1f battery=%.1f\n",(unsigned)result,(unsigned)fleet.slots[at].sample_valid,(unsigned)fleet.slots[at].sample_ms,fleet.slots[at].connection.telemetry.temperature_c,fleet.slots[at].connection.telemetry.humidity_pct,fleet.slots[at].connection.telemetry.battery_pct);
 CHECK(fleet.slots[at].sample_valid&&fleet.slots[at].sample_ms==100);
 CHECK(!memcmp(&fleet.slots[at].connection.telemetry,&last,sizeof(last)));
}

int main(void)
{
 lost_bundle_preserves_last_good("{",DSR_BAD_RESPONSE);
 lost_bundle_preserves_last_good("{\"temperature:0\":[],\"humidity:0\":{\"id\":0,\"rh\":50},\"devicepower:0\":{\"id\":0}}",DSR_UNSUPPORTED);
 puts("PASS: genuine HT HTTP malformed/missing bundle retains the last-good snapshot and original receipt time; SDK/NVS MODEL");return 0;
}
