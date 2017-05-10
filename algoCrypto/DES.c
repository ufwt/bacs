#include <string.h>

#include "DES.h"

#ifdef __GNUC__
# 	define DES_LOAD_DWORD(w) 	__builtin_bswap32(w)
# 	define DES_STORE_DWORD(w) 	__builtin_bswap32(w)
#else
# 	define DES_LOAD_DWORD(w) 	(((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
# 	define DES_STORE_DWORD(w) 	(((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
#endif


static const uint8_t perm_key[16][48] = {
	{
		 9, 50, 33, 59, 48, 16, 32, 56,  1,  8, 18, 41,  2, 34, 25, 24,
		43, 57, 58,  0, 35, 26, 17, 40, 21, 27, 38, 53, 36,  3, 46, 29,
		 4, 52, 22, 28, 60, 20, 37, 62, 14, 19, 44, 13, 12, 61, 54, 30
	},
	{
		 1, 42, 25, 51, 40,  8, 24, 48, 58,  0, 10, 33, 59, 26, 17, 16,
		35, 49, 50, 57, 56, 18,  9, 32, 13, 19, 30, 45, 28, 62, 38, 21,
		27, 44, 14, 20, 52, 12, 29, 54,  6, 11, 36,  5,  4, 53, 46, 22
	},
	{
		50, 26,  9, 35, 24, 57,  8, 32, 42, 49, 59, 17, 43, 10,  1,  0,
		48, 33, 34, 41, 40,  2, 58, 16, 60,  3, 14, 29, 12, 46, 22,  5,
		11, 28, 61,  4, 36, 27, 13, 38, 53, 62, 20, 52, 19, 37, 30,  6
	},
	{
		34, 10, 58, 48,  8, 41, 57, 16, 26, 33, 43,  1, 56, 59, 50, 49,
		32, 17, 18, 25, 24, 51, 42,  0, 44, 54, 61, 13, 27, 30,  6, 52,
		62, 12, 45, 19, 20, 11, 60, 22, 37, 46,  4, 36,  3, 21, 14, 53
	},
	{
		18, 59, 42, 32, 57, 25, 41,  0, 10, 17, 56, 50, 40, 43, 34, 33,
		16,  1,  2,  9,  8, 35, 26, 49, 28, 38, 45, 60, 11, 14, 53, 36,
		46, 27, 29,  3,  4, 62, 44,  6, 21, 30, 19, 20, 54,  5, 61, 37
	},
	{
		 2, 43, 26, 16, 41,  9, 25, 49, 59,  1, 40, 34, 24, 56, 18, 17,
		 0, 50, 51, 58, 57, 48, 10, 33, 12, 22, 29, 44, 62, 61, 37, 20,
		30, 11, 13, 54, 19, 46, 28, 53,  5, 14,  3,  4, 38, 52, 45, 21
	},
	{
		51, 56, 10,  0, 25, 58,  9, 33, 43, 50, 24, 18,  8, 40,  2,  1,
		49, 34, 35, 42, 41, 32, 59, 17, 27,  6, 13, 28, 46, 45, 21,  4,
		14, 62, 60, 38,  3, 30, 12, 37, 52, 61, 54, 19, 22, 36, 29,  5
	},
	{
		35, 40, 59, 49,  9, 42, 58, 17, 56, 34,  8,  2, 57, 24, 51, 50,
		33, 18, 48, 26, 25, 16, 43,  1, 11, 53, 60, 12, 30, 29,  5, 19,
		61, 46, 44, 22, 54, 14, 27, 21, 36, 45, 38,  3,  6, 20, 13, 52
	},
	{
		56, 32, 51, 41,  1, 34, 50,  9, 48, 26,  0, 59, 49, 16, 43, 42,
		25, 10, 40, 18, 17,  8, 35, 58,  3, 45, 52,  4, 22, 21, 60, 11,
		53, 38, 36, 14, 46,  6, 19, 13, 28, 37, 30, 62, 61, 12,  5, 44
	},
	{
		40, 16, 35, 25, 50, 18, 34, 58, 32, 10, 49, 43, 33,  0, 56, 26,
		 9, 59, 24,  2,  1, 57, 48, 42, 54, 29, 36, 19,  6,  5, 44, 62,
		37, 22, 20, 61, 30, 53,  3, 60, 12, 21, 14, 46, 45, 27, 52, 28
	},
	{
		24,  0, 48,  9, 34,  2, 18, 42, 16, 59, 33, 56, 17, 49, 40, 10,
		58, 43,  8, 51, 50, 41, 32, 26, 38, 13, 20,  3, 53, 52, 28, 46,
		21,  6,  4, 45, 14, 37, 54, 44, 27,  5, 61, 30, 29, 11, 36, 12
	},
	{
		 8, 49, 32, 58, 18, 51,  2, 26,  0, 43, 17, 40,  1, 33, 24, 59,
		42, 56, 57, 35, 34, 25, 16, 10, 22, 60,  4, 54, 37, 36, 12, 30,
		 5, 53, 19, 29, 61, 21, 38, 28, 11, 52, 45, 14, 13, 62, 20, 27
	},
	{
		57, 33, 16, 42,  2, 35, 51, 10, 49, 56,  1, 24, 50, 17,  8, 43,
		26, 40, 41, 48, 18,  9,  0, 59,  6, 44, 19, 38, 21, 20, 27, 14,
		52, 37,  3, 13, 45,  5, 22, 12, 62, 36, 29, 61, 60, 46,  4, 11
	},
	{
		41, 17,  0, 26, 51, 48, 35, 59, 33, 40, 50,  8, 34,  1, 57, 56,
		10, 24, 25, 32,  2, 58, 49, 43, 53, 28,  3, 22,  5,  4, 11, 61,
		36, 21, 54, 60, 29, 52,  6, 27, 46, 20, 13, 45, 44, 30, 19, 62
	},
	{
		25,  1, 49, 10, 35, 32, 48, 43, 17, 24, 34, 57, 18, 50, 41, 40,
		59,  8,  9, 16, 51, 42, 33, 56, 37, 12, 54,  6, 52, 19, 62, 45,
		20,  5, 38, 44, 13, 36, 53, 11, 30,  4, 60, 29, 28, 14,  3, 46
	},
	{
		17, 58, 41,  2, 56, 24, 40, 35,  9, 16, 26, 49, 10, 42, 33, 32,
		51,  0,  1,  8, 43, 34, 25, 48, 29,  4, 46, 61, 44, 11, 54, 37,
		12, 60, 30, 36,  5, 28, 45,  3, 22, 27, 52, 21, 20,  6, 62, 38
	}
};

static const uint32_t round_key_byte_order[8] = {3, 7, 2, 6, 1, 5, 0, 4};

static const uint32_t spBox[8][64] = {
	{
		0x01010400,0x00000000,0x00010000,0x01010404,0x01010004,0x00010404,0x00000004,0x00010000,
		0x00000400,0x01010400,0x01010404,0x00000400,0x01000404,0x01010004,0x01000000,0x00000004,
		0x00000404,0x01000400,0x01000400,0x00010400,0x00010400,0x01010000,0x01010000,0x01000404,
		0x00010004,0x01000004,0x01000004,0x00010004,0x00000000,0x00000404,0x00010404,0x01000000,
		0x00010000,0x01010404,0x00000004,0x01010000,0x01010400,0x01000000,0x01000000,0x00000400,
		0x01010004,0x00010000,0x00010400,0x01000004,0x00000400,0x00000004,0x01000404,0x00010404,
		0x01010404,0x00010004,0x01010000,0x01000404,0x01000004,0x00000404,0x00010404,0x01010400,
		0x00000404,0x01000400,0x01000400,0x00000000,0x00010004,0x00010400,0x00000000,0x01010004
	},
	{
		0x80108020,0x80008000,0x00008000,0x00108020,0x00100000,0x00000020,0x80100020,0x80008020,
		0x80000020,0x80108020,0x80108000,0x80000000,0x80008000,0x00100000,0x00000020,0x80100020,
		0x00108000,0x00100020,0x80008020,0x00000000,0x80000000,0x00008000,0x00108020,0x80100000,
		0x00100020,0x80000020,0x00000000,0x00108000,0x00008020,0x80108000,0x80100000,0x00008020,
		0x00000000,0x00108020,0x80100020,0x00100000,0x80008020,0x80100000,0x80108000,0x00008000,
		0x80100000,0x80008000,0x00000020,0x80108020,0x00108020,0x00000020,0x00008000,0x80000000,
		0x00008020,0x80108000,0x00100000,0x80000020,0x00100020,0x80008020,0x80000020,0x00100020,
		0x00108000,0x00000000,0x80008000,0x00008020,0x80000000,0x80100020,0x80108020,0x00108000
	},
	{
		0x00000208,0x08020200,0x00000000,0x08020008,0x08000200,0x00000000,0x00020208,0x08000200,
		0x00020008,0x08000008,0x08000008,0x00020000,0x08020208,0x00020008,0x08020000,0x00000208,
		0x08000000,0x00000008,0x08020200,0x00000200,0x00020200,0x08020000,0x08020008,0x00020208,
		0x08000208,0x00020200,0x00020000,0x08000208,0x00000008,0x08020208,0x00000200,0x08000000,
		0x08020200,0x08000000,0x00020008,0x00000208,0x00020000,0x08020200,0x08000200,0x00000000,
		0x00000200,0x00020008,0x08020208,0x08000200,0x08000008,0x00000200,0x00000000,0x08020008,
		0x08000208,0x00020000,0x08000000,0x08020208,0x00000008,0x00020208,0x00020200,0x08000008,
		0x08020000,0x08000208,0x00000208,0x08020000,0x00020208,0x00000008,0x08020008,0x00020200
	},
	{
		0x00802001,0x00002081,0x00002081,0x00000080,0x00802080,0x00800081,0x00800001,0x00002001,
		0x00000000,0x00802000,0x00802000,0x00802081,0x00000081,0x00000000,0x00800080,0x00800001,
		0x00000001,0x00002000,0x00800000,0x00802001,0x00000080,0x00800000,0x00002001,0x00002080,
		0x00800081,0x00000001,0x00002080,0x00800080,0x00002000,0x00802080,0x00802081,0x00000081,
		0x00800080,0x00800001,0x00802000,0x00802081,0x00000081,0x00000000,0x00000000,0x00802000,
		0x00002080,0x00800080,0x00800081,0x00000001,0x00802001,0x00002081,0x00002081,0x00000080,
		0x00802081,0x00000081,0x00000001,0x00002000,0x00800001,0x00002001,0x00802080,0x00800081,
		0x00002001,0x00002080,0x00800000,0x00802001,0x00000080,0x00800000,0x00002000,0x00802080
	},
	{
		0x00000100,0x02080100,0x02080000,0x42000100,0x00080000,0x00000100,0x40000000,0x02080000,
		0x40080100,0x00080000,0x02000100,0x40080100,0x42000100,0x42080000,0x00080100,0x40000000,
		0x02000000,0x40080000,0x40080000,0x00000000,0x40000100,0x42080100,0x42080100,0x02000100,
		0x42080000,0x40000100,0x00000000,0x42000000,0x02080100,0x02000000,0x42000000,0x00080100,
		0x00080000,0x42000100,0x00000100,0x02000000,0x40000000,0x02080000,0x42000100,0x40080100,
		0x02000100,0x40000000,0x42080000,0x02080100,0x40080100,0x00000100,0x02000000,0x42080000,
		0x42080100,0x00080100,0x42000000,0x42080100,0x02080000,0x00000000,0x40080000,0x42000000,
		0x00080100,0x02000100,0x40000100,0x00080000,0x00000000,0x40080000,0x02080100,0x40000100
	},
	{
		0x20000010,0x20400000,0x00004000,0x20404010,0x20400000,0x00000010,0x20404010,0x00400000,
		0x20004000,0x00404010,0x00400000,0x20000010,0x00400010,0x20004000,0x20000000,0x00004010,
		0x00000000,0x00400010,0x20004010,0x00004000,0x00404000,0x20004010,0x00000010,0x20400010,
		0x20400010,0x00000000,0x00404010,0x20404000,0x00004010,0x00404000,0x20404000,0x20000000,
		0x20004000,0x00000010,0x20400010,0x00404000,0x20404010,0x00400000,0x00004010,0x20000010,
		0x00400000,0x20004000,0x20000000,0x00004010,0x20000010,0x20404010,0x00404000,0x20400000,
		0x00404010,0x20404000,0x00000000,0x20400010,0x00000010,0x00004000,0x20400000,0x00404010,
		0x00004000,0x00400010,0x20004010,0x00000000,0x20404000,0x20000000,0x00400010,0x20004010
	},
	{
		0x00200000,0x04200002,0x04000802,0x00000000,0x00000800,0x04000802,0x00200802,0x04200800,
		0x04200802,0x00200000,0x00000000,0x04000002,0x00000002,0x04000000,0x04200002,0x00000802,
		0x04000800,0x00200802,0x00200002,0x04000800,0x04000002,0x04200000,0x04200800,0x00200002,
		0x04200000,0x00000800,0x00000802,0x04200802,0x00200800,0x00000002,0x04000000,0x00200800,
		0x04000000,0x00200800,0x00200000,0x04000802,0x04000802,0x04200002,0x04200002,0x00000002,
		0x00200002,0x04000000,0x04000800,0x00200000,0x04200800,0x00000802,0x00200802,0x04200800,
		0x00000802,0x04000002,0x04200802,0x04200000,0x00200800,0x00000000,0x00000002,0x04200802,
		0x00000000,0x00200802,0x04200000,0x00000800,0x04000002,0x04000800,0x00000800,0x00200002
	},
	{
		0x10001040,0x00001000,0x00040000,0x10041040,0x10000000,0x10001040,0x00000040,0x10000000,
		0x00040040,0x10040000,0x10041040,0x00041000,0x10041000,0x00041040,0x00001000,0x00000040,
		0x10040000,0x10000040,0x10001000,0x00001040,0x00041000,0x00040040,0x10040040,0x10041000,
		0x00001040,0x00000000,0x00000000,0x10040040,0x10000040,0x10001000,0x00041040,0x00040000,
		0x00041040,0x00040000,0x10041000,0x00001000,0x00000040,0x10040040,0x00001000,0x00041040,
		0x10001000,0x00000040,0x10000040,0x10040000,0x10040040,0x10000000,0x00040000,0x10001040,
		0x00000000,0x10041040,0x00040040,0x10000040,0x10040000,0x10001000,0x10001040,0x00000000,
		0x10041040,0x00041000,0x00041000,0x00001040,0x00001040,0x00040040,0x10000000,0x10041000
	}
};

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define ROTATE_RIGHT(x, n) (((x) >> (n)) | ((x) << (32-(n))))

#define IPERM(x1, x2) 						\
	{ 										\
		uint32_t tmp_; 						\
											\
		x2 = ROTATE_LEFT(x2, 4); 			\
		tmp_ = (x1 ^ x2) & 0xf0f0f0f0; 		\
		x1 ^= tmp_; 						\
		x2 = ROTATE_RIGHT(x2 ^ tmp_, 20); 	\
		tmp_ = (x1 ^ x2) & 0xffff0000; 		\
		x1 ^= tmp_; 						\
		x2 = ROTATE_RIGHT(x2 ^ tmp_, 18); 	\
		tmp_ = (x1 ^ x2) & 0x33333333; 		\
		x1 ^= tmp_; 						\
		x2 = ROTATE_RIGHT(x2 ^ tmp_, 6); 	\
		tmp_ = (x1 ^ x2) & 0x00ff00ff; 		\
		x1 ^= tmp_; 						\
		x2 = ROTATE_LEFT(x2 ^ tmp_, 9); 	\
		tmp_ = (x1 ^ x2) & 0xaaaaaaaa; 		\
		x1 = ROTATE_LEFT(x1 ^ tmp_, 1); 	\
		x2 ^= tmp_; 						\
	}

#define FPERM(x1, x2) 						\
	{ 										\
		uint32_t tmp_; 						\
											\
		x2 = ROTATE_RIGHT(x2, 1); 			\
		tmp_ = (x1 ^ x2) & 0xaaaaaaaa; 		\
		x2 ^= tmp_; 						\
		x1 = ROTATE_RIGHT(x1 ^ tmp_, 9); 	\
		tmp_ = (x1 ^ x2) & 0x00ff00ff; 		\
		x2 ^= tmp_; 						\
		x1 = ROTATE_LEFT(x1 ^ tmp_, 6); 	\
		tmp_ = (x1 ^ x2) & 0x33333333; 		\
		x2 ^= tmp_; 						\
		x1 = ROTATE_LEFT(x1 ^ tmp_, 18); 	\
		tmp_ = (x1 ^ x2) & 0xffff0000; 		\
		x2 ^= tmp_; 						\
		x1 = ROTATE_LEFT(x1 ^ tmp_, 20); 	\
		tmp_ = (x1 ^ x2) & 0xf0f0f0f0; 		\
		x2 ^= tmp_; 						\
		x1 = ROTATE_RIGHT(x1 ^ tmp_, 4); 	\
	}

void des_key_expand(const uint8_t* key, uint8_t* round_key){
	uint32_t i;
	uint32_t j;

	memset(round_key, 0, DES_ROUND_KEY_NB_BYTE);

	for (i = 0; i < 16; i++, round_key += 8){
		for (j = 0; j < 48; j++){
			uint32_t l = perm_key[i][j];
			round_key[round_key_byte_order[j / 6]] |= ((key[l / 8] >> (7 - (l % 8))) & 0x01) << (5 - (j % 6));
		}
	}
}

void des_encrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output){
	uint32_t x1;
	uint32_t x2;
	uint32_t i;
	uint32_t tmp;

	x1 = DES_LOAD_DWORD(input[0]);
	x2 = DES_LOAD_DWORD(input[1]);

	IPERM(x1 ,x2);

	for (i = 0; i < 16; i++){
		tmp = ROTATE_RIGHT(x2, 4) ^ round_key[2 * i + 0];
		x1 ^= spBox[6][tmp & 0x3f] ^ spBox[4][(tmp >> 8) & 0x3f] ^ spBox[2][(tmp >> 16) & 0x3f] ^ spBox[0][(tmp >> 24) & 0x3f];
		tmp = x2 ^ round_key[2 * i + 1];
		x1 ^= spBox[7][tmp & 0x3f] ^ spBox[5][(tmp >> 8) & 0x3f] ^ spBox[3][(tmp >> 16) & 0x3f] ^ spBox[1][(tmp >> 24) & 0x3f];

		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	FPERM(x1, x2);

	output[0] = DES_STORE_DWORD(x2);
	output[1] = DES_STORE_DWORD(x1);
}

void des_decrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output){
	uint32_t x1;
	uint32_t x2;
	uint32_t i;
	uint32_t tmp;

	x1 = DES_LOAD_DWORD(input[0]);
	x2 = DES_LOAD_DWORD(input[1]);

	IPERM(x1 ,x2);

	for (i = 16; i > 0;){
		i --;
		tmp = ROTATE_RIGHT(x2, 4) ^ round_key[2 * i + 0];
		x1 ^= spBox[6][tmp & 0x3f] ^ spBox[4][(tmp >> 8) & 0x3f] ^ spBox[2][(tmp >> 16) & 0x3f] ^ spBox[0][(tmp >> 24) & 0x3f];
		tmp = x2 ^ round_key[2 * i + 1];
		x1 ^= spBox[7][tmp & 0x3f] ^ spBox[5][(tmp >> 8) & 0x3f] ^ spBox[3][(tmp >> 16) & 0x3f] ^ spBox[1][(tmp >> 24) & 0x3f];

		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	FPERM(x1, x2);

	output[0] = DES_STORE_DWORD(x2);
	output[1] = DES_STORE_DWORD(x1);
}
