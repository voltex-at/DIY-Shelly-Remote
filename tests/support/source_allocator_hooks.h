/* Cygwin PE import wrapping recurses during OpenSSL DLL startup. Instrument
 * the same production allocations directly, preserving all OOM assertions. */
#ifndef DSR_SOURCE_ALLOCATOR_HOOKS_H
#define DSR_SOURCE_ALLOCATOR_HOOKS_H
#include <stdlib.h>
void *__wrap_malloc(size_t size);
void *__wrap_calloc(size_t count,size_t size);
void __wrap_free(void *pointer);
#define malloc __wrap_malloc
#define calloc __wrap_calloc
#define free __wrap_free
#endif
