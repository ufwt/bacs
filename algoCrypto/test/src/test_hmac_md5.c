#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "MD5.h"
#include "mode.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

int main(){
	char 			message[] = "Hello I am a test vector for the HMAC MD5. Since I am 92 bytes wide I lay on several blocks.";
	char 			key[] = "1 4m 4 53cr3t k3y";
	struct md5State md5_state;
	struct hash 	hash;
	char 			hash_mac[MD5_HASH_NB_BYTE];

	hash.block_size = MD5_BLOCK_NB_BYTE;
	hash.hash_size 	= MD5_HASH_NB_BYTE;
	hash.state 		= &md5_state;
	hash.func_init 	= (void(*)(void*))md5_init;
	hash.func_feed 	= (void(*)(void*,void*,uint64_t))md5_feed;
	hash.func_hash 	= (void(*)(void*,void*))md5_hash;

	if (hmac(&hash, (uint8_t*)message, (uint8_t*)hash_mac, strlen(message), (uint8_t*)key, strlen(key))){
		printf("ERROR: in %s, the HMAC function failed\n", __func__);
	}
	else{
		printf("Plaintext: \"%s\"\n", message);
		printf("Key:       \"%s\"\n", key);
		printf("MD5 HMAC:  ");
		printBuffer_raw(stdout, hash_mac, MD5_HASH_NB_BYTE);
		printf("\n");
	}

	return 0;
}