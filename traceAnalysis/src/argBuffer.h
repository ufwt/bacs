#ifndef ARGBUFFER_H
#define ARGBUFFER_H

#include <stdint.h>

#include "array.h"
#include "address.h"

#define ARGBUFFER_FRAGMENT_MAX_NB_ELEMENT 	9
#define ARGBUFFER_ACCESS_SIZE_UNDEFINED 	-1

enum argLocationType{
	ARG_LOCATION_MEMORY,
	ARG_LOCATION_REGISTER
};

/* Explanation about the reg attribute in the location union:
 * 		bits [0 :3 ]: nb register combined (max is 15)
 * 		bits [4 :8 ]: register 1  compressed name
 * 		bits [9 :13]: register 2  compressed name
 * 		bits [14:18]: register 3  compressed name
 * 		bits [19:23]: register 4  compressed name
 * 		bits [24:28]: register 5  compressed name
 * 		bits [29:33]: register 6  compressed name
 * 		bits [34:38]: register 7  compressed name
 * 		bits [39:43]: register 8  compressed name
 * 		bits [44:48]: register 9  compressed name
 * 		bits [49:53]: register 10 compressed name
 * 		bits [54:58]: register 11 compressed name
 * 		bits [59:63]: register 12 compressed name
 */

#define ARGBUFFER_GET_NB_REG(reg)			((uint32_t)(reg) & 0x0000000f)
#define ARGBUFFER_SET_NB_REG(reg, val)		(reg) = (((reg) & (uint64_t)0xfffffffffffffff0ULL) | (uint64_t)((val) & 0x0000000f))
#define ARGBUFFER_GET_REG_NAME(reg, i) 		((uint32_t)((reg) >> ((i) * 5 + 4)) & 0x0000001f)
#define ARGBUFFER_SET_REG_NAME(reg, i, val) (reg) = (((reg) & (((uint64_t)0xffffffffffffffe0ULL << ((i) * 5 + 4)) | ((uint64_t)0xffffffffffffffffULL >> (64 - ((i) * 5 + 4))))) | ((uint64_t)((val) & 0x0000001f) << ((i) * 5 + 4)))

struct argBuffer{
	enum argLocationType 	location_type;
	union {
		ADDRESS 			address;
		uint64_t 			reg;
	} 						location;
	uint32_t 				size;
	int8_t					access_size;
	char* 					data;
};

void argBuffer_print_raw(struct argBuffer* arg);
int32_t argBuffer_clone(struct argBuffer* arg_src, struct argBuffer* arg_dst);
int32_t argBuffer_equal(struct argBuffer* arg1, struct argBuffer* arg2);
int32_t argBuffer_search(struct argBuffer* arg, char* buffer, uint32_t buffer_size);
struct argBuffer* argBuffer_compare(struct argBuffer* arg1, struct argBuffer* arg2);
int32_t argBuffer_try_merge(struct argBuffer* arg1, struct argBuffer* arg2);
void argBuffer_create_fragment_table(struct argBuffer* arg, uint32_t** table_, uint32_t* nb_element_); /* à l'occasion on peut supprimer c'est lourd pour pas grand chose */
void argBuffer_delete(struct argBuffer* arg);

int32_t argBuffer_clone_array(struct array* array_src, struct array* array_dst);
void argBuffer_delete_array(struct array* arg_array);

#endif