/* Host provider uses genuine OpenSSL SHA-256, not a canned digest. */
#include "mbedtls/sha256.h"
#include <openssl/evp.h>
void mbedtls_sha256_init(mbedtls_sha256_context *c){c->opaque=NULL;}
void mbedtls_sha256_free(mbedtls_sha256_context *c){EVP_MD_CTX_free(c->opaque);c->opaque=NULL;}
int mbedtls_sha256_starts(mbedtls_sha256_context *c,int is224)
{if(is224)return -1;EVP_MD_CTX_free(c->opaque);c->opaque=EVP_MD_CTX_new();return c->opaque&&EVP_DigestInit_ex(c->opaque,EVP_sha256(),NULL)?0:-1;}
int mbedtls_sha256_update(mbedtls_sha256_context *c,const unsigned char *p,size_t n)
{return c->opaque&&EVP_DigestUpdate(c->opaque,p,n)?0:-1;}
int mbedtls_sha256_finish(mbedtls_sha256_context *c,unsigned char out[32])
{unsigned n=0;return c->opaque&&EVP_DigestFinal_ex(c->opaque,out,&n)&&n==32?0:-1;}
int mbedtls_sha256(const unsigned char *data,size_t size,unsigned char out[32],int is224)
{unsigned length=0;return !is224&&EVP_Digest(data,size,out,&length,EVP_sha256(),NULL)&&length==32?0:-1;}
