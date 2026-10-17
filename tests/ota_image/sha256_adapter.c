/* Real SHA-256 provider behind the SDK-shaped test interface. */
#include "mbedtls/sha256.h"
#include <stdlib.h>
static unsigned fail_at,calls,live;
static int fails(void){calls++;return fail_at&&calls==fail_at;}
void dsr_test_sha_fault(unsigned nth){fail_at=nth;calls=0;}
unsigned dsr_test_sha_calls(void){return calls;}
unsigned dsr_test_sha_live(void){return live;}
void mbedtls_sha256_init(mbedtls_sha256_context *context)
{context->evp=fails()?NULL:EVP_MD_CTX_new();if(context->evp)live++;}
int mbedtls_sha256_starts(mbedtls_sha256_context *context,int is224)
{if(fails()||!context->evp||is224)return -1;return EVP_DigestInit_ex(context->evp,EVP_sha256(),NULL)==1?0:-1;}
int mbedtls_sha256_update(mbedtls_sha256_context *context,const unsigned char *input,size_t length)
{if(fails()||!context->evp)return -1;return EVP_DigestUpdate(context->evp,input,length)==1?0:-1;}
int mbedtls_sha256_finish(mbedtls_sha256_context *context,unsigned char output[32])
{unsigned length=0;if(fails()||!context->evp)return -1;return EVP_DigestFinal_ex(context->evp,output,&length)==1&&length==32?0:-1;}
void mbedtls_sha256_free(mbedtls_sha256_context *context)
{if(context->evp){EVP_MD_CTX_free(context->evp);context->evp=NULL;if(!live)abort();live--;}}
