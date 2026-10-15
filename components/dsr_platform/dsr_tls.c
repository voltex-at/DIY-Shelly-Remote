#include "dsr_workspace.h"
#include "esp_random.h"
#include "nvs.h"
#include "mbedtls/ecp.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/oid.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <string.h>

static struct {uint32_t schema;unsigned char certificate[2048],key[512];} identity;
static char fingerprint[66];
static int random_bytes(void *context,unsigned char *data,size_t size)
{(void)context;esp_fill_random(data,size);return 0;}
esp_err_t dsr_tls_identity(void)
{
 nvs_handle_t handle;esp_err_t error=nvs_open("dsr_tls",NVS_READWRITE,&handle);if(error!=ESP_OK)return error;
 size_t length=sizeof(identity);error=nvs_get_blob(handle,"identity",&identity,&length);
 if(error==ESP_ERR_NVS_NOT_FOUND){
  mbedtls_pk_context key;mbedtls_pk_init(&key);mbedtls_x509write_cert cert;mbedtls_x509write_crt_init(&cert);mbedtls_mpi serial;mbedtls_mpi_init(&serial);
  uint8_t bytes[16];esp_fill_random(bytes,sizeof(bytes));bytes[0]=(bytes[0]&0x7f)|1;
  int result=mbedtls_pk_setup(&key,mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
  if(!result)result=mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,mbedtls_pk_ec(key),random_bytes,NULL);
  if(!result)result=mbedtls_mpi_read_binary(&serial,bytes,sizeof(bytes));
  mbedtls_x509write_crt_set_version(&cert,MBEDTLS_X509_CRT_VERSION_3);mbedtls_x509write_crt_set_md_alg(&cert,MBEDTLS_MD_SHA256);mbedtls_x509write_crt_set_subject_key(&cert,&key);mbedtls_x509write_crt_set_issuer_key(&cert,&key);
  if(!result)result=mbedtls_x509write_crt_set_serial(&cert,&serial);
  if(!result)result=mbedtls_x509write_crt_set_subject_name(&cert,"CN=shelly-remote.local,O=DIY Shelly Remote");
  if(!result)result=mbedtls_x509write_crt_set_issuer_name(&cert,"CN=shelly-remote.local,O=DIY Shelly Remote");
  if(!result)result=mbedtls_x509write_crt_set_validity(&cert,"20240101000000","20491231235959");
  if(!result)result=mbedtls_x509write_crt_set_basic_constraints(&cert,0,-1);
  if(!result)result=mbedtls_x509write_crt_set_key_usage(&cert,MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
  const char dns[]="shelly-remote.local";unsigned char san[2+2+sizeof(dns)-1];san[0]=0x30;san[1]=2+sizeof(dns)-1;san[2]=0x82;san[3]=sizeof(dns)-1;memcpy(san+4,dns,sizeof(dns)-1);
  if(!result)result=mbedtls_x509write_crt_set_extension(&cert,MBEDTLS_OID_SUBJECT_ALT_NAME,MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),0,san,sizeof(san));
  if(!result)result=mbedtls_pk_write_key_pem(&key,identity.key,sizeof(identity.key));
  if(!result)result=mbedtls_x509write_crt_pem(&cert,identity.certificate,sizeof(identity.certificate),random_bytes,NULL);
  mbedtls_mpi_free(&serial);mbedtls_x509write_crt_free(&cert);mbedtls_pk_free(&key);
  if(result){nvs_close(handle);return ESP_FAIL;}
  identity.schema=1;error=nvs_set_blob(handle,"identity",&identity,sizeof(identity));if(error==ESP_OK)error=nvs_commit(handle);
 }
 nvs_close(handle);
 if(error!=ESP_OK||length!=sizeof(identity)||identity.schema!=1||!memchr(identity.certificate,0,sizeof(identity.certificate))||!memchr(identity.key,0,sizeof(identity.key)))return ESP_FAIL;
 mbedtls_x509_crt cert;mbedtls_x509_crt_init(&cert);mbedtls_pk_context key;mbedtls_pk_init(&key);
 int result=mbedtls_x509_crt_parse(&cert,identity.certificate,strlen((char *)identity.certificate)+1);
 if(!result)result=mbedtls_pk_parse_key(&key,identity.key,strlen((char *)identity.key)+1,NULL,0,random_bytes,NULL);
 if(!result)result=mbedtls_pk_check_pair(&cert.pk,&key,random_bytes,NULL);
 if(!result){unsigned char digest[32];mbedtls_sha256(cert.raw.p,cert.raw.len,digest,0);for(unsigned i=0;i<32;i++)sprintf(fingerprint+i*2,"%02X",digest[i]);}
 mbedtls_pk_free(&key);mbedtls_x509_crt_free(&cert);return result?ESP_FAIL:ESP_OK;
}
const unsigned char *dsr_tls_certificate(void){return identity.certificate;}
const unsigned char *dsr_tls_key(void){return identity.key;}
const char *dsr_tls_fingerprint(void){return fingerprint;}
