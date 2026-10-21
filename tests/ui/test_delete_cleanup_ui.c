/* Real LCD/reconciliation path; NVS/worker are explicit delayed ACK providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void press_remove(const char *text)
{
 lv_obj_t *l=find_label(lv_screen_active(),text);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(b,&a);CHECK(a.y1>=0&&a.y2<480);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_enable();cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"theme",cJSON_CreateString("cyber_blue"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Kanał pierwszy\",\"host\":\"192.0.2.20\",\"channel\":0,\"kind\":\"relay\",\"room\":0},{\"id\":2,\"name\":\"Drugi kanał\",\"host\":\"192.0.2.20\",\"channel\":1,\"kind\":\"relay\",\"room\":0}]"));publish_document(r);
 boot_loaded_count=2;for(unsigned i=0;i<2;i++){boot_loaded[i]=(dsr_connection_t){.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH,.channel=(uint8_t)i,.host="192.0.2.20",.device_id="shelly-test-two-channels"}};}
 dsr_ui_init();tick();CHECK(dsr_network_layout_reserved(1)&&dsr_network_layout_reserved(2));
 dsr_ui_edit_local_device(1);press_remove("Usuń urządzenie");hold_maintenance=true;press_remove("Usuń");dsr_ui_workspace_tick();tick();dsr_ui_workspace_tick();
 CHECK(unbind_jobs==1&&maintenance_ready&&dsr_network_removal_status(1)==DSR_BUSY&&dsr_network_layout_reserved(2));CHECK(find_label(lv_screen_active(),"Karta usunięta. Kończenie usuwania powiązania…"));screenshot("native-final-delete-nvs-pending");
 maintenance_result=DSR_IO;hold_maintenance=false;tick();dsr_ui_workspace_tick();CHECK(dsr_network_removal_status(1)==DSR_IO&&dsr_network_layout_reserved(2));CHECK(find_label(lv_screen_active(),"Karta usunięta. Błąd zapisu powiązania. Pilot ponowi próbę."));screenshot("native-final-delete-nvs-retry");unsigned n=unbind_jobs;tick();CHECK(unbind_jobs==n);
 maintenance_result=DSR_OK;lv_tick_inc(5100);tick();dsr_ui_workspace_tick();CHECK(dsr_network_removal_status(1)==DSR_OK&&!dsr_network_layout_reserved(1)&&dsr_network_layout_reserved(2));CHECK(find_label(lv_screen_active(),"Urządzenie usunięte."));screenshot("native-final-delete-nvs-success");
 CHECK(!requests);printf("PASS: %u device cleanup checks; exact channel, delayed/failed NVS ACK, bounded retry, sibling retained; no physical device I/O\n",checks);lv_deinit();return 0;
}
