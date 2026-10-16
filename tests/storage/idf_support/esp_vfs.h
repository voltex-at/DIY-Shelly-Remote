#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include "esp_err.h"
#define ESP_VFS_PATH_MAX 15
#define ESP_VFS_FLAG_CONTEXT_PTR 1
#define ESP_VFS_FLAG_STATIC 2
#define ESP_VFS_FLAG_READONLY_FS 4
typedef struct {
 ssize_t (*write_p)(void *,int,const void *,size_t);
 off_t (*lseek_p)(void *,int,off_t,int);
 ssize_t (*read_p)(void *,int,void *,size_t);
 int (*open_p)(void *,const char *,int,int);
 int (*close_p)(void *,int);
 int (*fstat_p)(void *,int,struct stat *);
 int (*fsync_p)(void *,int);
} esp_vfs_fs_ops_t;
esp_err_t esp_vfs_register_fs(const char *,const esp_vfs_fs_ops_t *,int,void *);
esp_err_t esp_vfs_unregister(const char *);
