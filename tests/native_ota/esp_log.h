#pragma once
/* Host model of ESP-IDF logging: format checked by the compiler, output kept
 * so the actor's failure diagnostics are visible in test logs. */
#include <stdio.h>
#define ESP_LOGW(tag,format,...) fprintf(stderr,"W %s: " format "\n",tag,##__VA_ARGS__)
#define ESP_LOGI(tag,format,...) do{(void)(tag);if(0)printf(format,##__VA_ARGS__);}while(0)
