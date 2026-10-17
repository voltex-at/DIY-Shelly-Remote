/* First-boot SPIFFS format progress screen (#28).
 * Shown only while dsr_storage_mount() runs its single format pass. Before
 * this, the LCD stayed dark for the whole ~1 minute, which invites a power
 * cut exactly when one leaves an interrupted format behind.
 * Progress is READ FROM FLASH (formatter block headers), not estimated. */
#include "dsr_ui_provision.h"
#include "dsr_ui_style.h"
#include "dsr_ui_brand.h"
#include "dsr_board.h"
#include "dsr_boot.h"
#include "dsr_storage_progress.h"
#include <stdio.h>

static lv_obj_t *root,*spinner,*bar,*count_label,*time_label,*status_label;
static lv_timer_t *timer;
static uint32_t (*progress_source)(void);
static uint32_t total,remnant,shown,started;
static bool resume;

static void render(void);
static void update(void)
{
    if(!root)return;
    const uint32_t read=progress_source?progress_source():0;
    /* A resumed format still shows the old prefix until the formatter
     * passes it, so only values above the remnant count as new progress. */
    const uint32_t value=resume&&read<=remnant?0:read;
    if(value>shown&&value<=total)shown=value;
    render();
}
static void render(void)
{
    lv_bar_set_value(bar,(int32_t)shown,LV_ANIM_OFF);
    if(resume&&!shown)dsr_sc_label_set_text(count_label,"Wznawianie od początku…");
    else dsr_sc_label_set_text_fmt(count_label,"%u / %u bloków • %u%%",(unsigned)shown,(unsigned)total,(unsigned)(total?shown*100u/total:0u));
    const uint32_t seconds=lv_tick_elaps(started)/1000u;
    dsr_sc_label_set_text_fmt(time_label,"Czas: %u:%02u",(unsigned)(seconds/60u),(unsigned)(seconds%60u));
    /* Keep the idle-sleep timer from darkening the screen mid-format. */
    lv_display_trigger_activity(NULL);
}
static void on_timer(lv_timer_t *t){(void)t;update();}

void dsr_ui_provision_show(bool resumed,uint32_t blocks,uint32_t remnant_blocks,uint32_t (*progress)(void))
{
    if(root)return;
    resume=resumed;total=blocks;remnant=remnant_blocks;progress_source=progress;shown=0;started=lv_tick_get();
    lv_obj_t *screen=lv_screen_active();
    root=dsr_sc_panel(screen,0,0,320,480,DSR_SC_BG,0);
    lv_obj_set_style_bg_image_src(root,dsr_ui_brand_background(),0);
    dsr_sc_text(root,"Przygotowanie pamięci",16,28,288,&dsr_roboto_22,DSR_SC_TEXT);
    spinner=lv_spinner_create(root);
    lv_spinner_set_anim_params(spinner,1200,240);
    lv_obj_set_size(spinner,84,84);lv_obj_set_pos(spinner,118,86);
    lv_obj_set_style_arc_width(spinner,8,LV_PART_MAIN);lv_obj_set_style_arc_width(spinner,8,LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner,dsr_sc_color(DSR_SC_CARD),LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);
    status_label=dsr_sc_text(root,resumed?"Poprzednie formatowanie zostało przerwane. Dokańczanie formatowania pamięci konfiguracji.":"Pierwsze uruchomienie: formatowanie pamięci konfiguracji.",16,192,288,&dsr_roboto_16,DSR_SC_TEXT);
    bar=lv_bar_create(root);
    lv_obj_set_size(bar,288,12);lv_obj_set_pos(bar,16,288);
    lv_bar_set_range(bar,0,(int32_t)(blocks?blocks:1u));
    /* No theme exists yet (dsr_ui_init runs after storage): style fully. */
    lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_CARD),LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_MAIN);
    lv_obj_set_style_radius(bar,6,LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar,6,LV_PART_INDICATOR);
    count_label=dsr_sc_text(root,"",16,308,288,&dsr_roboto_14,DSR_SC_MUTED);
    time_label=dsr_sc_text(root,"",16,330,288,&dsr_roboto_14,DSR_SC_MUTED);
    lv_obj_t *warning=dsr_sc_panel(root,16,364,288,84,DSR_SC_PANEL,DSR_SC_RADIUS);
    dsr_sc_text(warning,resumed?"Nie odłączaj zasilania. Partycja nie zawiera danych użytkownika. Zwykle 1–2 minuty.":"Nie odłączaj zasilania. Zwykle trwa 1–2 minuty.",12,12,264,&dsr_roboto_14,DSR_SC_TEXT);
    timer=lv_timer_create(on_timer,500,NULL);
    update();
}

void dsr_ui_provision_finish(esp_err_t result)
{
    if(!root)return;
    if(timer){lv_timer_delete(timer);timer=NULL;}
    update();progress_source=NULL;
    lv_obj_add_flag(spinner,LV_OBJ_FLAG_HIDDEN);
    if(result==ESP_OK){shown=total;render();dsr_sc_label_set_text(status_label,"Pamięć gotowa. Uruchamianie…");}
    else dsr_sc_label_set_text(status_label,"Formatowanie nie powiodło się. Uruchom ponownie i zachowaj log USB.");
}

void dsr_ui_provision_release(void)
{
    if(timer){lv_timer_delete(timer);timer=NULL;}
    if(root){lv_obj_delete(root);root=NULL;}
    spinner=bar=count_label=time_label=status_label=NULL;progress_source=NULL;
}

bool dsr_ui_provision_visible(void){return root!=NULL;}
/* Layout guard for tests: the wrapped status text must end above the bar. */
bool dsr_ui_provision_layout_ok(void)
{
    if(!root)return false;
    lv_obj_update_layout(root);
    lv_area_t text,track;lv_obj_get_coords(status_label,&text);lv_obj_get_coords(bar,&track);
    return text.y2<track.y1;
}

/* ---- Storage observer glue: runs on app_main before/after the format. ---- */
static bool task_running;
/* Rendering happens ONLY in the 8 KiB UI task, never on app_main's stack:
 * main builds/updates objects while the gate is closed and the task idle. */
static void observer_begin(bool resumed,uint32_t blocks,uint32_t remnant_blocks)
{
    /* The boot splash may already be rendering: take LVGL back first. */
    dsr_boot_provisioning_ui(false);dsr_board_ui_quiesce();
    dsr_ui_provision_show(resumed,blocks,remnant_blocks,dsr_storage_format_progress);
    dsr_boot_provisioning_ui(true);
    task_running=dsr_board_start_ui_task(dsr_boot_ui_gate)==ESP_OK;
    if(!task_running){dsr_boot_provisioning_ui(false);return;} /* stay dark, as before */
    dsr_board_ui_hold(150); /* first frame before backlight: no panel garbage */
    (void)dsr_board_restore_backlight();
}
static void observer_end(esp_err_t result)
{
    if(!task_running){dsr_ui_provision_finish(result);return;}
    dsr_boot_provisioning_ui(false);dsr_board_ui_quiesce();
    dsr_ui_provision_finish(result);
    /* Let the UI task show the outcome, then hand LVGL back to app_main. */
    dsr_boot_provisioning_ui(true);dsr_board_ui_hold(result==ESP_OK?800:4000);
    dsr_boot_provisioning_ui(false);dsr_board_ui_quiesce();
}
static const dsr_storage_observer_t observer={.begin=observer_begin,.end=observer_end};
const dsr_storage_observer_t *dsr_ui_storage_observer(void){return &observer;}
