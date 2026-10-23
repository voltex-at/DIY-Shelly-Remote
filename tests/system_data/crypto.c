/* Host provider uses genuine OpenSSL SHA-256, not a canned digest. */
#include "mbedtls/sha256.h"
#include <openssl/evp.h>
int mbedtls_sha256(const unsigned char *data,size_t size,unsigned char out[32],int is224)
{unsigned length=0;return !is224&&EVP_Digest(data,size,out,&length,EVP_sha256(),NULL)&&length==32?0:-1;}
