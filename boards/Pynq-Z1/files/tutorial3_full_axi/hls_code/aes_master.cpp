/*
This is an implementation of the AES algorithm, specifically ECB, CTR and CBC mode.
Block size can be chosen in aes.h - available choices are AES128, AES192, AES256.
The implementation is verified against the test vectors in:
  National Institute of Standards and Technology Special Publication 800-38A 2001 ED
ECB-AES128
----------
  plain-text:
    6bc1bee22e409f96e93d7e117393172a
    ae2d8a571e03ac9c9eb76fac45af8e51
    30c81c46a35ce411e5fbc1191a0a52ef
    f69f2445df4f9b17ad2b417be66c3710
  key:
    2b7e151628aed2a6abf7158809cf4f3c
  resulting cipher
    3ad77bb40d7a3660a89ecaf32466ef97 
    f5d3d58503b9699de785895a96fdbaaf 
    43b1cd7f598ece23881b00e3ed030688 
    7b0c785e27e8ad3f8223207104725dd4 
NOTE:   String length must be evenly divisible by 16byte (str_len % 16 == 0)
        You should pad the end of the string with zeros if this is not the case.
        For AES192/256 the key size is proportionally larger.
*/

// Final version - ii = 1

/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/
#include <stdint.h>
#include <string.h>
#include "aes.h"
#include <ap_int.h>

#include <iostream>
using namespace std;

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4
#define Nk 4        // The number of 32 bit words in a key.
#define Nr 10       // The number of rounds in AES Cipher.
#define KEY_SIZE 16

#define NBR_BLOCKS 4

#define xtime(x) ((x<<1) ^ (((x>>7) & 1) * 0x1b))

/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/
// state - array holding the intermediate results during decryption.
typedef uint8_t state_t[4][4];


// The lookup-tables are marked const so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM - 
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
static const uint8_t sbox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };


static const ap_uint<128> mask[16] = {
	"0xff000000000000000000000000000000",
	"0x00ff0000000000000000000000000000",
	"0x0000ff00000000000000000000000000",
	"0x000000ff000000000000000000000000",
	"0x00000000ff0000000000000000000000",
	"0x0000000000ff00000000000000000000",
	"0x000000000000ff000000000000000000",
	"0x00000000000000ff0000000000000000",
	"0x0000000000000000ff00000000000000",
	"0x000000000000000000ff000000000000",
	"0x00000000000000000000ff0000000000",
	"0x0000000000000000000000ff00000000",
	"0x000000000000000000000000ff000000",
	"0x00000000000000000000000000ff0000",
	"0x0000000000000000000000000000ff00",
	"0x000000000000000000000000000000ff",
};



// The round constant word array, Rcon[i], contains the values given by 
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
static const uint8_t Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };


/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/

// This function produces Nb(Nr+1) round keys. The round keys are used in each round to decrypt the states. 
static void KeyExpansion(uint8_t RoundKey[AES_keyExpSize], uint8_t Key[KEY_SIZE])
{
#pragma HLS PIPELINE
  unsigned i, j, k;
  uint8_t tempa[4]; // Used for the column/row operations
  
  // The first round key is the key itself.
get_first_roundkey:
  for (i = 0; i < 4; ++i)
  {
    RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
    RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
    RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
    RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
  }

  // All other round keys are found from the previous round keys.
RoundKey_main_loop:
  for (i = Nk; i < 44; ++i)
  {
      k = (i - 1) * 4;
      tempa[0]=RoundKey[k + 0];
      tempa[1]=RoundKey[k + 1];
      tempa[2]=RoundKey[k + 2];
      tempa[3]=RoundKey[k + 3];


    if (i % Nk == 0)
    {
      // This function shifts the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

      // Function RotWord()
        const uint8_t u8tmp = tempa[0];
        tempa[0] = tempa[1];
        tempa[1] = tempa[2];
        tempa[2] = tempa[3];
        tempa[3] = u8tmp;

      // SubWord() is a function that takes a four-byte input word and 
      // applies the S-box to each of the four bytes to produce an output word.

      // Function Subword()
        tempa[0] = sbox[(tempa[0])];
        tempa[1] = sbox[(tempa[1])];
        tempa[2] = sbox[(tempa[2])];
        tempa[3] = sbox[(tempa[3])];

      tempa[0] = tempa[0] ^ Rcon[i/Nk];
    }

    j = i * 4; k=(i - Nk) * 4;
    RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
    RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
    RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
    RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
  }
}




// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static void AddRoundKey(uint8_t round, state_t state, uint8_t RoundKey[AES_keyExpSize])
{
  uint8_t i,j;
AddRoundKey_outer_loop:
  for (i = 0; i < 4; ++i)
  {
AddRoundKey_inner_loop:
    for (j = 0; j < 4; ++j)
    {
      state[i][j] ^= RoundKey[(round * Nb * 4) + (i * Nb) + j];
    }
  }
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void SubBytes(state_t state)
{
  uint8_t i, j;
Subbytes_outer_loop:
for (i = 0; i < 4; ++i){
Subbytes_inner_loop:
    for (j = 0; j < 4; ++j)
    {
      state[j][i] = sbox[(state[j][i])];
    }
  }
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void ShiftRows(state_t state)
{
  uint8_t temp;

  // Rotate first row 1 columns to left  
  temp           = state[0][1];
  state[0][1] = state[1][1];
  state[1][1] = state[2][1];
  state[2][1] = state[3][1];
  state[3][1] = temp;

  // Rotate second row 2 columns to left  
  temp           = state[0][2];
  state[0][2] = state[2][2];
  state[2][2] = temp;

  temp           = state[1][2];
  state[1][2] = state[3][2];
  state[3][2] = temp;

  // Rotate third row 3 columns to left
  temp           = state[0][3];
  state[0][3] = state[3][3];
  state[3][3] = state[2][3];
  state[2][3] = state[1][3];
  state[1][3] = temp;
}


// MixColumns function mixes the columns of the state matrix
static void MixColumns(state_t state)
{
  uint8_t i;
  uint8_t Tmp, Tm, t;
MixColumns_loop:
  for (i = 0; i < 4; ++i)
  {  
    t   = state[i][0];
    Tmp = state[i][0] ^ state[i][1] ^ state[i][2] ^ state[i][3] ;
    Tm  = state[i][0] ^ state[i][1] ; Tm = xtime(Tm);  state[i][0] ^= Tm ^ Tmp ;
    Tm  = state[i][1] ^ state[i][2] ; Tm = xtime(Tm);  state[i][1] ^= Tm ^ Tmp ;
    Tm  = state[i][2] ^ state[i][3] ; Tm = xtime(Tm);  state[i][2] ^= Tm ^ Tmp ;
    Tm  = state[i][3] ^ t ;              Tm = xtime(Tm);  state[i][3] ^= Tm ^ Tmp ;
  }
}


// Cipher is the main function that encrypts the PlainText.
static void Cipher(state_t state, uint8_t RoundKey[AES_keyExpSize])
{
  uint8_t round = 0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(0, state, RoundKey); 
  
  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
Cipher_Round:
  for (round = 1; round < 10; ++round)
  {
    SubBytes(state);
    ShiftRows(state);
    MixColumns(state);
    AddRoundKey(round, state, RoundKey);
  }
  
  // The last round is given below.
  // The MixColumns function is not here in the last round.
  SubBytes(state);
  ShiftRows(state);
  AddRoundKey(Nr, state, RoundKey);
}

// break up the 128 bit input into 16 bytes
// reorder the bytes to account for how data gets arranged into the 128bit value
void read_data(ap_uint<128> input, state_t in_buf)
{
   int r, c;
   uint8_t temp[16];
   	uint8_t i;
   	for (i=0; i<16; i++){
   		temp[i] = (input & mask[i]) >> ((15-i)*8);
   	}

RD_Loop_Row:
   for (r = 0; r < 4; r++) {
RD_Loop_Col:
      for (c = 0; c < 4; c++){
         in_buf[3-r][c] = temp[r*4+c];
      }
   }
}

// break up the 128 bit key into 16 bytes
// reorder the bytes to account for how data gets arranged into the 128bit value
void read_key(ap_uint<128> key_in, uint8_t key_out[16]){
#pragma HLS PIPELINE

	uint8_t temp[16];
	uint8_t i;
	for (i=0; i<16; i++){
		temp[i] = (key_in & mask[i]) >> ((15-i)*8);
	}
	ap_uint<3> w,b;
RD_Loop_Word:
   for (w = 0; w < 4; w++) {
RD_Loop_Byte:
	  for (b = 0; b < 4; b++){
		  key_out[(3-w)*4 + b] = temp[4*w+b]; 	// reorder the words, bytes within each word rermain unchanged
	  }
   }
}

void write_data(state_t buf, ap_uint<128> &output)
{
   int r, c;
   uint8_t temp[16];
WR_Loop_Row:
   for (r = 0; r < 4; r++) {
WR_Loop_Col:
      for (c = 0; c < 4; c++){
         temp[r * 4 + 3-c] = buf[r][c];
      }
   }
   output = *(ap_uint<128> *)temp;
}


// Top level function 
void aes(ap_uint<128> key, ap_uint<128> input[NBR_BLOCKS], ap_uint<128> output[NBR_BLOCKS], int size)
 {
#pragma HLS DATAFLOW
#pragma HLS INTERFACE s_axilite port=size
#pragma HLS INTERFACE m_axi depth=4 port=output offset=slave
#pragma HLS INTERFACE m_axi depth=4 port=input offset=slave
#pragma HLS INTERFACE s_axilite port=key
#pragma HLS INTERFACE ap_ctrl_hs port=return

	uint8_t key_new[16];
	state_t state;
	uint8_t RoundKey[AES_keyExpSize];
#pragma HLS ARRAY_PARTITION variable=RoundKey complete dim=1
	ap_uint<128> mem_buff_in[NBR_BLOCKS], mem_buff_out[NBR_BLOCKS];

	memcpy(mem_buff_in,(const ap_uint<128>*)input,size);	// copy data from memory

	read_key(key, key_new);						// process key
	KeyExpansion(RoundKey, key_new);

	int i;
	aes_label0:for(i = 0; i<size/16; i++){
#pragma HLS PIPELINE
		// encrypt data by block of 16 bytes
		read_data(mem_buff_in[i], state);
		Cipher(state, RoundKey);
		write_data(state, mem_buff_out[i]);
	}

	memcpy(output,mem_buff_out,size);					// copy data into memory
}





