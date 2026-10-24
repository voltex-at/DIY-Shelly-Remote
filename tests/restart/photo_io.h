/* Forced include for the production photo translation unit ONLY. Global GNU
 * wrapping of libc startup/stdio is not portable to Cygwin's DLL runtime. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void *__wrap_malloc(size_t);
FILE *__wrap_fopen(const char *,const char *);
size_t __wrap_fwrite(const void *,size_t,size_t,FILE *);
int __wrap_fflush(FILE *);
int __wrap_fileno(FILE *);
int __wrap_fsync(int);
int __wrap_fclose(FILE *);
#define malloc __wrap_malloc
#define fopen __wrap_fopen
#define fwrite __wrap_fwrite
#define fflush __wrap_fflush
#define fileno __wrap_fileno
#define fsync __wrap_fsync
#define fclose __wrap_fclose
