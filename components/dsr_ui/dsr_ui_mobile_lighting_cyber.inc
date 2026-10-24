/* Shared dimmer and RGB pages after the owner's HUD references (2026-10).
 * Every control uses confirmed model data and the existing Light/RGB RPC
 * actions; a finger drag is only a preview until the release sends it. */
#define LIGHT_CX 146
#define LIGHT_CY 84
#define LIGHT_R 70
#define WHEEL_CX 147
#define WHEEL_CY 108
#define WHEEL_R 64
#define WHEEL_W 18
static const struct {const char *name;uint8_t r,g,b;uint32_t dot;} rgb_presets[4]={
 {"Ciepły",255,147,41,0xffb547},{"Neutralny",255,214,170,0xf4f1ff},{"Niebieski",0,120,255,0x3a8dff},{"Relaks",150,60,255,0xa64dff}};
static bool lighting_page(void)
{
 if((page!=DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_GROUP_CONTROL)||!lighting_selected())return false;
 uint8_t kind=lighting_device()->endpoint_kind;return kind==DSR_ENDPOINT_LIGHT||kind==DSR_ENDPOINT_RGB;
}
static uint32_t lighting_accent(void)
{return dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?DSR_SC_GLOW:DSR_SC_BLUE;}
static uint32_t lighting_warning(void)
{return dsr_sc_style()==DSR_SC_STYLE_LIGHT?0x9a5a00:0xffc65b;}
/* Shared surfaces keep each skin's fill, border and corner treatment. */
static lv_obj_t *lighting_box(lv_obj_t *parent,int x,int y,int w,int h)
{
 lv_obj_t *o=dsr_sc_panel(parent,x,y,w,h,DSR_SC_PANEL,8);
 if(dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE)edge(o,DSR_SC_EDGE,1);
 return o;
}
static void lighting_light_tabs(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=lighting_device();bool rgb=d->endpoint_kind==DSR_ENDPOINT_RGB;
 const char *names[]={"Sterowanie",rgb?"Sceny":"Zużycie","Ustawienia"};const uintptr_t targets[]={15,rgb?19:16,14};
 const bool enabled[]={true,!cover_preview&&!model.demo&&(rgb?actions.open_actions!=NULL:page!=DSR_MOBILE_GROUP_CONTROL&&actions.open_usage!=NULL),!cover_preview&&!model.demo&&(page==DSR_MOBILE_GROUP_CONTROL?actions.edit_action!=NULL:d->id!=0)};
 for(unsigned i=0;i<3;i++){
  bool chosen=i==0,back=chosen&&rgb_color_view;
  lv_obj_t *b=dsr_sc_button(root,12+(int)i*100,46,96,38,chosen?DSR_SC_BLUE:DSR_SC_PANEL,8,(chosen&&!back)||!enabled[i]?NULL:navigate,(void *)targets[i]);
  if(chosen&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_panel(b,true);
  const lv_font_t *font=&dsr_roboto_14;lv_point_t size;lv_text_get_size(&size,dsr_sc_tr(names[i]),dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(size.x>86)font=&dsr_roboto_12;
  lv_obj_t *label=dsr_sc_text(b,names[i],2,0,88,font,chosen?DSR_SC_TEXT:DSR_SC_MUTED);center(label);lv_obj_align(label,LV_ALIGN_CENTER,0,0);
  if(!enabled[i])lv_obj_add_state(b,LV_STATE_DISABLED);else if(chosen&&!back)lv_obj_remove_flag(b,LV_OBJ_FLAG_CLICKABLE);
 }
}
static const char *lighting_light_problem(const dsr_mobile_device_t *d)
{
 const char *group=group_display_state(d);if(group)return group;
 bool live=d->online&&d->error==DSR_OK,blocked=!(d->endpoint_kind==DSR_ENDPOINT_RGB&&rgb_color_view?d->can_rgb:d->can_light);
 return unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_problem(d):!live?"Brak aktualnego odczytu":d->busy?"Wysyłanie polecenia…":blocked?(d->endpoint_kind==DSR_ENDPOINT_LIGHT&&(d->light.conditions||d->light.errors)?"Sterowanie światłem zablokowane":"Sterowanie zablokowane"):NULL;
}
/* Problems replace the confirmation age on the last line, in amber. */
static void lighting_light_footer(lv_obj_t *root,const dsr_mobile_device_t *d,int y)
{
 const char *problem=lighting_light_problem(d);
 dsr_sc_icon(root,problem?DSR_ICON_WARNING:DSR_ICON_CLOCK,14,y,16,problem?lighting_warning():DSR_SC_MUTED);
 lv_obj_t *line=dsr_sc_text(root,problem?problem:"",36,y+1,272,&dsr_roboto_12,problem?lighting_warning():DSR_SC_MUTED);truncate(line,16);
 if(!problem){if(page==DSR_MOBILE_GROUP_CONTROL)dsr_sc_label_set_text(line,"Każde urządzenie ma własny wynik.");else{sample_age=line;update_sample_age();}}
}
static void lighting_light_metrics(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 bool readable=d->online&&d->error==DSR_OK;char text[32];
 const bool present[]={d->has_power,d->has_voltage,d->has_current};const double values[]={d->power_w,d->voltage_v,d->current_a};
 const char *names[]={"Moc","Napięcie","Prąd"},*units[]={"W","V","A"};
 for(unsigned i=0;i<3;i++){
  lv_obj_t *box=lighting_box(root,12+(int)i*100,90,96,42);
  center(dsr_sc_text(box,names[i],6,3,84,&dsr_roboto_11,DSR_SC_MUTED));
  if(readable&&present[i]&&isfinite(values[i]))snprintf(text,sizeof(text),i==2?"%.2f %s":"%.1f %s",values[i],units[i]);else snprintf(text,sizeof(text),"— %s",units[i]);
  lv_obj_t *value=dsr_sc_text(box,dsr_sc_user(text),6,17,84,&dsr_roboto_14,DSR_SC_TEXT);center(value);
  detail_electrical[i]=value;
 }
}
static void lighting_light_step(lv_event_t *event)
{
 if(!lighting_selected()||!light_enabled(lighting_device()))return;
 int value=brightness(lighting_device())+((uintptr_t)lv_event_get_user_data(event)?5:-5);
 if(value<0)value=0;
 if(value>100)value=100;
 lighting_send_brightness((uint8_t)value);
}
static void lighting_light_preset(lv_event_t *event)
{if(lighting_selected()&&light_enabled(lighting_device()))lighting_send_brightness((uint8_t)(uintptr_t)lv_event_get_user_data(event));}
static bool light_known(const dsr_mobile_device_t *d)
{return d->online&&d->error==DSR_OK&&(d->endpoint_kind==DSR_ENDPOINT_RGB?(d->rgb.flags&DSR_RGB_VALID):(d->light.flags&DSR_LIGHT_BRIGHTNESS));}
/* Knob on the dial and the big value: amber while it is only a preview. */
static void lighting_light_knob(int value)
{if(light_knob){double t=(135.0+2.7*value)*3.14159265358979/180.0;lv_obj_set_pos(light_knob,LIGHT_CX+(int)lround((LIGHT_R-5)*cos(t))-11,LIGHT_CY+(int)lround((LIGHT_R-5)*sin(t))-11);}}
static void lighting_light_label(const dsr_mobile_device_t *d,int value,bool preview)
{if(light_value){char text[16];if(preview||light_known(d))snprintf(text,sizeof(text),"%d%%",value);else snprintf(text,sizeof(text),"—");lv_label_set_text(light_value,text);lv_obj_set_style_text_color(light_value,dsr_sc_text_color(preview?lighting_warning():DSR_SC_TEXT),0);}}
static void lighting_light_show(const dsr_mobile_device_t *d,int value,bool preview){lighting_light_knob(value);lighting_light_label(d,value,preview);}
static void lighting_light_arc_event(lv_event_t *event)
{
 if(!lighting_selected())return;
 dsr_mobile_device_t *d=lighting_device();lv_event_code_t code=lv_event_get_code(event);lv_obj_t *arc=lv_event_get_target(event);
 if(code==LV_EVENT_PRESS_LOST||!light_enabled(d)){light_drag=false;lv_arc_set_value(arc,brightness(d));lighting_light_show(d,brightness(d),false);return;}
 if(code==LV_EVENT_PRESSED){light_drag=true;light_drag_id=d->id;return;}
 if(!light_drag||light_drag_id!=d->id)return;
 int value=lv_arc_get_value(arc);
 if(code==LV_EVENT_VALUE_CHANGED){lighting_light_show(d,value,true);return;}
 if(code!=LV_EVENT_RELEASED)return;
 light_drag=false;lv_indev_t *input=lv_indev_active();lv_area_t area;lv_obj_get_coords(arc,&area);
 if(input){lv_point_t point;lv_indev_get_point(input,&point);if(point.x<area.x1-24||point.x>area.x2+24||point.y<area.y1-24||point.y>area.y2+24)value=brightness(d);}
 /* The number returns to the confirmed level; the knob marks the sent target. */
 int confirmed=brightness(d);lighting_light_label(d,confirmed,false);lighting_light_knob(value);
 if(value!=confirmed)lighting_send_brightness((uint8_t)value);else lv_arc_set_value(arc,value);
}
static void lighting_light_ticks(lv_event_t *event)
{
 lv_area_t a;lv_obj_get_content_coords(lv_event_get_target(event),&a);
 lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.color=dsr_sc_color(DSR_SC_MUTED);line.width=1;line.opa=LV_OPA_70;
 for(int i=0;i<=27;i++){
  double t=(135.0+10.0*i)*3.14159265358979/180.0;int inner=i%3?LIGHT_R-20:LIGHT_R-25,outer=LIGHT_R-15;
  line.p1=(lv_point_precise_t){a.x1+LIGHT_CX+(int)lround(inner*cos(t)),a.y1+LIGHT_CY+(int)lround(inner*sin(t))};
  line.p2=(lv_point_precise_t){a.x1+LIGHT_CX+(int)lround(outer*cos(t)),a.y1+LIGHT_CY+(int)lround(outer*sin(t))};
  lv_draw_line(lv_event_get_layer(event),&line);
 }
}
/* "Włączony" row: the whole row toggles the output; the switch only shows it. */
static void lighting_power_row(lv_obj_t *parent,const dsr_mobile_device_t *d,int x,int y,int w)
{
 bool valid=d->online&&!d->busy&&d->can_switch,on=d->online&&d->error==DSR_OK&&d->output;
 lv_obj_t *row=dsr_sc_button(parent,x,y,w,38,DSR_SC_PANEL,8,page==DSR_MOBILE_GROUP_CONTROL?group_power:control,(void *)(uintptr_t)selected);
 lv_obj_t *ring=dsr_sc_panel(row,4,1,32,32,on?DSR_SC_BLUE:DSR_SC_PANEL,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(ring,DSR_ICON_POWER,18,on?0xffffff:lighting_accent());
 dsr_sc_text(row,d->output?"Włączony":"Wyłączony",46,7,w-120,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *toggle=lv_switch_create(row);lv_obj_set_pos(toggle,w-66,5);lv_obj_set_size(toggle,50,24);lv_obj_remove_flag(toggle,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_set_style_bg_color(toggle,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_bg_opa(toggle,LV_OPA_COVER,LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_bg_color(toggle,lv_color_hex(0xffffff),LV_PART_KNOB);
 /* This is a readback indicator; show the known state immediately on entry. */
 static const lv_style_prop_t props[]={LV_STYLE_BG_COLOR,LV_STYLE_BG_OPA,0};
 static lv_style_transition_dsc_t readback;static bool ready;
 if(!ready){lv_style_transition_dsc_init(&readback,props,lv_anim_path_linear,0,0,NULL);ready=true;}
 lv_obj_set_style_anim_duration(toggle,0,LV_PART_MAIN);lv_obj_set_style_transition(toggle,&readback,LV_PART_MAIN|LV_STATE_CHECKED);lv_obj_set_style_transition(toggle,&readback,LV_PART_INDICATOR|LV_STATE_CHECKED);
 if(on)lv_obj_add_state(toggle,LV_STATE_CHECKED);
 if(!valid){lv_obj_add_state(row,LV_STATE_DISABLED);lv_obj_add_state(toggle,LV_STATE_DISABLED);}
}
static void lighting_rgb_main(lv_obj_t *root,const dsr_mobile_device_t *d);
static void lighting_light_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=lighting_device();
 if(d->endpoint_kind==DSR_ENDPOINT_RGB){lighting_rgb_main(root,d);return;}
 bool known=light_known(d),enabled=light_enabled(d);
 lighting_light_metrics(root,d);
 lv_obj_t *panel=dsr_sc_panel(root,12,138,296,190,DSR_SC_CARD,12);lv_obj_add_event_cb(panel,lighting_light_ticks,LV_EVENT_DRAW_MAIN_END,NULL);
 const uint32_t colors[]={0xff4a1c,0xffc34a,0x7fd0ff,0x1d72ff};
 dsr_sc_gradient_arc(panel,LIGHT_CX,LIGHT_CY,LIGHT_R,10,135,270,colors,4,18,known);
 dsr_sc_icon(panel,DSR_ICON_LIGHT,LIGHT_CX-18,26,36,known&&d->output?lighting_warning():DSR_SC_OFF);
 light_value=dsr_sc_text(panel,"",LIGHT_CX-60,62,120,&dsr_roboto_28,DSR_SC_TEXT);center(light_value);
 center(dsr_sc_text(panel,"Jasność",LIGHT_CX-60,98,120,&dsr_roboto_14,lighting_accent()));
 /* Native arc: annular hit testing and continuous drag over the drawn scale. */
 lv_obj_t *arc=lv_arc_create(panel);lv_obj_remove_style_all(arc);lv_obj_set_pos(arc,LIGHT_CX-LIGHT_R-8,LIGHT_CY-LIGHT_R-8);lv_obj_set_size(arc,2*LIGHT_R+16,2*LIGHT_R+16);
 lv_arc_set_range(arc,0,100);lv_arc_set_bg_angles(arc,135,45);lv_arc_set_value(arc,known?brightness(d):0);lv_arc_set_change_rate(arc,720);
 lv_obj_set_style_arc_width(arc,30,LV_PART_MAIN);lv_obj_set_style_arc_width(arc,30,LV_PART_INDICATOR);lv_obj_set_style_arc_opa(arc,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_arc_opa(arc,LV_OPA_TRANSP,LV_PART_INDICATOR);
 lv_obj_add_flag(arc,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_remove_flag(arc,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 const lv_event_code_t codes[]={LV_EVENT_PRESSED,LV_EVENT_VALUE_CHANGED,LV_EVENT_RELEASED,LV_EVENT_PRESS_LOST};
 for(unsigned i=0;i<4;i++)lv_obj_add_event_cb(arc,lighting_light_arc_event,codes[i],NULL);
 if(!enabled)lv_obj_add_state(arc,LV_STATE_DISABLED);
 light_knob=dsr_sc_panel(panel,0,0,22,22,0xffffff,LV_RADIUS_CIRCLE);lv_obj_set_style_bg_color(light_knob,lv_color_hex(0xe9f7ff),0);
 if(dsr_sc_style()==DSR_SC_STYLE_LIGHT)edge(light_knob,DSR_SC_BLUE,2);
 if(!known)lv_obj_add_flag(light_knob,LV_OBJ_FLAG_HIDDEN);
 lighting_light_show(d,known?brightness(d):0,false);
 for(unsigned i=0;i<2;i++){
  lv_obj_t *b=dsr_sc_button(panel,i?240:8,96,44,44,DSR_SC_PANEL,LV_RADIUS_CIRCLE,lighting_light_step,(void *)(uintptr_t)i);
  center(dsr_sc_text(b,i?"+":"-",0,6,40,&dsr_roboto_22,DSR_SC_TEXT));if(!enabled)lv_obj_add_state(b,LV_STATE_DISABLED);
 }
 lighting_power_row(panel,d,8,144,276);
 lv_obj_t *quick=lighting_box(root,12,334,296,70);dsr_sc_text(quick,"Szybkie ustawienia",10,3,200,&dsr_roboto_12,DSR_SC_MUTED);
 const uint8_t levels[]={10,25,50,100};const uint32_t bulbs[]={0x8aa4bf,0xd9b45a,0xffd166,0xffc107};
 for(unsigned k=0;k<4;k++){
  bool chosen=known&&brightness(d)==levels[k];char text[8];snprintf(text,sizeof(text),"%u%%",levels[k]);
  lv_obj_t *b=dsr_sc_button(quick,7+(int)k*71,22,66,42,chosen?DSR_SC_BLUE:DSR_SC_PANEL,8,lighting_light_preset,(void *)(uintptr_t)levels[k]);
  dsr_sc_icon(b,DSR_ICON_LIGHT,23,2,16,enabled?bulbs[k]:DSR_SC_OFF);center(dsr_sc_text(b,text,2,19,58,&dsr_roboto_14,DSR_SC_TEXT));
  if(!enabled)lv_obj_add_state(b,LV_STATE_DISABLED);
 }
 lighting_light_footer(root,d,409);
}
/* Hue ring: angle clockwise from 3 o'clock, red 0°, matching the drawn spectrum. */
static void lighting_hue_rgb(int hue,uint8_t out[3])
{
 hue=((hue%360)+360)%360;int fold=hue%120-60;uint8_t x=(uint8_t)(255*(60-(fold<0?-fold:fold))/60);
 const uint8_t table[6][3]={{255,x,0},{x,255,0},{0,255,x},{0,x,255},{x,0,255},{255,0,x}};memcpy(out,table[hue/60],3);
}
static int lighting_rgb_hue(const uint8_t c[3])
{
 int r=c[0],g=c[1],b=c[2],max=r>g?(r>b?r:b):(g>b?g:b),min=r<g?(r<b?r:b):(g<b?g:b),span=max-min;if(!span)return -1;
 double h=max==r?fmod((double)(g-b)/span+6.0,6.0):max==g?(double)(b-r)/span+2.0:(double)(r-g)/span+4.0;return (int)lround(h*60.0)%360;
}
static void lighting_wheel_show(void)
{
 if(rgb_text){char text[24];bool readable=lighting_selected()&&light_known(lighting_device());if(readable||rgb_dirty)snprintf(text,sizeof(text),"%u, %u, %u",rgb_draft[0],rgb_draft[1],rgb_draft[2]);else snprintf(text,sizeof(text),"—");lv_label_set_text(rgb_text,text);}
 if(!wheel_knob)return;
 int hue=lighting_rgb_hue(rgb_draft);
 if(hue<0){lv_obj_add_flag(wheel_knob,LV_OBJ_FLAG_HIDDEN);return;}
 double t=hue*3.14159265358979/180.0;int r=WHEEL_R-WHEEL_W/2;lv_obj_remove_flag(wheel_knob,LV_OBJ_FLAG_HIDDEN);
 lv_obj_set_pos(wheel_knob,WHEEL_CX+(int)lround(r*cos(t))-11,WHEEL_CY+(int)lround(r*sin(t))-11);
 lv_obj_set_style_bg_color(wheel_knob,lv_color_hex(((uint32_t)rgb_draft[0]<<16)|((uint32_t)rgb_draft[1]<<8)|rgb_draft[2]),0);
}
/* Only a press that starts on the ring picks a hue; the centre stays inert. */
static bool wheel_drag;
static void lighting_wheel_event(lv_event_t *event)
{
 lv_event_code_t code=lv_event_get_code(event);lv_obj_t *wheel=lv_event_get_target(event);
 if(!lighting_selected())return;
 const dsr_mobile_device_t *d=lighting_device();
 if(code==LV_EVENT_PRESS_LOST||!rgb_enabled(d)){wheel_drag=rgb_dirty=false;const uint8_t actual[]={d->rgb.r,d->rgb.g,d->rgb.b};memcpy(rgb_draft,actual,3);rgb_preview();lighting_wheel_show();return;}
 lv_indev_t *input=lv_indev_active();if(!input)return;
 lv_point_t point;lv_indev_get_point(input,&point);lv_area_t a;lv_obj_get_coords(wheel,&a);
 int dx=point.x-(a.x1+a.x2)/2,dy=point.y-(a.y1+a.y2)/2,r2=dx*dx+dy*dy;
 if(code==LV_EVENT_PRESSED)wheel_drag=r2>=(WHEEL_R-WHEEL_W-10)*(WHEEL_R-WHEEL_W-10)&&r2<=(WHEEL_R+6)*(WHEEL_R+6);
 if(!wheel_drag)return;
 if(code==LV_EVENT_PRESSED||code==LV_EVENT_PRESSING){
  double h=atan2((double)dy,(double)dx)*180.0/3.14159265358979;if(h<0)h+=360.0;
  lighting_hue_rgb((int)lround(h),rgb_draft);rgb_draft_id=d->id;rgb_dirty=true;rgb_preview();lighting_wheel_show();return;
 }
 if(code==LV_EVENT_RELEASED){wheel_drag=false;if(rgb_dirty&&rgb_draft_id==d->id){rgb_dirty=false;lighting_send_rgb(rgb_draft[0],rgb_draft[1],rgb_draft[2]);}}
}
static void lighting_rgb_preset(lv_event_t *event)
{
 unsigned k=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(k<4&&lighting_selected()&&rgb_enabled(lighting_device()))lighting_send_rgb(rgb_presets[k].r,rgb_presets[k].g,rgb_presets[k].b);
}
static void lighting_light_percent(lv_event_t *event)
{if(light_value){char text[16];snprintf(text,sizeof(text),"%d%%",(int)lv_slider_get_value(lv_event_get_target(event)));lv_label_set_text(light_value,text);}}
static void lighting_rgb_main(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 bool known=light_known(d),enabled=light_enabled(d),live=d->online&&d->error==DSR_OK;char text[32];
 rgb_draft[0]=known?d->rgb.r:0;rgb_draft[1]=known?d->rgb.g:0;rgb_draft[2]=known?d->rgb.b:0;rgb_draft_id=d->id;
 lv_obj_t *panel=lighting_box(root,12,90,296,180);
 lv_obj_t *title=dsr_sc_text(panel,"Sterowanie RGB",10,6,178,&dsr_roboto_18,DSR_SC_TEXT);truncate(title,22);
 const char *problem=lighting_light_problem(d);
 lv_obj_t *status=dsr_sc_text(panel,problem?problem:d->output?"Włączone • Online":"Wyłączone • Online",10,30,178,&dsr_roboto_12,problem?lighting_warning():lighting_accent());truncate(status,16);
 lv_obj_t *color=dsr_sc_button(panel,196,6,90,34,DSR_SC_PANEL,8,rgb_view,NULL);dsr_sc_icon(color,DSR_ICON_PALETTE,6,3,24,lighting_accent());
 dsr_sc_text(color,"Kolor",34,7,40,&dsr_roboto_14,DSR_SC_TEXT);dsr_sc_text(color,">",74,6,10,&dsr_roboto_16,lighting_accent());
 const uint32_t spectrum[]={0xff354f,0xffcc3b,0x00ee63,0x00dfff,0x2174ff,0xe333ff,0xff354f};
 dsr_sc_gradient_arc(panel,WHEEL_CX,WHEEL_CY,WHEEL_R,WHEEL_W,0,360,spectrum,7,24,known);
 rgb_swatch=dsr_sc_panel(panel,WHEEL_CX-20,WHEEL_CY-30,40,40,0,LV_RADIUS_CIRCLE);lv_obj_set_style_bg_grad_dir(rgb_swatch,LV_GRAD_DIR_NONE,0);
 center(dsr_sc_text(panel,d->output?"Włączone":"Wyłączone",WHEEL_CX-42,WHEEL_CY+14,84,&dsr_roboto_12,lighting_accent()));
 wheel_knob=dsr_sc_panel(panel,0,0,22,22,0xffffff,LV_RADIUS_CIRCLE);lv_obj_set_style_border_color(wheel_knob,lv_color_hex(0xffffff),0);lv_obj_set_style_border_width(wheel_knob,3,0);
 lv_obj_t *wheel=dsr_sc_panel(panel,WHEEL_CX-WHEEL_R-4,WHEEL_CY-WHEEL_R-4,2*WHEEL_R+8,2*WHEEL_R+8,DSR_SC_BG,0);lv_obj_set_style_bg_opa(wheel,LV_OPA_TRANSP,0);
 lv_obj_add_flag(wheel,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(wheel,LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 const lv_event_code_t codes[]={LV_EVENT_PRESSED,LV_EVENT_PRESSING,LV_EVENT_RELEASED,LV_EVENT_PRESS_LOST};
 for(unsigned i=0;i<4;i++)lv_obj_add_event_cb(wheel,lighting_wheel_event,codes[i],NULL);
 if(!rgb_enabled(d))lv_obj_add_state(wheel,LV_STATE_DISABLED);
 dsr_sc_text(panel,"RGB",10,132,70,&dsr_roboto_12,DSR_SC_MUTED);rgb_text=dsr_sc_text(panel,"",10,149,76,&dsr_roboto_12,DSR_SC_TEXT);
 lv_obj_t *label=dsr_sc_text(panel,"Moc",214,132,72,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_RIGHT,0);
 if(live&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);else snprintf(text,sizeof(text),"— W");
 label=dsr_sc_text(panel,dsr_sc_user(text),204,147,82,&dsr_roboto_18,DSR_SC_TEXT);detail_electrical[0]=label;lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_RIGHT,0);truncate(label,22);
 rgb_preview();lighting_wheel_show();
 lv_obj_t *level=lighting_box(root,12,274,296,42);dsr_sc_icon(level,DSR_ICON_LIGHT,10,5,16,lighting_accent());
 dsr_sc_text(level,"Jasność",32,6,160,&dsr_roboto_12,DSR_SC_TEXT);
 light_value=dsr_sc_text(level,"",200,4,84,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(light_value,LV_TEXT_ALIGN_RIGHT,0);
 if(known)snprintf(text,sizeof(text),"%u%%",brightness(d));else snprintf(text,sizeof(text),"—");lv_label_set_text(light_value,text);
 light_slider=lv_slider_create(level);lv_obj_set_pos(light_slider,16,26);lv_obj_set_size(light_slider,262,10);
 lv_slider_set_range(light_slider,0,100);lv_slider_set_value(light_slider,known?brightness(d):0,LV_ANIM_OFF);lv_obj_set_ext_click_area(light_slider,14);
 lv_obj_remove_flag(light_slider,LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 const lv_event_code_t slide[]={LV_EVENT_PRESSED,LV_EVENT_VALUE_CHANGED,LV_EVENT_RELEASED,LV_EVENT_PRESS_LOST};
 for(unsigned i=0;i<4;i++)lv_obj_add_event_cb(light_slider,light_drag_event,slide[i],NULL);
 lv_obj_add_event_cb(light_slider,lighting_light_percent,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_add_event_cb(light_slider,lighting_light_percent,LV_EVENT_PRESS_LOST,NULL);
 if(!enabled)lv_obj_add_state(light_slider,LV_STATE_DISABLED);
 for(unsigned k=0;k<4;k++){
  lv_obj_t *b=dsr_sc_button(root,12+(int)k*75,320,71,44,DSR_SC_PANEL,8,lighting_rgb_preset,(void *)(uintptr_t)k);
  lv_obj_t *dot=dsr_sc_panel(b,26,5,14,14,rgb_presets[k].dot,LV_RADIUS_CIRCLE);lv_obj_set_style_border_width(dot,0,0);
  center(dsr_sc_text(b,rgb_presets[k].name,2,23,63,&dsr_roboto_12,DSR_SC_TEXT));
  if(!rgb_enabled(d))lv_obj_add_state(b,LV_STATE_DISABLED);
 }
 bool valid=d->online&&!d->busy&&d->can_switch;
 lv_obj_t *power=dsr_sc_button(root,12,368,296,44,DSR_SC_BLUE,10,page==DSR_MOBILE_GROUP_CONTROL?group_power:control,(void *)(uintptr_t)selected);
 dsr_sc_icon(power,DSR_ICON_POWER,94,10,24,dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?lighting_accent():DSR_SC_TEXT);lv_obj_t *power_text=dsr_sc_text(power,d->output?"Wyłącz":"Włącz",128,0,150,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_align(power_text,LV_ALIGN_LEFT_MID,128,0);
 if(!valid)lv_obj_add_state(power,LV_STATE_DISABLED);
}
/* "Kolor >": exact R/G/B draft, sent only by Zastosuj. */
static void lighting_rgb_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=lighting_device();bool readable=light_known(d);
 lv_obj_t *panel=lighting_box(root,12,90,296,264);
 dsr_sc_text(panel,"Kolor RGB",10,6,274,&dsr_roboto_18,DSR_SC_TEXT);
 const char *problem=lighting_light_problem(d);
 lv_obj_t *status=dsr_sc_text(panel,problem?problem:readable?((d->rgb.flags&DSR_RGB_TRANSITION)?"Przejście • poczekaj na odczyt":"Podgląd • Zastosuj wysyła kolor"):"Brak aktualnego odczytu",10,30,274,&dsr_roboto_12,problem?lighting_warning():DSR_SC_MUTED);truncate(status,16);
 rgb_swatch=dsr_sc_panel(panel,10,54,274,40,0,5);lv_obj_set_style_bg_grad_dir(rgb_swatch,LV_GRAD_DIR_NONE,0);
 const uint32_t colors[]={0xff3c64,0x21d884,0x08bfff};
 for(unsigned i=0;i<3;i++){
  rgb_values[i]=dsr_sc_text(panel,"",10,111+(int)i*52,76,&dsr_roboto_14,DSR_SC_TEXT);
  lv_obj_t *slider=lv_slider_create(panel);lv_obj_set_pos(slider,96,114+(int)i*52);lv_obj_set_size(slider,180,12);lv_slider_set_range(slider,0,255);lv_slider_set_value(slider,rgb_draft[i],LV_ANIM_OFF);lv_obj_set_ext_click_area(slider,14);
  lv_obj_set_style_bg_color(slider,lv_color_hex(colors[i]),LV_PART_INDICATOR);lv_obj_set_style_bg_color(slider,lv_color_hex(colors[i]),LV_PART_KNOB);
  lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)i);lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_PRESS_LOST,(void *)(uintptr_t)i);
  if(!rgb_enabled(d))lv_obj_add_state(slider,LV_STATE_DISABLED);
 }
 lv_obj_t *back=dsr_sc_button(root,12,364,144,48,DSR_SC_PANEL,10,rgb_view,NULL);center(dsr_sc_text(back,"Wstecz",4,13,136,&dsr_roboto_18,DSR_SC_TEXT));
 rgb_apply_button=dsr_sc_button(root,164,364,144,48,DSR_SC_BLUE,10,rgb_apply,NULL);center(dsr_sc_text(rgb_apply_button,"Zastosuj",4,13,136,&dsr_roboto_18,DSR_SC_TEXT));
 rgb_preview();
}
