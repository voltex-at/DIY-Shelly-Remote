#ifndef DSR_TEST_MBEDTLS_SHA256_H
#define DSR_TEST_MBEDTLS_SHA256_H
#include <stddef.h>
#include <openssl/evp.h>
typedef struct {EVP_MD_CTX *evp;} mbedtls_sha256_context;
void mbedtls_sha256_init(mbedtls_sha256_context *context);
int mbedtls_sha256_starts(mbedtls_sha256_context *context,int is224);
int mbedtls_sha256_update(mbedtls_sha256_context *context,const unsigned char *input,size_t length);
int mbedtls_sha256_finish(mbedtls_sha256_context *context,unsigned char output[32]);
void mbedtls_sha256_free(mbedtls_sha256_context *context);
/* Zero disables faults; otherwise fail the nth init/starts/update/finish.
 * free always releases the genuine OpenSSL context. */
void dsr_test_sha_fault(unsigned nth);
unsigned dsr_test_sha_calls(void);
unsigned dsr_test_sha_live(void);
#endif
