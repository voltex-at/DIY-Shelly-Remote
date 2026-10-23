#ifndef AUTOMATION_NVS_FIXTURE_H
#define AUTOMATION_NVS_FIXTURE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
extern uint8_t auto_flash[2][3072];
extern size_t auto_lengths[2];
extern unsigned auto_fault,auto_commits,auto_sets,auto_opens;
void auto_flash_reset(void);
void auto_flash_fault(unsigned fault);
#endif
