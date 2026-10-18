#ifndef DSR_DEVICE_SETTINGS_H
#define DSR_DEVICE_SETTINGS_H
#include "dsr_platform.h"
#include "cJSON.h"

#define DSR_SETTINGS_GROUPS 32u
#define DSR_SETTINGS_FIELDS 80u
typedef enum {DSR_SETTING_BOOL,DSR_SETTING_NUMBER,DSR_SETTING_TEXT,DSR_SETTING_SECRET,DSR_SETTING_ENUM} dsr_setting_type_t;
typedef struct {
 const char *component,*path,*label,*choices;
 double minimum,maximum;
 uint16_t max_length;
 uint8_t type,flags;
} dsr_setting_field_t;
enum {DSR_SETTING_INTEGER=1,DSR_SETTING_NETWORK=2,DSR_SETTING_RESTART=4};
typedef struct {char component[20];uint16_t id;bool singleton,remote;} dsr_setting_group_t;
typedef struct {
 uint32_t serial,operation_id;
 dsr_result_t result;
 bool busy,ready,uncertain,restart_required;
 bool blu_registered,blu_receiving;
 uint16_t blu_cid;
 uint8_t group_count,field_count,selected;
 dsr_setting_group_t groups[DSR_SETTINGS_GROUPS];
 uint16_t fields[DSR_SETTINGS_FIELDS];
} dsr_device_settings_snapshot_t;
const dsr_setting_field_t *dsr_setting_field(unsigned index);
const cJSON *dsr_setting_value(const cJSON *configuration,const char *path);
dsr_result_t dsr_setting_patch(const cJSON *configuration,unsigned field,const char *value,cJSON **patch);
dsr_result_t dsr_setting_network_patch(const cJSON *configuration,unsigned field,const char *ssid,const char *password,cJSON **patch);
void dsr_setting_json_delete(cJSON *value);
bool dsr_setting_equal(const cJSON *a,const cJSON *b);
dsr_result_t dsr_setting_groups(const cJSON *methods,const cJSON *components,dsr_setting_group_t *groups,unsigned *count);
unsigned dsr_setting_fields(const dsr_setting_group_t *group,const cJSON *configuration,uint16_t *fields,unsigned capacity);
/* One private worker workspace. Snapshots never contain passwords or keys. */
esp_err_t dsr_device_settings_init(void);
dsr_result_t dsr_device_settings_prepare(uint32_t operation,const dsr_connection_t *connection,unsigned action,unsigned selection,const char *value);
bool dsr_device_settings_admitted(const dsr_job_t *job);
void dsr_device_settings_reject(uint32_t operation);
void dsr_device_settings_execute(const dsr_job_t *job,dsr_event_t *event);
void dsr_device_settings_snapshot(dsr_device_settings_snapshot_t *out);
bool dsr_device_settings_value(unsigned field,char *value,size_t capacity);
void dsr_device_settings_close(void);
bool dsr_device_settings_blu_connection(dsr_connection_t *out);
void dsr_device_settings_visibility(bool visible);
bool dsr_device_settings_sensitive(void);
enum {DSR_SETTINGS_LOAD,DSR_SETTINGS_GROUP,DSR_SETTINGS_SAVE,DSR_SETTINGS_BLU_ADD};
/* Sole worker only, HTTP transport retains identity/auth/cancellation guards. */
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *connection,const char *method,const cJSON *params,cJSON **out,bool mutating,bool *attempted);
dsr_result_t dsr_http_settings_identity(const dsr_connection_t *connection);
#endif
