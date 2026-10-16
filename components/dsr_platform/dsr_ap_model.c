#include "dsr_ap.h"
#include <ctype.h>
#include <string.h>
static bool same_text(const char *a,const char *b)
{while(*a&&*b){if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return false;}return *a==*b;}
bool dsr_ap_candidate(const char *ssid)
{
 if(!ssid)return false;
 size_t length=0;while(length<33&&ssid[length])length++;
 if(length>32||length<14)return false;
 const char *prefix="shelly";for(unsigned i=0;i<6;i++)if(tolower((unsigned char)ssid[i])!=prefix[i])return false;
 const char *dash=strrchr(ssid,'-');if(!dash)return false;size_t suffix=strlen(dash+1);if(suffix!=6&&suffix!=12)return false;
 for(const unsigned char *p=(const unsigned char *)ssid;*p;p++)if(!isalnum(*p)&&*p!='-')return false;
 for(const unsigned char *p=(const unsigned char *)dash+1;*p;p++)if(!isxdigit(*p))return false;
 return true;
}
bool dsr_ap_identity_matches(const char *ssid,const dsr_ap_identity_t *identity)
{
 if(!identity||!memchr(identity->id,0,sizeof(identity->id))||!memchr(identity->mac,0,sizeof(identity->mac))||!memchr(identity->ssid,0,sizeof(identity->ssid))||!dsr_ap_candidate(ssid)||strlen(identity->mac)!=12)return false;
 const char *dash=strrchr(ssid,'-');size_t length=strlen(dash+1);
 if(!same_text(dash+1,identity->mac+12-length))return false;
 return identity->generation==1||(identity->generation>=2&&identity->generation<=4&&same_text(ssid,identity->id));
}
