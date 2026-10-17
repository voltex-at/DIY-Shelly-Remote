#ifndef DSR_TEST_PHOTO_SHA256_H
#define DSR_TEST_PHOTO_SHA256_H
#include <stddef.h>
int mbedtls_sha256(const unsigned char *,size_t,unsigned char[32],int);
#endif
