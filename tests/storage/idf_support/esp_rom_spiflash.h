#pragma once
#include <stdint.h>
typedef struct {uint32_t page_size, sector_size;} test_flashchip_t;
extern test_flashchip_t g_rom_flashchip;
