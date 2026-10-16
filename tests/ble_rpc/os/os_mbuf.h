#pragma once
#include <stdint.h>
struct os_mbuf {const uint8_t *data;unsigned length;struct os_mbuf *next;};
unsigned test_mbuf_length(const struct os_mbuf *buffer);
#define OS_MBUF_PKTLEN(om) test_mbuf_length(om)
int os_mbuf_copydata(const struct os_mbuf *buffer,int offset,int length,void *out);
