#ifndef DSR_UI_SEARCH_H
#define DSR_UI_SEARCH_H
/* Latin case-insensitive search for the supported PL/EN/DE UI. */
#include <stdint.h>
static inline uint32_t dsr_ui_search_letter(const char **text)
{
 const unsigned char *p=(const unsigned char *)*text;uint32_t c=*p++;if(c>=0xc2&&c<=0xdf&&(*p&0xc0)==0x80){c=((c&31)<<6)|(*p++&63);}*text=(const char *)p;
 if(c>='A'&&c<='Z')return c+32;
 if(c>=0xc0&&c<=0xde&&c!=0xd7)return c+32;
 const uint16_t upper[]={0x104,0x106,0x118,0x141,0x143,0x15a,0x179,0x17b};for(unsigned i=0;i<sizeof(upper)/sizeof(*upper);i++)if(c==upper[i])return c+1;return c;
}
static inline int dsr_ui_search_match(const char *text,const char *query)
{
 if(!query||!*query)return 1;
 for(;text&&*text;text++){const char *a=text,*b=query;while(*a&&*b){if(dsr_ui_search_letter(&a)!=dsr_ui_search_letter(&b))break;if(!*b)return 1;}}return 0;
}
#endif
