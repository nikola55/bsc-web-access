#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>

void AES128_ECB_Enc(const uint8_t *key, uint32_t szKey, const uint8_t *plain, uint32_t szPlain, uint8_t *cipher, uint32_t szCipher);

char *base64(const uint8_t *plain, uint32_t length);

#endif // CRYPTO_H
