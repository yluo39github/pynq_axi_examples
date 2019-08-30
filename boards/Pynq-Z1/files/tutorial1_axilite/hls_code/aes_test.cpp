#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ap_int.h>

// Enable ECB, CTR and CBC mode. Note this can be done before including aes.h or at compile-time.
// E.g. with GCC by using the -D flag: gcc -c aes.c -DCBC=0 -DCTR=1 -DECB=1
#define CBC 1
#define CTR 1
#define ECB 1

#include "aes.h"


static int test_encrypt_ecb(void);
void aes(ap_uint<128> key, uint8_t *input, uint8_t *output);
typedef uint8_t state_t[4][4];

int main(void)
{
    int exit;

    printf("\nTesting AES128\n\n");

    exit = test_encrypt_ecb();

    return exit;
}


static int test_encrypt_ecb(void)
{
//    uint8_t key[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//    uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };

//    uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
//	uint8_t key[] = {0x16, 0x15, 0x7e, 0x2b,
//					 0xa6, 0xd2, 0xae, 0x28,
//					 0x88, 0x15, 0xf7, 0xab,
//					 0x3c, 0x4f, 0xcf, 0x09};
//   uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
   uint8_t out[] = { 0xb4, 0x7b, 0xd7, 0x3a,
    				 0x60, 0x36, 0x7a, 0x0d,
					 0xf3, 0xca, 0x9e, 0xa8,
					 0x97, 0xef, 0x66, 0x24 };

//    uint8_t out[16] = { 0xd4, 0x4b, 0xe9, 0x66, 0x3b, 0x2c, 0x8a, 0xef, 0x59, 0xfa, 0x4c, 0x88, 0x2e, 0x2b, 0x34, 0xca}; // for 0s at inputs

//    uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t in[] = { 0xe2, 0xbe, 0xc1, 0x6b,
					 0x96, 0x9f, 0x40, 0x2e,
					 0x11, 0x7e, 0x3d, 0xe9,
					 0x2a, 0x17, 0x93, 0x73 };

 //   uint8_t in[16]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	ap_uint<128> key = "0x09cf4f3cabf7158828aed2a62b7e1516";
	uint8_t output[16];
    /*    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, in);*/
	aes(key, in, output);

    printf("ECB encrypt: ");

    if (0 == memcmp((char*) out, (char*) output, 16)) {
        printf("SUCCESS!\n");
	return(0);
    } else {
        printf("FAILURE!\n");
        int i;
        printf("Result:");
        for(i=0; i<16; i++){
        	printf("%x", output[i]);
        }
        printf("\n");
	return(1);
    }
}



