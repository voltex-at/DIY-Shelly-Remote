#include "dsr_core.h"
#include <stdio.h>
#include <string.h>

bool dsr_text_copy(char *out,size_t size,const char *text)
{
 if(!out||!size||!text)return false;
 out[0]=0;size_t used=0,kept=0;
 const unsigned char *p=(const unsigned char *)text;
 while(*p){
  unsigned n=*p<0x80?1:*p>=0xc2&&*p<=0xdf?2:*p>=0xe0&&*p<=0xef?3:*p>=0xf0&&*p<=0xf4?4:0;
  if(!n||*p<32||*p==127)return false;
  for(unsigned i=1;i<n;i++)if(!p[i]||(p[i]&0xc0)!=0x80)return false;
  if((n==3&&((*p==0xe0&&p[1]<0xa0)||(*p==0xed&&p[1]>=0xa0)))||
     (n==4&&((*p==0xf0&&p[1]<0x90)||(*p==0xf4&&p[1]>=0x90))))return false;
  used+=n;if(used<size)kept=used;p+=n;
 }
 memcpy(out,text,kept);out[kept]=0;return true;
}
void dsr_name_disambiguate(char name[48],const char *identity,uint8_t channel)
{
 char base[35];dsr_text_copy(base,sizeof(base),name);size_t n=identity?strlen(identity):0;
 snprintf(name,48,"%s · %.4s/%u",base,n>4?identity+n-4:identity?identity:"?",channel);
}
const char *dsr_device_model_name(const char *id)
{
 static const struct {const char *prefix,*name;} models[]={
  {"shellypro2cover-","Shelly Pro Dual Cover PM"},{"shellypro2pm-","Shelly Pro 2PM"},
  {"shellypro1pm-","Shelly Pro 1PM"},{"shellypro1-","Shelly Pro 1"},
  {"shellyplus2pm-","Shelly Plus 2PM"},{"shellyplus1pm-","Shelly Plus 1PM"},
  {"shellyplus1-","Shelly Plus 1"},{"shelly1pmminig3-","Shelly 1PM Mini Gen3"},
  {"shelly1minig3-","Shelly 1 Mini Gen3"},{"shellyhtg3-","Shelly H&T Gen3"},
  {"shellyplusrgbwpm-","Shelly Plus RGBW PM"},{"shellyblugwg3-","Shelly BLU Gateway Gen3"}};
 if(id)for(size_t i=0;i<sizeof(models)/sizeof(*models);i++)if(!strncmp(id,models[i].prefix,strlen(models[i].prefix)))return models[i].name;
 return NULL;
}

static bool ascii_digit(char c) { return c >= '0' && c <= '9'; }
uint8_t dsr_endpoint_from_kind(const char *kind)
{return kind&&!strcmp(kind,"cover")?DSR_ENDPOINT_COVER:kind&&!strcmp(kind,"light")?DSR_ENDPOINT_LIGHT:kind&&!strcmp(kind,"sensor")?DSR_ENDPOINT_HT:DSR_ENDPOINT_SWITCH;}
const char *dsr_endpoint_kind_name(uint8_t kind)
{return kind==DSR_ENDPOINT_COVER?"cover":(kind==DSR_ENDPOINT_LIGHT||kind==DSR_ENDPOINT_RGB)?"light":(kind==DSR_ENDPOINT_HT||kind==DSR_ENDPOINT_BLU)?"sensor":"relay";}
bool dsr_endpoint_layout_matches(uint8_t kind,const char *display)
{return kind==DSR_ENDPOINT_RGB?display&&!strcmp(display,"light"):kind==DSR_ENDPOINT_BLU?display&&(!strcmp(display,"sensor")||!strcmp(display,"contact")):kind==dsr_endpoint_from_kind(display);}
bool dsr_rgb_intent_valid(dsr_rgb_intent_t i)
{return i.command>=DSR_RGB_ON&&i.command<=DSR_RGB_SET_COLOR&&(i.command==DSR_RGB_SET_COLOR||(i.rgb[1]==0&&i.rgb[2]==0&&(i.command==DSR_RGB_SET_BRIGHTNESS?i.brightness<=100:i.brightness==0)));}
bool dsr_rgb_can_set(const dsr_rgb_status_t *s,dsr_rgb_intent_t i)
{return s&&dsr_rgb_intent_valid(i)&&(s->flags&DSR_RGB_VALID)&&!(s->flags&(DSR_RGB_TRANSITION|DSR_RGB_ERROR));}
dsr_result_t dsr_rgb_request_build(uint8_t channel,dsr_rgb_intent_t i,dsr_request_t *out)
{
 if(!out||channel!=0||!dsr_rgb_intent_valid(i))return DSR_INVALID;
 memset(out,0,sizeof(*out));out->post=true;strcpy(out->path,"/rpc/RGB.Set");
 if(i.command==DSR_RGB_SET_COLOR)snprintf(out->body,sizeof(out->body),"{\"id\":0,\"rgb\":[%u,%u,%u]}",i.rgb[0],i.rgb[1],i.rgb[2]);
 else if(i.command==DSR_RGB_SET_BRIGHTNESS)snprintf(out->body,sizeof(out->body),"{\"id\":0,\"brightness\":%u}",i.brightness);
 else snprintf(out->body,sizeof(out->body),"{\"id\":0,\"on\":%s}",i.command==DSR_RGB_ON?"true":"false");
 return DSR_OK;
}
bool dsr_light_intent_valid(dsr_light_intent_t i)
{return i.command>=DSR_LIGHT_ON&&i.command<=DSR_LIGHT_SET_BRIGHTNESS&&(i.command==DSR_LIGHT_SET_BRIGHTNESS?i.brightness<=100:i.brightness==0);}
bool dsr_light_can_set(const dsr_light_status_t *s,dsr_light_intent_t i)
{return s&&dsr_light_intent_valid(i)&&(s->flags&(DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS))==(DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS)&&!s->errors&&!s->conditions;}
dsr_result_t dsr_light_request_build(uint8_t channel,dsr_light_intent_t i,dsr_request_t *out)
{
 if(!out||channel>DSR_MAX_CHANNEL||!dsr_light_intent_valid(i))return DSR_INVALID;
 memset(out,0,sizeof(*out));out->post=true;strcpy(out->path,"/rpc/Light.Set");
 if(i.command==DSR_LIGHT_SET_BRIGHTNESS)snprintf(out->body,sizeof(out->body),"{\"id\":%u,\"brightness\":%u}",channel,i.brightness);
 else snprintf(out->body,sizeof(out->body),"{\"id\":%u,\"on\":%s}",channel,i.command==DSR_LIGHT_ON?"true":"false");
 return DSR_OK;
}
bool dsr_cover_intent_valid(dsr_cover_intent_t i)
{return i.command>=DSR_COVER_OPEN&&i.command<=DSR_COVER_GOTO&&(i.command==DSR_COVER_GOTO?i.position<=100:i.position==0);}
bool dsr_cover_can_move(const dsr_cover_status_t *s,dsr_cover_intent_t i)
{
 if(!s||!dsr_cover_intent_valid(i))return false;
 if(i.command==DSR_COVER_STOP)return true;
 if(!(s->flags&DSR_COVER_VALID)||s->errors||s->state==DSR_COVER_STATE_CALIBRATING)return false;
 return i.command!=DSR_COVER_GOTO||((s->flags&(DSR_COVER_POSITION_CONTROL|DSR_COVER_POSITION))==(DSR_COVER_POSITION_CONTROL|DSR_COVER_POSITION));
}
dsr_result_t dsr_cover_request_build(uint8_t channel,dsr_cover_intent_t i,dsr_request_t *out)
{
 if(!out||channel>DSR_MAX_CHANNEL||!dsr_cover_intent_valid(i))return DSR_INVALID;
 memset(out,0,sizeof(*out));const char *method=i.command==DSR_COVER_OPEN?"Open":i.command==DSR_COVER_CLOSE?"Close":i.command==DSR_COVER_STOP?"Stop":"GoToPosition";
 out->post=true;snprintf(out->path,sizeof(out->path),"/rpc/Cover.%s",method);
 if(i.command==DSR_COVER_GOTO)snprintf(out->body,sizeof(out->body),"{\"id\":%u,\"pos\":%u}",channel,i.position);
 else snprintf(out->body,sizeof(out->body),"{\"id\":%u}",channel);
 return DSR_OK;
}

bool dsr_ipv4_valid(const char *host)
{
    if (!host) return false;
    const char *p = host;
    unsigned first = 0;
    for (unsigned part = 0; part < 4; part++) {
        unsigned value = 0, digits = 0;
        const char *start = p;
        while (ascii_digit(*p)) {
            if (++digits > 3) return false;
            value = value * 10 + (unsigned)(*p++ - '0');
        }
        if (!digits || value > 255 || (digits > 1 && *start == '0')) return false;
        if (part == 0) first = value;
        if (part < 3) { if (*p++ != '.') return false; }
        else if (*p != '\0') return false;
    }
    /* Local device endpoint: refuse unspecified, multicast and broadcast ranges. */
    return first != 0 && first < 224 && first != 127;
}

dsr_result_t dsr_request_build(dsr_generation_t gen, uint8_t channel,
                             dsr_command_t cmd, bool on, dsr_request_t *out)
{
    if (!out || channel > DSR_MAX_CHANNEL) return DSR_INVALID;
    memset(out, 0, sizeof(*out));
    if (cmd != DSR_GET_STATUS && cmd != DSR_SET_OUTPUT) return DSR_INVALID;
    if (gen == DSR_GEN1) {
        snprintf(out->path, sizeof(out->path), "/relay/%u%s%s", (unsigned)channel,
                 cmd == DSR_SET_OUTPUT ? "?turn=" : "",
                 cmd == DSR_SET_OUTPUT ? (on ? "on" : "off") : "");
    } else if (gen == DSR_GEN2_PLUS) {
        if (cmd == DSR_GET_STATUS) {
            snprintf(out->path, sizeof(out->path), "/rpc/Switch.GetStatus?id=%u", (unsigned)channel);
        } else {
            out->post = true;
            snprintf(out->path, sizeof(out->path), "/rpc/Switch.Set");
            snprintf(out->body, sizeof(out->body), "{\"id\":%u,\"on\":%s}",
                     (unsigned)channel, on ? "true" : "false");
        }
    } else return DSR_UNSUPPORTED;
    return DSR_OK;
}

dsr_result_t dsr_registry_upsert(dsr_registry_t *registry, const dsr_device_t *device)
{
    if (!registry || !device || registry->count > DSR_MAX_DEVICES ||
        !memchr(device->host, '\0', sizeof(device->host)) ||
        !memchr(device->name, '\0', sizeof(device->name)) ||
        !dsr_ipv4_valid(device->host) || device->channel > DSR_MAX_CHANNEL || device->endpoint_kind>DSR_ENDPOINT_BLU ||
        ((device->endpoint_kind==DSR_ENDPOINT_RGB||device->endpoint_kind==DSR_ENDPOINT_HT)&&device->channel!=0) ||
        (device->endpoint_kind!=DSR_ENDPOINT_SWITCH&&device->generation!=DSR_GEN2_PLUS) ||
        (device->generation != DSR_GEN1 && device->generation != DSR_GEN2_PLUS)) return DSR_INVALID;
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->items[i].host, device->host) == 0 &&
            registry->items[i].channel == device->channel&&registry->items[i].endpoint_kind==device->endpoint_kind) {
            registry->items[i] = *device;
            return DSR_OK;
        }
    }
    if (registry->count == DSR_MAX_DEVICES) return DSR_FULL;
    registry->items[registry->count++] = *device;
    return DSR_OK;
}

bool dsr_version_parse(const char *text, uint32_t parts[3])
{
    if (!text || !parts) return false;
    if (*text == 'v') text++;
    uint32_t parsed[3];
    for (unsigned part = 0; part < 3; part++) {
        const char *start = text;
        uint32_t value = 0;
        while (ascii_digit(*text)) {
            uint32_t digit = (uint32_t)(*text++ - '0');
            if (value > (UINT32_MAX - digit) / 10) return false;
            value = value * 10 + digit;
        }
        if (text == start || (text - start > 1 && *start == '0')) return false;
        parsed[part] = value;
        if (part < 2) { if (*text++ != '.') return false; }
        else if (*text != '\0') return false;
    }
    memcpy(parts, parsed, sizeof(parsed));
    return true;
}

bool dsr_version_is_newer(const char *current, const char *candidate)
{
    uint32_t a[3], b[3];
    if (!dsr_version_parse(current, a) || !dsr_version_parse(candidate, b)) return false;
    for (unsigned i = 0; i < 3; i++) {
        if (b[i] != a[i]) return b[i] > a[i];
    }
    return false;
}

dsr_result_t dsr_update_metadata_check(const dsr_update_metadata_t *meta,
                                     const char *hardware_id, const char *current,
                                     size_t inactive_slot_size)
{
    if (!meta || !hardware_id || !*hardware_id || strcmp(hardware_id, "unconfigured") == 0 ||
        !meta->hardware_id || strcmp(meta->hardware_id, hardware_id) != 0 ||
        !dsr_version_is_newer(current, meta->version) || !meta->url ||
        strncmp(meta->url, "https://", 8) != 0 || !meta->url[8] ||
        meta->url[8] == '/' || !meta->sha256 || strlen(meta->sha256) != 64 ||
        !meta->size || meta->size > inactive_slot_size) return DSR_INVALID;
    for (const char *p = meta->url; *p; p++) {
        if ((unsigned char)*p <= 32 || *p == '@' || *p == '\\' || *p == '#') return DSR_INVALID;
    }
    for (const char *p = meta->sha256; *p; p++) {
        if (!ascii_digit(*p) && !(*p >= 'a' && *p <= 'f')) return DSR_INVALID;
    }
    return DSR_OK;
}
