/* Genuine OpenSSL AES-256-GCM behind the pinned mbedTLS API shape. Target
 * compilation and hardware verify the actual SDK backend separately. */
#include "mbedtls/gcm.h"
#include <openssl/evp.h>
#include <string.h>
void mbedtls_gcm_init(mbedtls_gcm_context *c){memset(c,0,sizeof(*c));}
void mbedtls_gcm_free(mbedtls_gcm_context *c){OPENSSL_cleanse(c,sizeof(*c));}
int mbedtls_gcm_setkey(mbedtls_gcm_context *c,int kind,const unsigned char *k,unsigned bits)
{if(kind!=MBEDTLS_CIPHER_ID_AES||bits!=256)return -1;memcpy(c->key,k,32);c->bits=bits;return 0;}
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *c,int mode,size_t n,const unsigned char *iv,size_t ivn,const unsigned char *aad,size_t an,const unsigned char *in,unsigned char *out,size_t tn,unsigned char *tag)
{
 if(mode!=MBEDTLS_GCM_ENCRYPT||c->bits!=256||n>115200||ivn!=12||tn!=16)return -1;
 EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();if(!ctx)return -1;int len=0,total=0;
 int ok=EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL)&&EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,(int)ivn,NULL)&&EVP_EncryptInit_ex(ctx,NULL,NULL,c->key,iv)&&EVP_EncryptUpdate(ctx,NULL,&len,aad,(int)an)&&EVP_EncryptUpdate(ctx,out,&len,in,(int)n);total=len;
 if(ok)ok=EVP_EncryptFinal_ex(ctx,out+total,&len)&&total+len==(int)n&&EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,(int)tn,tag);
 EVP_CIPHER_CTX_free(ctx);return ok?0:-1;
}
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *c,size_t n,const unsigned char *iv,size_t ivn,const unsigned char *aad,size_t an,const unsigned char *tag,size_t tn,const unsigned char *in,unsigned char *out)
{
 if(c->bits!=256||n>115200||ivn!=12||tn!=16)return -1;
 EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();if(!ctx)return -1;int len=0,total=0;
 int ok=EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL)&&EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,(int)ivn,NULL)&&EVP_DecryptInit_ex(ctx,NULL,NULL,c->key,iv)&&EVP_DecryptUpdate(ctx,NULL,&len,aad,(int)an)&&EVP_DecryptUpdate(ctx,out,&len,in,(int)n);total=len;
 if(ok)ok=EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,(int)tn,(void *)tag)&&EVP_DecryptFinal_ex(ctx,out+total,&len)&&total+len==(int)n;
 EVP_CIPHER_CTX_free(ctx);return ok?0:-1;
}
