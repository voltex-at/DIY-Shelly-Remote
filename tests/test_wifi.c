/* Actual Wi-Fi lifecycle source against deterministic ESP-IDF event/driver
 * fixtures. No radio or cryptographic authentication is emulated as verified. */
#include "dsr_platform.h"
#include "dsr_activity_internal.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}} while(0)
const char test_wifi_base[]="wifi",test_ip_base[]="ip";
struct test_event_group {EventBits_t bits;};
struct test_semaphore {bool locked;};
struct test_timer {void (*callback)(void *);bool armed;uint64_t delay;};
static struct test_event_group group;
static struct test_semaphore semaphores[2];static unsigned semaphore_count;
static struct test_timer timer;
static esp_netif_t netif;
static esp_event_handler_t handler;
static wifi_config_t config;
static wifi_ap_record_t aps[48];static uint16_t ap_count;
static bool scan_error,records_error,save_error,pending_connect,driver_memory,inject_old_ip;
static unsigned connects,saves,stops,clears;
static enum {SUCCESS,BAD_PASSWORD,NOT_FOUND,TIMEOUT,CANCELLED} outcome;
static dsr_settings_t persisted;
static unsigned sets,starts,fail_set,fail_start,fail_stop,fail_connect,missing_start_at,cancel_stop_at;
static bool visit_test_mode,missing_stop,cancel_after_action;
static unsigned ap_outcome,home_outcome,action_calls;
static dsr_result_t action_outcome;

static void got_ip(void)
{ip_event_got_ip_t event={.esp_netif=&netif,.ip_info={.ip={{192,168,1,42}}}};handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,&event);}
static void disconnect(unsigned reason)
{wifi_event_sta_disconnected_t event={.reason=(uint16_t)reason};handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,&event);}
EventGroupHandle_t xEventGroupCreate(void) {return &group;}
EventBits_t xEventGroupSetBits(EventGroupHandle_t g,EventBits_t b) {g->bits|=b;return g->bits;}
EventBits_t xEventGroupClearBits(EventGroupHandle_t g,EventBits_t b) {g->bits&=~b;return g->bits;}
EventBits_t xEventGroupGetBits(EventGroupHandle_t g) {return g->bits;}
EventBits_t xEventGroupWaitBits(EventGroupHandle_t g,EventBits_t b,int clear,int all,TickType_t timeout)
{
    (void)all;(void)timeout;
    if(!(g->bits&b)&&pending_connect) {
        pending_connect=false;
        unsigned observed=visit_test_mode?(config.sta.bssid_set?ap_outcome:home_outcome):(unsigned)outcome;
        if(observed==SUCCESS)got_ip();
        else if(observed==BAD_PASSWORD)disconnect(WIFI_REASON_AUTH_FAIL);
        else if(observed==NOT_FOUND)disconnect(WIFI_REASON_NO_AP_FOUND);
        else if(observed==CANCELLED)dsr_wifi_cancel_connect();
    }
    EventBits_t result=g->bits;if(clear)g->bits&=~b;return result;
}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {CHECK(semaphore_count<2);return &semaphores[semaphore_count++];}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t timeout)
{if(s->locked){CHECK(timeout==0);return pdFALSE;}s->locked=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s) {CHECK(s->locked);s->locked=false;return pdTRUE;}
esp_err_t esp_netif_init(void) {return ESP_OK;}
esp_netif_t *esp_netif_create_default_wifi_sta(void) {return &netif;}
esp_err_t esp_event_loop_create_default(void) {return ESP_OK;}
esp_err_t esp_event_handler_register(esp_event_base_t base,int32_t id,esp_event_handler_t cb,void *arg)
{(void)base;(void)id;(void)arg;handler=cb;return ESP_OK;}
esp_err_t esp_timer_create(const esp_timer_create_args_t *args,esp_timer_handle_t *out)
{timer.callback=args->callback;*out=&timer;return ESP_OK;}
esp_err_t esp_timer_start_once(esp_timer_handle_t t,uint64_t delay) {t->armed=true;t->delay=delay;return ESP_OK;}
esp_err_t esp_timer_stop(esp_timer_handle_t t) {t->armed=false;return ESP_OK;}
esp_err_t esp_wifi_init(const wifi_init_config_t *init) {(void)init;return ESP_OK;}
esp_err_t esp_wifi_set_storage(int storage) {CHECK(storage==WIFI_STORAGE_RAM);return ESP_OK;}
esp_err_t esp_wifi_set_mode(int mode) {CHECK(mode==WIFI_MODE_STA);return ESP_OK;}
esp_err_t esp_wifi_set_config(int interface,const wifi_config_t *c)
{(void)interface;if(++sets==fail_set)return ESP_FAIL;config=*c;return ESP_OK;}
esp_err_t esp_wifi_start(void)
{if(++starts==fail_start)return ESP_FAIL;if(starts!=missing_start_at)handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_START,NULL);return ESP_OK;}
esp_err_t esp_wifi_stop(void)
{if(++stops==fail_stop)return ESP_FAIL;pending_connect=false;if(inject_old_ip)got_ip();if(!missing_stop)handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_STOP,NULL);if(stops==cancel_stop_at)dsr_wifi_cancel_connect();return ESP_OK;}
esp_err_t esp_wifi_connect(void) {if(++connects==fail_connect)return ESP_FAIL;pending_connect=true;return ESP_OK;}
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *scan,bool block)
{CHECK(block&&!scan->show_hidden);driver_memory=true;return scan_error?ESP_FAIL:ESP_OK;}
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *count,wifi_ap_record_t *records)
{if(records_error)return ESP_FAIL;CHECK(*count>=ap_count);*count=ap_count;memcpy(records,aps,ap_count*sizeof(*aps));driver_memory=false;return ESP_OK;}
esp_err_t esp_wifi_clear_ap_list(void) {driver_memory=false;clears++;return ESP_OK;}
esp_err_t dsr_settings_save(const dsr_settings_t *s) {saves++;if(save_error)return ESP_FAIL;persisted=*s;return ESP_OK;}

static dsr_settings_t candidate(const char *name)
{dsr_settings_t s={.brightness=70,.idle_seconds=60,.wifi_security=DSR_WIFI_WPA2,.password="test-password"};snprintf(s.ssid,sizeof(s.ssid),"%s",name);return s;}
static dsr_result_t connect_candidate(const dsr_settings_t *s,dsr_wifi_status_t *detail)
{dsr_wifi_prepare_connect();return dsr_wifi_connect_saved(s,detail);}

static dsr_wifi_ap_target_t target(void)
{dsr_wifi_ap_target_t t={.settings={.ssid="Shelly-HOST-FIXTURE",.wifi_security=DSR_WIFI_OPEN},.bssid={2,0,0,0,0,1}};return t;}
static dsr_result_t ap_action(void *context)
{
    dsr_wifi_ap_target_t *t=context;action_calls++;
    CHECK(!dsr_wifi_connected()); /* foreign DHCP is not home-LAN readiness */
    dsr_wifi_info_t snapshot;dsr_wifi_get_info(&snapshot);
    CHECK(!snapshot.connected&&!snapshot.ip[0]&&snapshot.status==DSR_WIFI_CONNECTING);
    CHECK(config.sta.bssid_set&&!memcmp(config.sta.bssid,t->bssid,6));
    CHECK(config.sta.threshold.authmode==(t->settings.wifi_security==DSR_WIFI_WPA3?WIFI_AUTH_WPA3_PSK:WIFI_AUTH_OPEN));
    CHECK(config.sta.pmf_cfg.required==(t->settings.wifi_security==DSR_WIFI_WPA3));
    CHECK(!dsr_wifi_ap_cancelled());CHECK(!dsr_activity_restart_reserve());
    dsr_wifi_scan_t scan;dsr_wifi_status_t detail;dsr_wifi_ap_visit_t nested;
    CHECK(dsr_wifi_scan(&scan)==DSR_BUSY);
    CHECK(dsr_wifi_connect_saved(&t->settings,&detail)==DSR_BUSY);
    CHECK(dsr_wifi_visit_ap(t,true,ap_action,t,&nested)==DSR_BUSY);
    unsigned before=connects;timer.callback(NULL);CHECK(connects==before);
    if(cancel_after_action)dsr_wifi_cancel_connect();
    return action_outcome;
}
static void visit_defaults(void)
{
    dsr_wifi_prepare_connect();visit_test_mode=true;ap_outcome=home_outcome=SUCCESS;
    action_outcome=DSR_OK;cancel_after_action=false;
    fail_set=fail_start=fail_stop=fail_connect=missing_start_at=cancel_stop_at=0;missing_stop=false;
}
static void admitted_again(void)
{CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void visit_regression(void)
{
    dsr_wifi_ap_target_t t=target();dsr_wifi_ap_visit_t report;dsr_wifi_status_t detail;
    dsr_settings_t home=candidate("Home before AP");outcome=SUCCESS;
    CHECK(connect_candidate(&home,&detail)==DSR_OK);unsigned prior_saves=saves;
    visit_defaults();unsigned old_stops=stops,old_calls=action_calls;
    CHECK(dsr_wifi_visit_ap(&t,false,ap_action,&t,&report)==DSR_INVALID);
    CHECK(dsr_wifi_visit_ap(NULL,true,ap_action,&t,&report)==DSR_INVALID);
    CHECK(dsr_wifi_visit_ap(&t,true,NULL,&t,&report)==DSR_INVALID);
    CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,NULL)==DSR_INVALID);
    memset(t.bssid,0,6);CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_INVALID);
    memset(t.bssid,255,6);CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_INVALID);
    t=target();memset(t.settings.ssid,'X',sizeof(t.settings.ssid));CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_INVALID);
    t=target();memset(t.settings.password,'X',sizeof(t.settings.password));CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_INVALID);
    t=target();CHECK(stops==old_stops&&action_calls==old_calls&&saves==prior_saves);
    CHECK(dsr_activity_restart_reserve());CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_BUSY);dsr_activity_restart_cancel();
    dsr_wifi_cancel_connect();CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
    CHECK(report.ap_status==DSR_WIFI_CANCELLED&&!report.switch_attempted&&!report.action_called&&stops==old_stops);admitted_again();
    visit_defaults();CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OK);
    CHECK(report.switch_attempted&&report.action_called&&report.action_result==DSR_OK&&report.ap_status==DSR_WIFI_CONNECTED);
    CHECK(report.home_config_restored&&report.home_connected&&report.home_status==DSR_WIFI_CONNECTED);
    CHECK(dsr_wifi_connected()&&!config.sta.bssid_set&&!strcmp((char*)config.sta.ssid,home.ssid));
    CHECK(!strcmp(persisted.ssid,home.ssid)&&saves==prior_saves);admitted_again();
    visit_defaults();t.settings.wifi_security=DSR_WIFI_WPA3;strcpy(t.settings.password,"HOST-ONLY-AP-PASSWORD");
    CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OK);
    CHECK(report.action_called&&report.home_connected&&!memcmp(&persisted,&home,sizeof(home)));admitted_again();t=target();
    visit_defaults();cancel_stop_at=stops+1;old_calls=action_calls;
    CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
    CHECK(!report.action_called&&action_calls==old_calls&&report.ap_status==DSR_WIFI_CANCELLED&&report.home_connected);admitted_again();
    for(unsigned failure=BAD_PASSWORD;failure<=CANCELLED;failure++){
        visit_defaults();ap_outcome=failure;old_calls=action_calls;
        CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
        CHECK(!report.action_called&&action_calls==old_calls&&report.home_config_restored&&report.home_connected);
        CHECK(report.ap_status==(failure==BAD_PASSWORD?DSR_WIFI_BAD_PASSWORD:failure==NOT_FOUND?DSR_WIFI_NOT_FOUND:failure==TIMEOUT?DSR_WIFI_TIMED_OUT:DSR_WIFI_CANCELLED));
        CHECK(saves==prior_saves);admitted_again();
    }
    for(unsigned failure=DSR_INVALID;failure<=DSR_RATE_LIMITED;failure++){
        visit_defaults();action_outcome=(dsr_result_t)failure;
        CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==(dsr_result_t)failure);
        CHECK(report.action_called&&report.action_result==(dsr_result_t)failure&&report.home_connected);
        CHECK(saves==prior_saves);admitted_again();
    }
    visit_defaults();cancel_after_action=true;
    CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
    CHECK(report.action_called&&report.action_result==DSR_OK&&report.ap_status==DSR_WIFI_CANCELLED&&report.home_connected);admitted_again();
    /* Each pre-callback driver error restores home, never invokes action. */
    for(unsigned fault=0;fault<5;fault++){
        visit_defaults();old_calls=action_calls;
        if(fault==0)fail_stop=stops+1;
        if(fault==1)fail_set=sets+1;
        if(fault==2)fail_start=starts+1;
        if(fault==3)fail_connect=connects+1;
        if(fault==4)missing_start_at=starts+1;
        CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
        CHECK(report.ap_status==DSR_WIFI_DRIVER_ERROR&&!report.action_called&&action_calls==old_calls&&report.home_connected);admitted_again();
    }
    for(unsigned failure=BAD_PASSWORD;failure<=TIMEOUT;failure++){
        visit_defaults();home_outcome=failure;
        CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_OFFLINE);
        CHECK(report.action_called&&report.action_result==DSR_OK&&report.home_config_restored&&!report.home_connected&&!dsr_wifi_connected());
        CHECK(!config.sta.bssid_set&&!strcmp((char*)config.sta.ssid,home.ssid));admitted_again();
    }
    /* Late restoration failures cannot automatically reconnect to the AP.
     * Explicit saved-network recovery is allowed and never saves AP data. */
    for(unsigned fault=0;fault<5;fault++){
        visit_defaults();if(fault==0)fail_stop=stops+2;
        if(fault==1)fail_set=sets+2;
        if(fault==2)fail_start=starts+2;
        if(fault==3)fail_connect=connects+2;
        if(fault==4)missing_start_at=starts+2;
        CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_IO);
        CHECK(report.action_called&&!report.home_config_restored&&!report.home_connected&&report.home_status==DSR_WIFI_DRIVER_ERROR);
        CHECK(!dsr_wifi_connected()&&!timer.armed&&saves==prior_saves);
        dsr_wifi_scan_t scan;CHECK(dsr_wifi_scan(&scan)==DSR_BUSY);
        unsigned before=connects;timer.callback(NULL);CHECK(connects==before);admitted_again();
        visit_defaults();CHECK(connect_candidate(&home,&detail)==DSR_OK);prior_saves=saves;
    }
    visit_defaults();missing_stop=true;old_calls=action_calls;unsigned old_sets=sets;
    CHECK(dsr_wifi_visit_ap(&t,true,ap_action,&t,&report)==DSR_IO);
    CHECK(!report.action_called&&action_calls==old_calls&&sets==old_sets&&!report.home_config_restored&&!timer.armed);
    /* A second entry may not bypass an unobserved STOP fence just because
     * driver_started is already false. A delayed event unlocks recovery. */
    CHECK(dsr_wifi_connect_saved(&home,&detail)!=DSR_OK);CHECK(sets==old_sets);
    missing_stop=false;handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_STOP,NULL);
    visit_defaults();CHECK(connect_candidate(&home,&detail)==DSR_OK);admitted_again();
    visit_test_mode=false;
    puts("AP visit: genuine lifecycle/RAM-only settings/BSSID, consent/cancel, exclusive callback, restart admission, faulted restoration and retained STOP fence; SDK/radio/boot MODEL");
}

int main(int argc,char **argv)
{
    dsr_wifi_ap_target_t uninitialized_target=target();dsr_wifi_ap_visit_t uninitialized_report;
    CHECK(dsr_wifi_visit_ap(&uninitialized_target,true,ap_action,&uninitialized_target,&uninitialized_report)==DSR_INVALID);
    if(argc>1){
        if(!strcmp(argv[1],"start-failure")){
            fail_start=1;CHECK(dsr_wifi_start(NULL)==ESP_FAIL);unsigned before_sets=sets;
            CHECK(dsr_wifi_visit_ap(&uninitialized_target,true,ap_action,&uninitialized_target,&uninitialized_report)==DSR_INVALID);
            CHECK(sets==before_sets&&stops==0&&action_calls==0&&saves==0);admitted_again();
        }else{
            CHECK(!strcmp(argv[1],"boot-home"));
            dsr_settings_t home=candidate("Persisted home");CHECK(dsr_wifi_start(&home)==ESP_OK);
            CHECK(!strcmp((char*)config.sta.ssid,home.ssid)&&!config.sta.bssid_set&&saves==0);
            CHECK(!dsr_wifi_ap_cancelled());admitted_again();
        }
        printf("PASS: %u Wi-Fi fresh-process checks (%s), reboot/SDK providers MODEL\n",checks,argv[1]);return 0;
    }
    CHECK(dsr_wifi_start(NULL)==ESP_OK);CHECK(!dsr_wifi_connected());CHECK(connects==0);
    CHECK(dsr_wifi_start(NULL)==ESP_ERR_INVALID_STATE);
    /* No saved home network: return to idle RAM configuration, not a fake
     * home-connected result or a persisted foreign AP. */
    visit_defaults();dsr_wifi_ap_target_t initial_target=target();dsr_wifi_ap_visit_t initial_report;
    CHECK(dsr_wifi_visit_ap(&initial_target,true,ap_action,&initial_target,&initial_report)==DSR_OK);
    CHECK(initial_report.home_config_restored&&!initial_report.home_connected&&initial_report.home_status==DSR_WIFI_IDLE);
    CHECK(!config.sta.ssid[0]&&!config.sta.bssid_set&&saves==0&&!dsr_wifi_connected());
    visit_test_mode=false;admitted_again();
    aps[0]=(wifi_ap_record_t){.ssid="Home",.rssi=-70,.authmode=WIFI_AUTH_WPA2_PSK};
    aps[1]=(wifi_ap_record_t){.ssid="Guest",.rssi=-60,.authmode=WIFI_AUTH_OPEN};
    aps[2]=(wifi_ap_record_t){.ssid="Home",.rssi=-31,.authmode=WIFI_AUTH_WPA2_PSK};
    aps[3]=(wifi_ap_record_t){.ssid="Office",.rssi=-50,.authmode=WIFI_AUTH_WPA2_ENTERPRISE};
    aps[4]=(wifi_ap_record_t){.rssi=-20,.authmode=WIFI_AUTH_OPEN};ap_count=5;
    dsr_wifi_scan_t scan;CHECK(dsr_wifi_scan(&scan)==DSR_OK);CHECK(scan.count==3);
    CHECK(!strcmp(scan.items[0].ssid,"Home")&&scan.items[0].rssi==-31);
    CHECK(scan.items[1].security==DSR_WIFI_UNSUPPORTED);CHECK(!driver_memory&&clears==1);
    scan_error=true;CHECK(dsr_wifi_scan(&scan)==DSR_IO&&!driver_memory);scan_error=false;
    records_error=true;CHECK(dsr_wifi_scan(&scan)==DSR_IO&&!driver_memory);records_error=false;
    for(unsigned i=0;i<48;i++){snprintf((char*)aps[i].ssid,sizeof(aps[i].ssid),"AP-%02u",i);aps[i].rssi=(int8_t)(-90+(int)i);aps[i].authmode=WIFI_AUTH_WPA2_PSK;}
    ap_count=48;CHECK(dsr_wifi_scan(&scan)==DSR_OK&&scan.count==20);
    for(unsigned i=1;i<scan.count;i++)CHECK(scan.items[i-1].rssi>=scan.items[i].rssi);
    dsr_wifi_status_t detail;dsr_settings_t s=candidate("Home");outcome=BAD_PASSWORD;
    CHECK(connect_candidate(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_BAD_PASSWORD);CHECK(saves==0&&!dsr_wifi_connected());
    outcome=SUCCESS;CHECK(connect_candidate(&s,&detail)==DSR_OK&&detail==DSR_WIFI_CONNECTED);CHECK(saves==1&&dsr_wifi_connected());
    CHECK(!strcmp(persisted.ssid,"Home"));CHECK(config.sta.threshold.authmode==WIFI_AUTH_WPA2_PSK);
    dsr_wifi_info_t info;dsr_wifi_get_info(&info);CHECK(!strcmp(info.ip,"192.168.1.42"));
    unsigned prior_stops=stops;CHECK(dsr_wifi_scan(&scan)==DSR_OK);CHECK(stops==prior_stops&&dsr_wifi_connected());
    s=candidate("Other");outcome=BAD_PASSWORD;inject_old_ip=true;
    CHECK(connect_candidate(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_BAD_PASSWORD);CHECK(saves==1);
    CHECK(!strcmp(persisted.ssid,"Home")&&!memcmp(config.sta.ssid,"Home",4));CHECK(!dsr_wifi_connected());
    outcome=NOT_FOUND;CHECK(connect_candidate(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_NOT_FOUND);CHECK(saves==1);
    outcome=TIMEOUT;CHECK(connect_candidate(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_TIMED_OUT);CHECK(saves==1);
    outcome=CANCELLED;CHECK(connect_candidate(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_CANCELLED);CHECK(saves==1);
    dsr_wifi_prepare_connect();dsr_wifi_cancel_connect();outcome=SUCCESS;
    CHECK(dsr_wifi_connect_saved(&s,&detail)==DSR_OFFLINE&&detail==DSR_WIFI_CANCELLED);CHECK(saves==1);
    save_error=true;CHECK(connect_candidate(&s,&detail)==DSR_IO&&detail==DSR_WIFI_STORAGE_ERROR);save_error=false;
    CHECK(!strcmp(persisted.ssid,"Home"));
    s=candidate("WPA3 network");s.wifi_security=DSR_WIFI_WPA3;strcpy(s.password,"short");
    CHECK(connect_candidate(&s,&detail)==DSR_OK);CHECK(config.sta.threshold.authmode==WIFI_AUTH_WPA3_PSK&&config.sta.pmf_cfg.required);
    disconnect(WIFI_REASON_AUTH_FAIL);CHECK(!timer.armed); /* auth failure does not loop */
    CHECK(saves==3);s.wifi_security=DSR_WIFI_OPEN;s.password[0]=0;
    CHECK(connect_candidate(&s,&detail)==DSR_OK&&config.sta.threshold.authmode==WIFI_AUTH_OPEN);
    disconnect(WIFI_REASON_NO_AP_FOUND);CHECK(timer.armed&&timer.delay==1000000);
    unsigned before=connects;timer.callback(NULL);CHECK(connects==before+1);
    visit_regression();
    puts("Wi-Fi fixture: real lifecycle, scan cap/dedupe, old-event fence, credentials saved only after IP, rollback/cancel, security and reconnect");
    printf("PASS: %u Wi-Fi lifecycle checks\n",checks);return 0;
}
