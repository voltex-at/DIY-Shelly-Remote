#ifndef DSR_MAINTENANCE_IO_H
#define DSR_MAINTENANCE_IO_H
#include "dsr_maintenance_model.h"
typedef struct dsr_maint_file dsr_maint_file_t;
/* Private deterministic stage names only; no archive/browser paths. Each
 * read/write call is bounded to DSR_MAINT_IO_CHUNK. Ports never touch OTA. */
dsr_result_t dsr_maint_io_limits(dsr_maint_limits_t *);
dsr_result_t dsr_maint_io_random(uint8_t *out,size_t size);
/* Nonsecret unique operation label, permitted with RF off for local reset.
 * Never use this as encryption key, IV or web authorization capability. */
dsr_result_t dsr_maint_io_identifier(uint8_t out[16]);
dsr_result_t dsr_maint_io_next_revision(uint32_t imported,uint32_t *out);
void *dsr_maint_io_alloc(size_t size);
void dsr_maint_io_free(void *owned,size_t size);
dsr_result_t dsr_maint_io_exists(const char *name,bool *exists);
dsr_result_t dsr_maint_io_open_write(const char *name,dsr_maint_file_t **out);
dsr_result_t dsr_maint_io_open_read(const char *name,dsr_maint_file_t **out,size_t *size);
dsr_result_t dsr_maint_io_write(dsr_maint_file_t *,const uint8_t *,size_t size);
dsr_result_t dsr_maint_io_read(dsr_maint_file_t *,uint8_t *,size_t size,size_t *read);
dsr_result_t dsr_maint_io_close(dsr_maint_file_t *);
dsr_result_t dsr_maint_io_remove(const char *name);
#endif
