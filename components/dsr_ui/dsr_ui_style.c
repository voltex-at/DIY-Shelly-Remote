#include "dsr_ui_style.h"
#include <stdint.h>

lv_obj_t *dsr_sc_panel(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_pad_all(o,0,0);
    lv_obj_set_style_text_color(o,lv_color_hex(DSR_SC_TEXT),0);
    return o;
}
lv_obj_t *dsr_sc_text(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font,uint32_t color)
{
    lv_obj_t *o=lv_label_create(p);lv_obj_remove_style_all(o);lv_label_set_text(o,s);
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_obj_set_style_text_font(o,font,0);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_WRAP);
    return o;
}
lv_obj_t *dsr_sc_button(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *o=dsr_sc_panel(p,x,y,w,h,color,radius);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_bg_color(o,lv_color_hex(0x344f71),LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x30373d),LV_STATE_DISABLED);
    lv_obj_set_style_opa(o,LV_OPA_COVER,LV_STATE_DISABLED);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,ctx);
    return o;
}

static void segment(lv_layer_t *layer,lv_area_t a,int size,int x1,int y1,int x2,int y2,lv_color_t color,int width)
{
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=color;d.width=width;d.round_start=d.round_end=1;
    d.p1=(lv_point_precise_t){a.x1+x1*size/24,a.y1+y1*size/24};
    d.p2=(lv_point_precise_t){a.x1+x2*size/24,a.y1+y2*size/24};lv_draw_line(layer,&d);
}
static void circle(lv_layer_t *layer,lv_area_t a,int size,int x,int y,int radius,int start,int end,lv_color_t color,int width)
{
    lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);d.color=color;d.width=width;d.rounded=1;
    d.center=(lv_point_t){a.x1+x*size/24,a.y1+y*size/24};d.radius=radius*size/24;
    d.start_angle=start;d.end_angle=end;lv_draw_arc(layer,&d);
}
static void icon_draw(lv_event_t *e)
{
    lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);
    int s=lv_obj_get_width(o),w=s>=40?2:2;
    lv_color_t c=lv_obj_get_style_line_color(o,0);lv_layer_t *l=lv_event_get_layer(e);
    dsr_ui_icon_t kind=(dsr_ui_icon_t)(uintptr_t)lv_event_get_user_data(e);
#define LINE(x1,y1,x2,y2) segment(l,a,s,x1,y1,x2,y2,c,w)
#define ARC(x,y,r,start,end) circle(l,a,s,x,y,r,start,end,c,w)
    switch(kind) {
    case DSR_ICON_HOME: LINE(2,10,12,2);LINE(12,2,22,10);LINE(5,9,5,22);LINE(5,22,19,22);LINE(19,22,19,9);LINE(9,22,9,14);LINE(9,14,15,14);LINE(15,14,15,22);break;
    case DSR_ICON_DASHBOARD: ARC(12,12,10,0,359);LINE(12,12,17,7);ARC(12,12,1,0,359);LINE(5,7,6,8);LINE(12,3,12,5);LINE(20,12,18,12);LINE(4,14,6,14);break;
    case DSR_ICON_ENERGY: LINE(14,2,5,14);LINE(5,14,11,14);LINE(11,14,10,22);LINE(10,22,20,9);LINE(20,9,13,9);LINE(13,9,14,2);break;
    case DSR_ICON_POWER: ARC(12,13,8,315,225);LINE(12,2,12,12);break;
    case DSR_ICON_PLUS: LINE(12,4,12,20);LINE(4,12,20,12);break;
    case DSR_ICON_BACK: LINE(15,3,6,12);LINE(6,12,15,21);break;
    case DSR_ICON_CHEVRON: LINE(5,8,12,15);LINE(12,15,19,8);break;
    case DSR_ICON_SETTINGS: ARC(12,12,6,0,359);ARC(12,12,2,0,359);LINE(12,2,12,5);LINE(12,19,12,22);LINE(2,12,5,12);LINE(19,12,22,12);LINE(5,5,7,7);LINE(17,17,19,19);LINE(5,19,7,17);LINE(17,7,19,5);break;
    case DSR_ICON_EDIT: LINE(4,16,16,4);LINE(16,4,20,8);LINE(20,8,8,20);LINE(8,20,3,21);LINE(3,21,4,16);LINE(14,6,18,10);break;
    case DSR_ICON_INFO: ARC(12,12,10,0,359);LINE(12,10,12,18);ARC(12,6,1,0,359);break;
    case DSR_ICON_LIGHT: ARC(12,9,6,150,30);LINE(7,12,9,17);LINE(17,12,15,17);LINE(9,17,15,17);LINE(9,20,15,20);LINE(11,22,13,22);break;
    case DSR_ICON_PLUG: LINE(8,2,8,8);LINE(16,2,16,8);LINE(5,8,19,8);LINE(6,8,6,14);LINE(18,8,18,14);ARC(12,13,6,0,180);LINE(12,19,12,23);break;
    case DSR_ICON_SOFA: LINE(5,12,5,5);LINE(5,5,19,5);LINE(19,5,19,12);LINE(2,12,7,12);LINE(7,12,7,17);LINE(7,17,17,17);LINE(17,17,17,12);LINE(17,12,22,12);LINE(22,12,22,21);LINE(22,21,2,21);LINE(2,21,2,12);LINE(5,21,5,23);LINE(19,21,19,23);break;
    case DSR_ICON_BED: LINE(3,4,3,22);LINE(3,13,21,13);LINE(21,13,21,22);LINE(3,19,21,19);LINE(6,8,11,8);LINE(11,8,11,13);LINE(6,8,6,13);break;
    case DSR_ICON_KITCHEN: LINE(3,4,21,4);LINE(3,4,3,22);LINE(21,4,21,22);LINE(3,22,21,22);LINE(3,10,21,10);LINE(6,14,18,14);LINE(6,14,6,19);LINE(18,14,18,19);LINE(6,19,18,19);ARC(7,7,1,0,359);ARC(17,7,1,0,359);break;
    case DSR_ICON_BATH: LINE(2,12,22,12);LINE(3,12,5,20);LINE(5,20,19,20);LINE(19,20,21,12);LINE(6,20,6,23);LINE(18,20,18,23);LINE(5,12,5,4);ARC(8,4,3,180,359);break;
    case DSR_ICON_WIFI: ARC(12,20,17,232,308);ARC(12,20,11,230,310);ARC(12,20,5,225,315);ARC(12,21,1,0,359);break;
    case DSR_ICON_GRID: for(int y=3;y<=14;y+=11)for(int x=3;x<=14;x+=11){LINE(x,y,x+7,y);LINE(x+7,y,x+7,y+7);LINE(x+7,y+7,x,y+7);LINE(x,y+7,x,y);}break;
    }
#undef LINE
#undef ARC
}
lv_obj_t *dsr_sc_icon(lv_obj_t *p,dsr_ui_icon_t kind,int x,int y,int size,uint32_t color)
{
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,size,size);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_color(o,lv_color_hex(color),0);
    lv_obj_add_event_cb(o,icon_draw,LV_EVENT_DRAW_MAIN,(void *)(uintptr_t)kind);return o;
}
void dsr_sc_icon_color(lv_obj_t *o,uint32_t color){lv_obj_set_style_line_color(o,lv_color_hex(color),0);lv_obj_invalidate(o);}
