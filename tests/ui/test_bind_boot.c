/* Fresh-process restart tests. Actual LVGL/network/fleet; private NVS,
 * workspace mount and SDK providers are explicit fixtures. No device I/O. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_runtime_capture(bool enabled);
size_t test_runtime_count(void);
const dsr_runtime_device_t *test_runtime_at(size_t index);
static void boot_entry(cJSON *r,unsigned id,const char *name,const char *host)
{
 cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"id",id);cJSON_AddStringToObject(d,"name",name);cJSON_AddStringToObject(d,"kind","relay");cJSON_AddStringToObject(d,"host",host);cJSON_AddNumberToObject(d,"channel",0);cJSON_AddNumberToObject(d,"room",0);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"devices"),d);
 cJSON *tile=cJSON_CreateObject();cJSON_AddNumberToObject(tile,"id",id);cJSON_AddStringToObject(tile,"type","device");cJSON_AddNumberToObject(tile,"ref",id);cJSON_AddNumberToObject(tile,"span",2);cJSON_AddNumberToObject(tile,"height",1);cJSON_AddNumberToObject(tile,"fields",255);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles"),tile);
}
int main(int argc,char **argv)
{
 CHECK(argc==3);unsigned mode=(unsigned)strtoul(argv[2],NULL,10);CHECK(mode<4);output=argv[1];
 lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");strcpy(stored.password,"fixture-only-password");stored.wifi_security=DSR_WIFI_WPA2;wifi.connected=true;test_runtime_capture(true);
 dsr_connection_t b={.device={.host="192.168.1.51",.name="Boot B",.device_id="shelly-B",.generation=DSR_GEN2_PLUS},.credentials={.password="retained-private"},.layout_id=20};
 dsr_connection_t c={.device={.host="192.168.1.70",.name="Boot pending C",.device_id="shelly-C",.generation=DSR_GEN2_PLUS},.credentials={.password="pending-private"}};
 if(mode==0){boot_loaded[0]=b;boot_loaded[1]=c;boot_loaded_count=2;} /* old schema2 pending, unavailable SPIFFS */
 if(mode==1){boot_loaded[0]=b;boot_loaded_count=1;test_workspace_enable();cJSON *r=room_document();boot_entry(r,20,"Boot B",b.device.host);boot_entry(r,30,"Boot pending C",c.device.host);publish_document(r);} /* workspace ACK, crash before private save */
 if(mode==2){strcpy(c.device.name,"Boot legacy");c.legacy_fallback=true;boot_loaded[0]=c;boot_loaded_count=1;} /* genuine v1 origin only */
 if(mode==3){boot_loaded[0]=b;boot_loaded[1]=c;boot_loaded_count=2;test_workspace_enable();cJSON *r=room_document();boot_entry(r,20,"Boot B",b.device.host);boot_entry(r,30,"Boot pending C",c.device.host);publish_document(r);} /* old schema2 pending cannot self-publish through locator match */
 dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&publish_jobs==0&&unbind_jobs==0);
 CHECK(!strcmp(submitted.connection.device.device_id,mode==2?"shelly-C":"shelly-B"));
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(test_runtime_count()==1&&test_runtime_at(0)&&!strcmp(test_runtime_at(0)->device.device_id,mode==2?"shelly-C":"shelly-B"));
 if(mode==0)CHECK(find_label(lv_screen_active(),"Boot B")&&!find_label(lv_screen_active(),"Boot pending C")&&job_counts[DSR_JOB_SET_OUTPUT]==0);
 if(mode==1||mode==3){click("Boot pending C");lv_obj_t *power=power_target(lv_screen_active());CHECK(power&&lv_obj_has_state(power,LV_STATE_DISABLED));unsigned before=requests;lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(requests==before&&job_counts[DSR_JOB_SET_OUTPUT]==0&&!dsr_network_layout_reserved(30));dsr_network_device_settings(30);CHECK(find_label(lv_screen_active(),"Tożsamość niepotwierdzona")&&find_label(lv_screen_active(),"Brak aktywnego połączenia")&&!find_label(lv_screen_active(),"shelly-C"));click("Odśwież status");CHECK(requests==before&&job_counts[DSR_JOB_SET_OUTPUT]==0);}
 if(mode==2){click("Boot legacy");lv_obj_t *power=power_target(lv_screen_active());CHECK(power&&!lv_obj_has_state(power,LV_STATE_DISABLED));lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT&&job_counts[DSR_JOB_SET_OUTPUT]==1);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 CHECK(!in_flight&&!maintenance_ready&&publish_jobs==0);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 if(mode==3){dsr_network_device_settings(30);CHECK(find_label(lv_screen_active(),"Tożsamość niepotwierdzona")&&find_label(lv_screen_active(),"Brak aktywnego połączenia"));CHECK(job_counts[DSR_JOB_SET_OUTPUT]==0&&publish_jobs==0);}
 screenshot(mode==0?"244-bind-boot-unpublished-quarantine":mode==1?"245-bind-boot-ACK-without-private":mode==2?"246-bind-boot-legacy-origin":"248-bind-boot-pending-matching-workspace");
 printf("PASS: %u actual LVGL BIND restart checks, mode%u; NVS/workspace/SDK MODEL; no physical restart or writes\n",checks,mode);lv_deinit();return 0;
}
