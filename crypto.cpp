#include "crypto.h"

#include <string.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <stdio.h>

void AES128_ECB_Enc(const uint8_t *key, uint32_t szKey,
                    const uint8_t *plain, uint32_t szPlain,
                    uint8_t *cipher, uint32_t szCipher) {

    AES_KEY encKey;
    AES_set_encrypt_key(key, 8*szKey, &encKey);

    uint32_t nBlocks = szPlain/16 + 1;
    uint32_t cipherLen = (nBlocks) * 16;
    if(szCipher < cipherLen) {
        return;
    }

    for(uint32_t i = 0 ; i < nBlocks - 1 ; i++) {
        AES_ecb_encrypt(&plain[i*16], &cipher[i*16], &encKey, AES_ENCRYPT);
    }

    uint32_t endBlockLen = 16 - (cipherLen - szPlain);
    uint8_t endBlock[16] = { 0 };
    memcpy(endBlock, &plain[16*(nBlocks-1)], endBlockLen);

    AES_ecb_encrypt(endBlock, &cipher[16*(nBlocks-1)], &encKey, AES_ENCRYPT);

}

char *base64(const uint8_t *plain, uint32_t length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, plain, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = new char[bptr->length];
    memcpy(buff, bptr->data, bptr->length-1);
    buff[bptr->length-1] = '\0';

    BIO_free_all(b64);

    return buff;
}
