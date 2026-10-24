#ifndef DSR_TEST_GCM_H
#define DSR_TEST_GCM_H
#include <stddef.h>
typedef struct {unsigned char key[32];unsigned bits;} mbedtls_gcm_context;
#define MBEDTLS_CIPHER_ID_AES 2
#define MBEDTLS_GCM_ENCRYPT 1
void mbedtls_gcm_init(mbedtls_gcm_context *);
void mbedtls_gcm_free(mbedtls_gcm_context *);
int mbedtls_gcm_setkey(mbedtls_gcm_context *,int,const unsigned char *,unsigned);
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *,int,size_t,const unsigned char *,size_t,const unsigned char *,size_t,const unsigned char *,unsigned char *,size_t,unsigned char *);
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *,size_t,const unsigned char *,size_t,const unsigned char *,size_t,const unsigned char *,size_t,const unsigned char *,unsigned char *);
#endif
