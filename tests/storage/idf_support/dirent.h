#pragma once
/* IDF/newlib embeds DIR by value even with directory VFS operations disabled. */
typedef struct {int unused;} DIR;
struct dirent {char d_name[256];};
DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int closedir(DIR *);
