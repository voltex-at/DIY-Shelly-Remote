#ifndef DSR_TEST_STORAGE_H
#define DSR_TEST_STORAGE_H
#include <stdbool.h>
extern unsigned test_nvs_calls,test_nvs_writes;
extern unsigned test_active_writes;
extern int test_nvs_fail_after;
extern bool test_boot_allowed;
void test_nvs_reset(void);
#endif
