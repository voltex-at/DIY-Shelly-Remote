#include "dsr_platform.h"
#include "nvs.h"
#include "dsr_activity.h"
#include <string.h>
#include <stdlib.h>
/* Exact schema-1 layout from before volatile telemetry was added. Keeping the
 * persistent type explicit preserves installed bindings across OTA updates. */
typedef struct {
 char host[16],name[48],device_id[64];
 int32_t generation;uint8_t channel;bool online,has_switch,has_power,output;
 double power_w;
} legacy_device_v1_t;
typedef struct {legacy_device_v1_t device;dsr_credentials_t credentials;} connection_record_v1_t;
typedef struct {uint32_t schema,count;struct {legacy_device_v1_t device;dsr_credentials_t credentials;uint32_t layout_id;} items[16];} legacy_registry_v2_t;
_Static_assert(sizeof(legacy_device_v1_t)==152&&offsetof(legacy_device_v1_t,power_w)==144&&sizeof(connection_record_v1_t)==256&&sizeof(legacy_registry_v2_t)==4104,"frozen installed NVS ABI");
static dsr_device_t import_legacy(const legacy_device_v1_t *v)
{
 dsr_device_t d={0};memcpy(d.host,v->host,16);memcpy(d.name,v->name,48);memcpy(d.device_id,v->device_id,64);
 d.generation=(dsr_generation_t)v->generation;d.channel=v->channel;d.endpoint_kind=DSR_ENDPOINT_SWITCH;
 /* Legacy telemetry and all padding bytes have no authority. */
 return d;
}

bool dsr_connection_valid(const dsr_connection_t *c, bool bound)
{
    if (!c || !memchr(c->device.host, 0, sizeof(c->device.host)) ||
        !memchr(c->device.name, 0, sizeof(c->device.name)) ||
        !memchr(c->device.device_id, 0, sizeof(c->device.device_id)) ||
        !memchr(c->credentials.username, 0, sizeof(c->credentials.username)) ||
        !memchr(c->credentials.password, 0, sizeof(c->credentials.password)) ||
        !dsr_ipv4_valid(c->device.host) || c->device.channel > DSR_MAX_CHANNEL || c->device.endpoint_kind>DSR_ENDPOINT_HT ||
        ((c->device.endpoint_kind==DSR_ENDPOINT_RGB||c->device.endpoint_kind==DSR_ENDPOINT_HT)&&c->device.channel!=0) ||
        (c->device.endpoint_kind!=DSR_ENDPOINT_SWITCH&&c->device.generation!=DSR_GEN2_PLUS)) return false;
    return !bound || (c->device.device_id[0] &&
        (c->device.generation == DSR_GEN1 || c->device.generation == DSR_GEN2_PLUS));
}

esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found)
{
    if (!out || !found) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out)); *found = false;
    nvs_handle_t handle;
    esp_err_t error = nvs_open("dsr_device", NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (error != ESP_OK) return error;
    uint32_t schema = 0;
    error = nvs_get_u32(handle, "schema", &schema);
    if (error == ESP_OK && schema != 1) error = ESP_ERR_NOT_SUPPORTED;
    connection_record_v1_t loaded = {0};
    size_t length = sizeof(loaded);
    if (error == ESP_OK) error = nvs_get_blob(handle, "connection", &loaded, &length);
    dsr_connection_t candidate={.device=import_legacy(&loaded.device),.credentials=loaded.credentials};
    if (error == ESP_OK && (length != sizeof(loaded) || !dsr_connection_valid(&candidate, true))) error = ESP_ERR_INVALID_SIZE;
    if (error == ESP_OK) {
        candidate.legacy_fallback=true;*out=candidate;*found=true;
    }
    nvs_close(handle);
    return error;
}

/* New namespace avoids rewriting the installed schema-1 record during OTA.
 * NVS replaces one atomic blob; no cross-key count/schema transaction. On an
 * existing but unreadable registry we fail closed, never resurrect legacy. */
typedef struct {
    uint32_t schema,count;
    struct {dsr_device_t device;dsr_credentials_t credentials;uint32_t layout_id;} items[DSR_MAX_DEVICES];
} connection_registry_v6_t;
_Static_assert(sizeof(connection_registry_v6_t)==sizeof(legacy_registry_v2_t),"registry v6 fixed footprint");
static bool same_endpoint(const dsr_device_t *a,const dsr_device_t *b)
{return !strcmp(a->device_id,b->device_id)&&a->generation==b->generation&&a->endpoint_kind==b->endpoint_kind&&a->channel==b->channel;}
static esp_err_t registry_read_origin(connection_registry_v6_t *registry,bool *legacy_origin)
{
    if(legacy_origin)*legacy_origin=false;
    memset(registry,0,sizeof(*registry));registry->schema=6;
    nvs_handle_t handle;esp_err_t error=nvs_open("dsr_bindings",NVS_READONLY,&handle);
    if(error!=ESP_OK&&error!=ESP_ERR_NVS_NOT_FOUND)return error;
    if(error==ESP_OK){
        size_t length=sizeof(*registry);error=nvs_get_blob(handle,"registry",registry,&length);nvs_close(handle);
        if(error!=ESP_ERR_NVS_NOT_FOUND){
            if(error!=ESP_OK)return error;
            if(length!=sizeof(*registry)||registry->count>DSR_MAX_DEVICES||(registry->schema!=2&&registry->schema!=3&&registry->schema!=4&&registry->schema!=5&&registry->schema!=6))return ESP_ERR_INVALID_SIZE;
            if(registry->schema==2){
                for(uint32_t i=0;i<registry->count;i++){
                    legacy_device_v1_t old;memcpy(&old,&registry->items[i].device,sizeof(old));
                    registry->items[i].device=import_legacy(&old);
                }
                /* Frozen v2 padding has no transport authority. */
            }
            for(uint32_t i=0;i<registry->count;i++){
                if((registry->schema==3&&registry->items[i].device.endpoint_kind>DSR_ENDPOINT_COVER)||(registry->schema==4&&registry->items[i].device.endpoint_kind>DSR_ENDPOINT_LIGHT)||(registry->schema==5&&registry->items[i].device.endpoint_kind>DSR_ENDPOINT_RGB))return ESP_ERR_INVALID_SIZE;
                dsr_connection_t candidate={.device=registry->items[i].device,.credentials=registry->items[i].credentials};
                if(!dsr_connection_valid(&candidate,true)||registry->items[i].layout_id>1000000)return ESP_ERR_INVALID_SIZE;
                for(uint32_t j=0;j<i;j++)if(same_endpoint(&registry->items[i].device,&registry->items[j].device)||(registry->items[i].layout_id&&registry->items[i].layout_id==registry->items[j].layout_id))return ESP_ERR_INVALID_SIZE;
            }
            registry->schema=6; /* Lazy RAM import; no write until a user mutation. */
            return ESP_OK;
        }
    }
    dsr_connection_t legacy;bool found=false;error=dsr_connection_load(&legacy,&found);
    if(error==ESP_OK){memset(registry,0,sizeof(*registry));registry->schema=6;if(found){registry->count=1;registry->items[0].device=legacy.device;registry->items[0].credentials=legacy.credentials;if(legacy_origin)*legacy_origin=true;}}
    memset(&legacy,0,sizeof(legacy));return error;
}
static esp_err_t registry_read(connection_registry_v6_t *registry)
{return registry_read_origin(registry,NULL);}
esp_err_t dsr_connections_load(dsr_connection_t *out,size_t capacity,size_t *count)
{
    if(!out||!count)return ESP_ERR_INVALID_ARG;
    *count=0;connection_registry_v6_t *registry=calloc(1,sizeof(*registry));if(!registry)return ESP_ERR_NO_MEM;
    bool legacy_origin=false;esp_err_t error=registry_read_origin(registry,&legacy_origin);
    if(error==ESP_OK&&registry->count>capacity)error=ESP_ERR_INVALID_SIZE;
    if(error==ESP_OK){
        for(uint32_t i=0;i<registry->count;i++){
            out[i]=(dsr_connection_t){.device=registry->items[i].device,.credentials=registry->items[i].credentials,.layout_id=registry->items[i].layout_id,.legacy_fallback=legacy_origin};
            out[i].device.online=out[i].device.has_switch=out[i].device.has_power=false;out[i].device.output=false;out[i].device.power_w=0;
        }
        *count=registry->count;
    }
    memset(registry,0,sizeof(*registry));free(registry);return error;
}
static esp_err_t registry_write(const connection_registry_v6_t *registry)
{
 nvs_handle_t handle;esp_err_t error=nvs_open("dsr_bindings",NVS_READWRITE,&handle);
 if(error!=ESP_OK)return error;
 error=nvs_set_blob(handle,"registry",registry,sizeof(*registry));if(error==ESP_OK)error=nvs_commit(handle);nvs_close(handle);
 if(error!=ESP_OK)return error;
 connection_registry_v6_t *readback=calloc(1,sizeof(*readback));if(!readback)return ESP_ERR_NO_MEM;
 error=nvs_open("dsr_bindings",NVS_READONLY,&handle);
 if(error==ESP_OK){size_t length=sizeof(*readback);error=nvs_get_blob(handle,"registry",readback,&length);if(error==ESP_OK&&(length!=sizeof(*readback)||memcmp(registry,readback,sizeof(*registry))))error=ESP_FAIL;nvs_close(handle);}
 memset(readback,0,sizeof(*readback));free(readback);return error;
}
esp_err_t dsr_connection_save(const dsr_connection_t *connection)
{
    if(!dsr_connection_valid(connection,true)||connection->layout_id>1000000)return ESP_ERR_INVALID_ARG;
    if(!dsr_activity_begin())return ESP_ERR_INVALID_STATE;
    connection_registry_v6_t *registry=calloc(1,sizeof(*registry));
    if(!registry){dsr_activity_end();return ESP_ERR_NO_MEM;}
    esp_err_t error=registry_read(registry);
    if(error==ESP_OK){
        uint32_t at=0;for(;at<registry->count;at++)if(same_endpoint(&registry->items[at].device,&connection->device))break;
        if(at==DSR_MAX_DEVICES)error=ESP_ERR_NO_MEM;
        else if(connection->layout_id){for(uint32_t i=0;i<registry->count;i++)if(i!=at&&registry->items[i].layout_id==connection->layout_id)error=ESP_ERR_INVALID_ARG;}
        if(error==ESP_OK){
            if(at==registry->count)registry->count++;
            /* One physical identity may expose several independent channels. A
             * verified DHCP/auth update applies to its siblings, preserving names. */
            for(uint32_t i=0;i<registry->count;i++)if(registry->items[i].device.generation==connection->device.generation&&!strcmp(registry->items[i].device.device_id,connection->device.device_id)){
                memcpy(registry->items[i].device.host,connection->device.host,sizeof(connection->device.host));registry->items[i].credentials=connection->credentials;
            }
            registry->items[at].device=connection->device;registry->items[at].credentials=connection->credentials;if(connection->layout_id)registry->items[at].layout_id=connection->layout_id;
            dsr_device_t *device=&registry->items[at].device;device->online=device->has_switch=device->has_power=device->output=false;device->power_w=0;
            error=registry_write(registry);
        }
    }
    memset(registry,0,sizeof(*registry));free(registry);dsr_activity_end();return error;
}

static esp_err_t registry_change(const dsr_connection_t *connection,bool remove)
{
 if(!dsr_connection_valid(connection,true)||(!remove&&(!connection->layout_id||connection->layout_id>1000000)))return ESP_ERR_INVALID_ARG;
 if(!dsr_activity_begin())return ESP_ERR_INVALID_STATE;
 connection_registry_v6_t *registry=calloc(1,sizeof(*registry));esp_err_t error=registry?registry_read(registry):ESP_ERR_NO_MEM;
 if(error==ESP_OK){uint32_t at=0;while(at<registry->count&&!same_endpoint(&registry->items[at].device,&connection->device))at++;
  /* A previous remove may have reached Flash before commit/readback failed.
   * Verified absence is its durable ACK; never rewrite unrelated endpoints. */
  if(at==registry->count)error=remove?ESP_OK:ESP_ERR_NOT_FOUND;
  else {if(!remove)for(uint32_t i=0;i<registry->count;i++)if(i!=at&&registry->items[i].layout_id==connection->layout_id)error=ESP_ERR_INVALID_ARG;
   if(error==ESP_OK){if(remove){memmove(&registry->items[at],&registry->items[at+1],(registry->count-at-1)*sizeof(registry->items[0]));memset(&registry->items[--registry->count],0,sizeof(registry->items[0]));}else registry->items[at].layout_id=connection->layout_id;error=registry_write(registry);}}
 }
 if(registry){memset(registry,0,sizeof(*registry));free(registry);}dsr_activity_end();return error;
}
esp_err_t dsr_connection_publish(const dsr_connection_t *connection){return registry_change(connection,false);}
esp_err_t dsr_connection_remove(const dsr_connection_t *connection){return registry_change(connection,true);}

esp_err_t dsr_connection_layout_id(const dsr_connection_t *connection,uint32_t *id)
{
 if(!connection||!id)return ESP_ERR_INVALID_ARG;
 connection_registry_v6_t *r=calloc(1,sizeof(*r));if(!r)return ESP_ERR_NO_MEM;
 esp_err_t e=registry_read(r);if(e==ESP_OK){e=ESP_ERR_NOT_FOUND;for(uint32_t i=0;i<r->count;i++)if(same_endpoint(&r->items[i].device,&connection->device)){*id=r->items[i].layout_id;e=ESP_OK;break;}}
 memset(r,0,sizeof(*r));free(r);return e;
}
