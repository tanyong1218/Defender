
#ifndef __OSSL_CRYPTO_SM4_H__
#define __OSSL_CRYPTO_SM4_H__

# define SM4_ENCRYPT     1 
# define SM4_DECRYPT     0 

 
# define SM4_BLOCK_SIZE    16 
# define SM4_KEY_SCHEDULE  32 

typedef struct SM4_KEY_st { 
    unsigned int rk[SM4_KEY_SCHEDULE]; 
} SM4_KEY; 


#ifdef __cplusplus
extern "C" {
#endif
  
int SM4_set_key(const unsigned char *key, SM4_KEY *ks); 
void SM4_encrypt(const unsigned char *in, unsigned char *out, const SM4_KEY *ks); 
void SM4_decrypt(const unsigned char *in, unsigned char *out, const SM4_KEY *ks); 

#ifdef __cplusplus
}
#endif
 
#endif // __OSSL_CRYPTO_SM4_H__
