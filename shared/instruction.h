#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdio.h>

#include "include/xed-iclass-enum.h"
#include "address.h"

#if defined __linux__
#include "multiColumn.h"
#elif defined WIN32
#include "../misc/multiColumn.h"
#endif

#define INSTRUCTION_MAX_NB_DATA 5 /* Do not update this value without taking care of the tracer */

/* Data type value:
 * - bit 1: set to 0 -> INVALID 	set to 1 -> VALID
 * - bit 2: set to 0 -> MEM 		set to 1 -> REG
 * - bit 3: set to 0 -> READ 		set to 1 -> WRITE
 */

#define INSTRUCTION_DATA_TYPE_SET_INVALID(type) (type) &= 0xfffffffe
#define INSTRUCTION_DATA_TYPE_SET_VALID(type) 	(type) |= 0x00000001
#define INSTRUCTION_DATA_TYPE_SET_MEM(type)		(type) &= 0xfffffffd
#define INSTRUCTION_DATA_TYPE_SET_REG(type)		(type) |= 0x00000002
#define INSTRUCTION_DATA_TYPE_SET_READ(type)	(type) &= 0xfffffffb
#define INSTRUCTION_DATA_TYPE_SET_WRITE(type)	(type) |= 0x00000004

#define INSTRUCTION_DATA_TYPE_IS_INVALID(type) 	(((type) & 0x00000001) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_VALID(type) 	(((type) & 0x00000001) == 0x00000001)
#define INSTRUCTION_DATA_TYPE_IS_MEM(type) 		(((type) & 0x00000002) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_REG(type)		(((type) & 0x00000002) == 0x00000002)
#define INSTRUCTION_DATA_TYPE_IS_READ(type)		(((type) & 0x00000004) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_WRITE(type) 	(((type) & 0x00000004) == 0x00000004)

enum insDataType{
	INSDATA_INVALID 	= 0x00000000,
	INSDATA_MEM_READ	= 0x00000001,
	INSDATA_REG_READ 	= 0x00000003,
	INSDATA_MEM_WRITE	= 0x00000005,
	INSDATA_REG_WRITE	= 0x00000007
};

#define NB_REGISTER 19 /* do not forget to update this value */

enum reg{
	REGISTER_INVALID,
	REGISTER_EAX,
	REGISTER_AX,
	REGISTER_AH,
	REGISTER_AL,
	REGISTER_EBX,
	REGISTER_BX,
	REGISTER_BH,
	REGISTER_BL,
	REGISTER_ECX,
	REGISTER_CX,
	REGISTER_CH,
	REGISTER_CL,
	REGISTER_EDX,
	REGISTER_DX,
	REGISTER_DH,
	REGISTER_DL,
	REGISTER_ESI,
	REGISTER_EDI,
	REGISTER_EBP
};

struct insData{
	enum insDataType 	type;
	union {
		ADDRESS 		address;
		enum reg 		reg;
	}					location;
	uint32_t 			value;
	uint8_t 			size;
};

struct instruction{
	ADDRESS 		pc;
	uint32_t 		opcode;
	struct insData 	data[INSTRUCTION_MAX_NB_DATA];
};

struct multiColumnPrinter* instruction_init_multiColumnPrinter();
void instruction_print(struct multiColumnPrinter* printer, struct instruction *ins);

int32_t instruction_compare_pc(struct instruction* ins1, struct instruction* ins2);

void instruction_flush_tracer_buffer(FILE* file, struct instruction* buffer, uint32_t nb_instruction);

const char* instruction_opcode_2_string(uint32_t opcode);
const char* reg_2_string(enum reg reg);


#endif