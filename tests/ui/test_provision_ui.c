/* Genuine LVGL first-format screen (#28); storage/board/boot are seams. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_provision.h"
static void advance(unsigned ms){for(unsigned t=0;t<ms;t+=250){lv_tick_inc(250);lv_timer_handler();}lv_refr_now(display);}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 /* Fresh blank partition through the real observer protocol. */
 const dsr_storage_observer_t *o=dsr_ui_storage_observer();CHECK(o&&o->begin&&o->end);
 provision_progress_value=0;o->begin(false,992,0);
 CHECK(provision_gate&&provision_task_starts==1&&provision_backlight==1&&provision_holds==1&&dsr_ui_provision_visible());
 CHECK(find_label(lv_screen_active(),"Przygotowanie pamięci")&&find_label(lv_screen_active(),"0 / 992 bloków • 0%")&&find_label(lv_screen_active(),"Nie odłączaj zasilania. Zwykle trwa 1–2 minuty."));
 CHECK(dsr_ui_provision_layout_ok());screenshot("290-format-start-pl");
 provision_progress_value=496;advance(20000);CHECK(find_label(lv_screen_active(),"496 / 992 bloków • 50%")&&find_label(lv_screen_active(),"Czas: 0:20"));
 provision_progress_value=300;advance(500);CHECK(find_label(lv_screen_active(),"496 / 992 bloków • 50%")); /* never moves backwards */
 provision_progress_value=5000;advance(500);CHECK(find_label(lv_screen_active(),"496 / 992 bloków • 50%")); /* out-of-range read ignored */
 screenshot("291-format-half-pl");
 o->end(ESP_OK);CHECK(!provision_gate&&provision_quiesces==3&&provision_holds==2&&find_label(lv_screen_active(),"Pamięć gotowa. Uruchamianie…")&&find_label(lv_screen_active(),"992 / 992 bloków • 100%"));
 unsigned reads=provision_progress_reads;advance(3000);CHECK(provision_progress_reads==reads); /* no Flash probe after end */
 screenshot("292-format-done-pl");
 dsr_ui_provision_release();CHECK(!dsr_ui_provision_visible());advance(500);
 /* Resumed remnant 109/992: old prefix does not count as new progress. */
 provision_progress_value=109;o->begin(true,992,109);advance(1000);
 CHECK(find_label(lv_screen_active(),"Wznawianie od początku…")&&find_label(lv_screen_active(),"Nie odłączaj zasilania. Partycja nie zawiera danych użytkownika. Zwykle 1–2 minuty."));
 CHECK(dsr_ui_provision_layout_ok());screenshot("293-format-resume-pl");
 provision_progress_value=110;advance(500);CHECK(find_label(lv_screen_active(),"110 / 992 bloków • 11%"));
 o->end(ESP_FAIL);CHECK(find_label(lv_screen_active(),"Formatowanie nie powiodło się. Uruchom ponownie i zachowaj log USB."));
 screenshot("294-format-failed-pl");dsr_ui_provision_release();
 /* Static fallback when the UI task cannot start: gate closed, no quiesce. */
 /* UI task cannot start: gate closed again, no hold/quiesce, LCD stays dark. */
 unsigned quiesces=provision_quiesces,holds=provision_holds,lights=provision_backlight;fail_task_start=true;
 o->begin(false,992,0);CHECK(!provision_gate&&provision_holds==holds&&provision_backlight==lights);o->end(ESP_OK);dsr_ui_provision_release();
 CHECK(provision_quiesces==quiesces+1&&provision_holds==holds);fail_task_start=false;
 /* Locales follow dsr_sc_tr (first boot normally has the PL default). */
 dsr_sc_appearance(DSR_SC_BLUE,false,1);provision_progress_value=248;o->begin(false,992,0);advance(500);
 CHECK(find_label(lv_screen_active(),"Preparing storage")&&find_label(lv_screen_active(),"248 / 992 blocks • 25%"));CHECK(dsr_ui_provision_layout_ok());screenshot("295-format-en");o->end(ESP_OK);dsr_ui_provision_release();
 dsr_sc_appearance(DSR_SC_BLUE,false,2);o->begin(true,992,313);advance(500);
 CHECK(find_label(lv_screen_active(),"Speicher wird vorbereitet")&&find_label(lv_screen_active(),"Neustart von Anfang an…"));CHECK(dsr_ui_provision_layout_ok());screenshot("296-format-resume-de");o->end(ESP_OK);dsr_ui_provision_release();
 dsr_sc_appearance(DSR_SC_BLUE,false,0);
 /* Repeated show/release leaves the LVGL pool unchanged (after warm-up). */
 for(unsigned i=0;i<2;i++){o->begin(i&1,992,i&1?109:0);advance(500);o->end(ESP_OK);dsr_ui_provision_release();}
 advance(500);lv_mem_monitor_t m1,m2;lv_mem_monitor(&m1);
 for(unsigned i=0;i<20;i++){o->begin(i&1,992,i&1?109:0);advance(500);o->end(ESP_OK);dsr_ui_provision_release();}
 advance(500);lv_mem_monitor(&m2);CHECK(m1.free_size==m2.free_size);
 printf("PASS: %u first-format LCD checks; %u frames; real Flash-progress seam, monotonic/bounded, resume prefix, failure, static fallback, PL EN DE, heap %lu→%lu; storage/board MODEL\n",checks,screenshots,(unsigned long)m1.free_size,(unsigned long)m2.free_size);
 lv_deinit();return 0;
}
