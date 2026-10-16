/* Public known-answer vectors: RFC 8032 section 7.1, Ed25519 tests 1–3.
 * https://www.rfc-editor.org/rfc/rfc8032#section-7.1
 * No private keys, signer or production trust store are used here. */
#include "monocypher-ed25519.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned hex_digit(char c)
{CHECK((c>='0'&&c<='9')||(c>='a'&&c<='f'));return c<='9'?(unsigned)(c-'0'):(unsigned)(c-'a'+10);}
static size_t unhex(uint8_t *out,const char *hex)
{size_t n=strlen(hex);CHECK(!(n&1));for(size_t i=0;i<n/2;i++)out[i]=(uint8_t)(hex_digit(hex[i*2])*16+hex_digit(hex[i*2+1]));return n/2;}
int main(void)
{
    static const struct {const char *key,*message,*signature;} vectors[]={
        {"d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a","",
         "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"},
        {"3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c","72",
         "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"},
        {"fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025","af82",
         "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a"}
    };
    uint8_t order[32];CHECK(unhex(order,"edd3f55c1a631258d69cf7a2def9de1400000000000000000000000000000010")==32);
    for(unsigned v=0;v<3;v++){
        uint8_t key[32],signature[64],message[3]={0};
        CHECK(unhex(key,vectors[v].key)==32);CHECK(unhex(signature,vectors[v].signature)==64);
        size_t length=unhex(message,vectors[v].message);
        CHECK(crypto_ed25519_check(signature,key,message,length)==0);
        /* Correct buffers are always passed to the fixed-length vendor API;
         * transport-level truncated key/signature rejection belongs to the
         * manifest parser, not an out-of-bounds test of vendor preconditions. */
        for(unsigned byte=0;byte<64;byte++)for(unsigned bit=0;bit<8;bit++){
            signature[byte]^=(uint8_t)(1u<<bit);
            CHECK(crypto_ed25519_check(signature,key,message,length)==-1);
            signature[byte]^=(uint8_t)(1u<<bit);
        }
        for(unsigned byte=0;byte<32;byte++){
            key[byte]^=1;CHECK(crypto_ed25519_check(signature,key,message,length)==-1);key[byte]^=1;
        }
        for(size_t byte=0;byte<length;byte++){
            message[byte]^=1;CHECK(crypto_ed25519_check(signature,key,message,length)==-1);message[byte]^=1;
        }
        if(length)CHECK(crypto_ed25519_check(signature,key,message,length-1)==-1);
        CHECK(crypto_ed25519_check(signature,key,message,length+1)==-1);
        /* Adding the group order yields the same scalar modulo L but must
         * fail the canonical S<L check, rather than authenticate a variant. */
        unsigned carry=0;
        for(unsigned i=0;i<32;i++){unsigned sum=signature[32+i]+order[i]+carry;signature[32+i]=(uint8_t)sum;carry=sum>>8;}
        CHECK(!carry);CHECK(crypto_ed25519_check(signature,key,message,length)==-1);
        memset(signature+32,0xff,32);CHECK(crypto_ed25519_check(signature,key,message,length)==-1);
    }
    printf("PASS: %u Ed25519 checks; unmodified Monocypher4.0.3, RFC8032 KAT1–3, signature/key/message tamper, message length, S+L; no signer or production trust key\n",checks);
    return 0;
}
