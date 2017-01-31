#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TEA.h"
#include "mode.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

int main(void){
	char 			plaintext[] = "Hi I am an XTEA ECB test vector distributed on 8 64-bit blocks!";
	unsigned char 	key[TEA_KEY_NB_BYTE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("Key:            ");
	fprintBuffer_raw(stdout, (char*)key, TEA_KEY_NB_BYTE);

	mode_enc_ecb((blockCipher)xtea_encrypt, TEA_BLOCK_NB_BYTE, (uint8_t*)plaintext, (uint8_t*)ciphertext, sizeof(plaintext), (void*)key);

	printf("\nCiphertext ECB: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

 	mode_dec_ecb((blockCipher)xtea_decrypt, TEA_BLOCK_NB_BYTE, (uint8_t*)ciphertext, (uint8_t*)deciphertext, sizeof(plaintext), (void*)key);

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
