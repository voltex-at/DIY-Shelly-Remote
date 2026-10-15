#pragma once
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_SPIRAM 4
static inline size_t heap_caps_get_free_size(unsigned flags){(void)flags;return 100000;}
static inline size_t heap_caps_get_largest_free_block(unsigned flags){(void)flags;return 90000;}
