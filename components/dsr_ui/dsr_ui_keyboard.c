#include "dsr_ui_internal.h"
#include <string.h>

/* One visible keyboard at a time. Maps have static lifetime as required by
 * LVGL; they contain labels only, never text/credentials from a field. */
#define DIGITS "1","2","3","4","5","6","7","8","9","0","\n"
#define FOOT(mode) mode,LV_SYMBOL_LEFT,"Spacja",LV_SYMBOL_RIGHT,"Gotowe",""
static const char *const lower[]={DIGITS,"q","w","e","r","t","y","u","i","o","p","\n","a","s","d","f","g","h","j","k","l","\n",LV_SYMBOL_UP,"z","x","c","v","b","n","m",LV_SYMBOL_BACKSPACE,"\n",FOOT("123")};
static const char *const upper[]={DIGITS,"Q","W","E","R","T","Y","U","I","O","P","\n","A","S","D","F","G","H","J","K","L","\n",LV_SYMBOL_UP,"Z","X","C","V","B","N","M",LV_SYMBOL_BACKSPACE,"\n",FOOT("123")};
static const char *const symbols[]={DIGITS,"!","@","#","$","%","^","&","*","(",")","\n","-","_","=","+","[","]","{","}","\\","|","\n","ABC","ąä",":",";","\"","'","<",">",LV_SYMBOL_BACKSPACE,"\n",FOOT("#+=")};
static const char *const symbols2[]={DIGITS,".",",","?","/","`","~","!","@","#","$","\n","%","^","&","*","(",")","-","_","=","+","\n","ABC","ąä",":",";","\"","'","<",">",LV_SYMBOL_BACKSPACE,"\n",FOOT("123")};
static const char *const accents[]={"ą","ć","ę","ł","ń","ó","ś","ź","ż","\n","Ą","Ć","Ę","Ł","Ń","Ó","Ś","Ź","Ż","\n","ä","ö","ü","ß","Ä","Ö","Ü","\n","ABC",LV_SYMBOL_BACKSPACE,"\n",FOOT("123")};
static const char *const numeric[]={"1","2","3","\n","4","5","6","\n","7","8","9","\n",".","0",LV_SYMBOL_BACKSPACE,"\n","ABC",LV_SYMBOL_LEFT,"Spacja",LV_SYMBOL_RIGHT,"Gotowe",""};
static const char *maps[6][64];
static lv_buttonmatrix_ctrl_t controls[6][64];
static const lv_keyboard_mode_t modes[]={LV_KEYBOARD_MODE_TEXT_LOWER,LV_KEYBOARD_MODE_TEXT_UPPER,LV_KEYBOARD_MODE_SPECIAL,LV_KEYBOARD_MODE_USER_1,LV_KEYBOARD_MODE_USER_2,LV_KEYBOARD_MODE_NUMBER};

static void keyboard_event(lv_event_t *event)
{
 lv_obj_t *kb=lv_event_get_target(event);uint32_t id=lv_keyboard_get_selected_button(kb);if(id==LV_BUTTONMATRIX_BUTTON_NONE)return;
 const char *text=lv_keyboard_get_button_text(kb,id);if(!text)return;
 if(!strcmp(text,"123")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_SPECIAL);return;}
 if(!strcmp(text,"#+=")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_USER_1);return;}
 if(!strcmp(text,"ąä")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_USER_2);return;}
 if(!strcmp(text,"ABC")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_TEXT_LOWER);return;}
 if(!strcmp(text,LV_SYMBOL_UP)){lv_keyboard_set_mode(kb,lv_keyboard_get_mode(kb)==LV_KEYBOARD_MODE_TEXT_UPPER?LV_KEYBOARD_MODE_TEXT_LOWER:LV_KEYBOARD_MODE_TEXT_UPPER);return;}
 /* READY can delete kb through join()/navigation. Never access it afterwards. */
 if(!strcmp(text,dsr_sc_tr("Gotowe"))){lv_obj_send_event(kb,LV_EVENT_READY,NULL);return;}
 lv_obj_t *field=lv_keyboard_get_textarea(kb);if(!field)return;
 if(!strcmp(text,LV_SYMBOL_LEFT))lv_textarea_cursor_left(field);
 else if(!strcmp(text,LV_SYMBOL_RIGHT))lv_textarea_cursor_right(field);
 else if(!strcmp(text,LV_SYMBOL_BACKSPACE))lv_textarea_delete_char(field);
 else lv_textarea_add_text(field,!strcmp(text,dsr_sc_tr("Spacja"))?" ":text);
}
void dsr_ui_keyboard(lv_obj_t *kb)
{
 const char *const *templates[]={lower,upper,symbols,symbols2,accents,numeric};
 for(unsigned m=0;m<6;m++){
  unsigned key=0,row=0;for(unsigned i=0;;i++){
   const char *text=templates[m][i];maps[m][i]=dsr_sc_tr(text);
   if(dsr_sc_language()==2&&m<2){if(!strcmp(text,"y"))maps[m][i]="z";else if(!strcmp(text,"z"))maps[m][i]="y";else if(!strcmp(text,"Y"))maps[m][i]="Z";else if(!strcmp(text,"Z"))maps[m][i]="Y";}
   if(!text[0])break;
   if(!strcmp(text,"\n")){row++;continue;}
   unsigned width=row==4?(key==0?5:key==1?4:key==2?9:key==3?4:8):(!strcmp(text,LV_SYMBOL_UP)||!strcmp(text,LV_SYMBOL_BACKSPACE)||!strcmp(text,"ABC"))?2:1;
   /* key is reset at every line below; no repeat avoids duplicate submit. */
   controls[m][i-row]=(lv_buttonmatrix_ctrl_t)(width|LV_BUTTONMATRIX_CTRL_CLICK_TRIG|LV_BUTTONMATRIX_CTRL_NO_REPEAT);
   key++;if(!strcmp(templates[m][i+1],"\n"))key=0;
  }
  lv_keyboard_set_map(kb,modes[m],maps[m],controls[m]);
 }
 lv_obj_remove_event_cb(kb,lv_keyboard_def_event_cb);lv_obj_add_event_cb(kb,keyboard_event,LV_EVENT_VALUE_CHANGED,NULL);
 lv_obj_set_size(kb,320,240);lv_obj_align(kb,LV_ALIGN_BOTTOM_MID,0,0);
 lv_obj_set_style_text_font(kb,&dsr_roboto_22,LV_PART_ITEMS);lv_obj_set_style_bg_color(kb,dsr_sc_color(DSR_SC_PANEL),0);
 lv_obj_set_style_pad_all(kb,4,0);lv_obj_set_style_pad_row(kb,4,0);lv_obj_set_style_pad_column(kb,3,0);
 lv_obj_set_style_bg_color(kb,lv_color_hex(0x45494d),LV_PART_ITEMS);lv_obj_set_style_text_color(kb,dsr_sc_color(DSR_SC_TEXT),LV_PART_ITEMS);
 lv_obj_set_style_bg_color(kb,dsr_sc_color(DSR_SC_BLUE),LV_PART_ITEMS|LV_STATE_PRESSED);lv_obj_set_style_bg_grad_dir(kb,LV_GRAD_DIR_NONE,LV_PART_ITEMS);
 lv_obj_set_style_border_width(kb,0,LV_PART_ITEMS);lv_obj_set_style_shadow_width(kb,0,LV_PART_ITEMS);lv_obj_set_style_radius(kb,7,LV_PART_ITEMS);
 lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_TEXT_LOWER);
}
void dsr_ui_keyboard_focus(lv_obj_t *kb,lv_obj_t *field)
{
 const char *accepted=lv_textarea_get_accepted_chars(field);
 bool number=accepted&&accepted[0]&&strspn(accepted,"0123456789.")==strlen(accepted);
 lv_keyboard_set_mode(kb,number?LV_KEYBOARD_MODE_NUMBER:LV_KEYBOARD_MODE_TEXT_LOWER);
 lv_keyboard_set_textarea(kb,field);lv_obj_remove_flag(kb,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(kb);
}
