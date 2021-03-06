#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irCheck.h"
#include "base.h"

uint32_t ir_check_size(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irOperation* 	operand_cursor;
	struct irDependence* 	dependence_cursor;
	uint32_t 				result 				= 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->size % 8 && operation_cursor->type != IR_OPERATION_TYPE_SYMBOL){
			log_err_m("incorrect size: %u is not a multiple of 8", operation_cursor->size);
			fputc('\t', stderr); irOperation_fprint(operation_cursor, stderr); fputc('\n', stderr);
			operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
			result = 1;
			continue;
		}

		switch(operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				if (operation_cursor->size != irRegister_get_size(operation_cursor->operation_type.in_reg.reg)){
					log_err_m("register %s has an incorrect size: %u", irRegister_2_string(operation_cursor->operation_type.in_reg.reg), operation_cursor->size);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence_cursor = ir_edge_get_dependence(edge_cursor);

					if (dependence_cursor->type == IR_DEPENDENCE_TYPE_ADDRESS && operand_cursor->size != ADDRESS_SIZE){
						log_err_m("memory load address is %u bits large", operand_cursor->size);
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence_cursor = ir_edge_get_dependence(edge_cursor);

					switch(dependence_cursor->type){
						case IR_DEPENDENCE_TYPE_ADDRESS : {
							if (operand_cursor->size != ADDRESS_SIZE){
								log_err_m("memory store address is %u bits large", operand_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						case IR_DEPENDENCE_TYPE_DIRECT 	: {
							if (operation_cursor->size != operand_cursor->size){
								log_err_m("incorrect operand size for memory store: (%u -> %u)", operand_cursor->size, operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						default 						: {
							break;
						}
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence_cursor = ir_edge_get_dependence(edge_cursor);

					if (dependence_cursor->type == IR_DEPENDENCE_TYPE_MACRO){
						continue;
					}

					switch(operation_cursor->operation_type.inst.opcode){
						case IR_DIVQ 		:
						case IR_DIVR 		:
						case IR_IDIVQ 		:
						case IR_IDIVR 		: {
							if (dependence_cursor->type == IR_DEPENDENCE_TYPE_DIVISOR){
								if (operation_cursor->size != operand_cursor->size){
									log_err_m("incorrect operand size for divide: %u", operand_cursor->size);
									operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
									result = 1;
								}
							}
							else{
								if (operation_cursor->size != operand_cursor->size / 2){
									log_err_m("found size mismatch (%u -> %s:%u)", operand_cursor->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
									operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
									result = 1;
								}
							}
							break;
						}
						case IR_MOVZX 		: {
							if (!valid_operand_size_ins_movzx(operation_cursor, operand_cursor)){
								log_err_m("found size mismatch (%u -> MOVZX:%u)", operand_cursor->size, operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						case IR_PART1_8 	: {
							if (!valid_operand_size_ins_partX_8(operation_cursor, operand_cursor)){
								log_err_m("found size mismatch (%u -> PART1_8:%u)", operand_cursor->size, operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						case IR_PART2_8 	: {
							if (!valid_operand_size_ins_partX_8(operation_cursor, operand_cursor)){
								log_err_m("found size mismatch (%u -> PART2_8:%u)", operand_cursor->size, operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						case IR_PART1_16 	: {
							if (!valid_operand_size_ins_partX_16(operation_cursor, operand_cursor)){
								log_err_m("found size mismatch (%u -> PART1_16:%u)", operand_cursor->size, operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						case IR_ROL 		:
						case IR_ROR 		:
						case IR_SHL 		:
						case IR_SHLD 		:
						case IR_SHR 		:
						case IR_SHRD 		: {
							if (dependence_cursor->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
								if (operand_cursor->size != 8 && operation_cursor->size != operand_cursor->size){
									log_err_m("incorrect operand size for displacement: %u", operand_cursor->size);
									operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
									result = 1;
								}
							}
							else{
								if (operation_cursor->size != operand_cursor->size){
									log_err_m("found size mismatch (%u -> %s:%u)", operand_cursor->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
									operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
									result = 1;
								}
							}
							break;
						}
						case IR_IMUL 	:
						case IR_MUL 	: {
							if (operand_cursor->size * 2 != operation_cursor->size && operand_cursor->size != operation_cursor->size){
								log_err_m("incorrect operand size for multiply: %u -> %u", operand_cursor->size, operation_cursor->size);
								putchar('\t'); irOperation_fprint(operation_cursor, stdout); putchar('\n');
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
						default 		: {
							if (operation_cursor->size != operand_cursor->size){
								log_err_m("found size mismatch (%u -> %s:%u)", operand_cursor->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
							break;
						}
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				break;
			}
			case IR_OPERATION_TYPE_NULL 	: {
				break;
			}
		}
	}

	return result;
}

static const uint32_t min_dst_edge[NB_IR_OPCODE] = {
	2, 	/* 0  IR_ADC 						*/
	2, 	/* 1  IR_ADD 						*/
	2, 	/* 2  IR_AND 						*/
	2, 	/* 3  IR_CMOV 						*/
	2, 	/* 4  IR_DIVQ 						*/
	2, 	/* 5  IR_DIVR 						*/
	2, 	/* 6  IR_IDIVQ 						*/
	2, 	/* 7  IR_IDIVR 						*/
	2, 	/* 8  IR_IMUL 						*/
	0, 	/* 9  IR_LEA 	It doesn't matter 	*/
	0, 	/* 10 IR_MOV 	It doesn't matter 	*/
	1, 	/* 11 IR_MOVZX 						*/
	2, 	/* 12 IR_MUL 						*/
	1, 	/* 13 IR_NEG 						*/
	1, 	/* 14 IR_NOT 						*/
	2, 	/* 15 IR_OR 						*/
	1, 	/* 16 IR_PART1_8 					*/
	1, 	/* 17 IR_PART2_8 					*/
	1, 	/* 18 IR_PART1_16 					*/
	2, 	/* 19 IR_ROL 						*/
	2, 	/* 20 IR_ROR 						*/
	2, 	/* 21 IR_SBB 						*/
	2, 	/* 22 IR_SHL 						*/
	3, 	/* 23 IR_SHLD 						*/
	2, 	/* 24 IR_SHR 						*/
	3, 	/* 25 IR_SHRD 						*/
	2, 	/* 26 IR_SUB 						*/
	2, 	/* 27 IR_XOR 						*/
	0, 	/* 28 IR_LOAD 	It doesn't matter 	*/
	0, 	/* 29 IR_STORE It doesn't matter 	*/
	0, 	/* 30 IR_JOKER It doesn't matter 	*/
	0, 	/* 31 IR_INVALID It doesn't matter 	*/
};

static const uint32_t max_dst_edge[NB_IR_OPCODE] = {
	0x00000002, /* 0  IR_ADc 						*/
	0xffffffff, /* 1  IR_ADD 						*/
	0xffffffff, /* 2  IR_AND 						*/
	0x00000002, /* 3  IR_CMOV 						*/
	0x00000003, /* 4  IR_DIVQ 						*/
	0x00000003, /* 5  IR_DIVR 						*/
	0x00000002, /* 6  IR_IDIVQ 						*/
	0x00000002, /* 7  IR_IDIVR 						*/
	0xffffffff, /* 8  IR_IMUL 						*/
	0x00000000, /* 9  IR_LEA 	It doesn't matter 	*/
	0x00000000, /* 10 IR_MOV 	It doesn't matter 	*/
	0x00000001, /* 11 IR_MOVZX 						*/
	0xffffffff, /* 12 IR_MUL 						*/
	0x00000001, /* 13 IR_NEG 						*/
	0x00000001, /* 14 IR_NOT 						*/
	0xffffffff, /* 15 IR_OR 						*/
	0x00000001, /* 16 IR_PART1_8 					*/
	0x00000001, /* 17 IR_PART2_8 					*/
	0x00000001, /* 18 IR_PART1_16 					*/
	0x00000002, /* 19 IR_ROL 						*/
	0x00000002, /* 20 IR_ROR 						*/
	0x00000002, /* 21 IR_SBB 						*/
	0x00000002, /* 22 IR_SHL 						*/
	0x00000003, /* 23 IR_SHLD 						*/
	0x00000002, /* 24 IR_SHR 						*/
	0x00000003, /* 25 IR_SHRD 						*/
	0x00000002, /* 26 IR_SUB 						*/
	0xffffffff, /* 27 IR_XOR 						*/
	0x00000000, /* 28 IR_LOAD 	It doesn't matter 	*/
	0x00000000, /* 29 IR_STORE It doesn't matter 	*/
	0x00000000, /* 30 IR_JOKER It doesn't matter 	*/
	0x00000000, /* 31 IR_INVALID It doesn't matter 	*/
};

uint32_t ir_check_connectivity(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irDependence* 	dependence;
	uint32_t 				nb_dependence[NB_DEPENDENCE_TYPE];
	uint32_t 				i;
	uint32_t 				nb_edge_dst;
	uint32_t 				result = 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		switch (operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst){
					log_err_m("input register %s has %u dst edge(s)", irRegister_2_string(operation_cursor->operation_type.in_reg.reg), node_cursor->nb_edge_dst);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src && !irOperation_is_final(operation_cursor)){
					log_err("input register has no src edge");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst == 1){
					dependence = ir_edge_get_dependence(node_get_head_edge_dst(node_cursor));

					if (dependence->type != IR_DEPENDENCE_TYPE_ADDRESS){
						log_err("memory load has an incorrect type of dependence");
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
					}
				}
				else{
					log_err_m("memory load has %u dst edge(s)", node_cursor->nb_edge_dst);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src && !irOperation_is_final(operation_cursor)){
					log_err("memory load has no src edge but neither is flagged as final");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst == 2){
					for (edge_cursor = node_get_head_edge_dst(node_cursor), memset(nb_dependence, 0, sizeof(uint32_t) * NB_DEPENDENCE_TYPE); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						dependence = ir_edge_get_dependence(edge_cursor);
						nb_dependence[dependence->type] ++;

						if (dependence->type != IR_DEPENDENCE_TYPE_ADDRESS && dependence->type != IR_DEPENDENCE_TYPE_DIRECT){
							log_err("memory store has an incorrect type of dependence");
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
					}
					if (nb_dependence[IR_DEPENDENCE_TYPE_ADDRESS] != 1){
						log_err_m("memory store has an incorrect number of address dependence (%u)", nb_dependence[IR_DEPENDENCE_TYPE_ADDRESS]);
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
					}
					if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
						log_err_m("memory store has an incorrect number of direct dependence (%u)", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
					}
				}
				else{
					log_err_m("memory store has %u dst edge(s)", node_cursor->nb_edge_dst);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				/* Check output edge(s) */
				if (node_cursor->nb_edge_src){
					log_err_m("memory store has %u src edge(s)", node_cursor->nb_edge_src);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst){
					log_err_m("immediate has %u dst edge(s)", node_cursor->nb_edge_dst);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src && !irOperation_is_final(operation_cursor)){
					log_err("immediate has no src edge but neither is flagged as final");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				for (edge_cursor = node_get_head_edge_dst(node_cursor), memset(nb_dependence, 0, sizeof(uint32_t) * NB_DEPENDENCE_TYPE), nb_edge_dst = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					dependence = ir_edge_get_dependence(edge_cursor);
					nb_dependence[dependence->type] ++;
					if (dependence->type != IR_DEPENDENCE_TYPE_MACRO){
						nb_edge_dst ++;
					}
				}

				if ((nb_edge_dst < min_dst_edge[operation_cursor->operation_type.inst.opcode] && nb_dependence[IR_DEPENDENCE_TYPE_MACRO] == 0) || nb_edge_dst > max_dst_edge[operation_cursor->operation_type.inst.opcode]){
					log_err_m("inst %s has an incorrect number of dst edge: %u (min=%u, max=%u)", irOpcode_2_string(operation_cursor->operation_type.inst.opcode), nb_edge_dst, min_dst_edge[operation_cursor->operation_type.inst.opcode], max_dst_edge[operation_cursor->operation_type.inst.opcode]);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				if (!node_cursor->nb_edge_src && !irOperation_is_final(operation_cursor)){
					log_err_m("inst %s has no src edge but neither is flagged as final", irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				switch(operation_cursor->operation_type.inst.opcode){
					case IR_ADC 		:
					case IR_ADD 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst ADD", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_AND 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst AND", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_CMOV 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst CMOV", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_DIVQ 		:
					case IR_DIVR 		:
					case IR_IDIVQ 		:
					case IR_IDIVR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIVISOR && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst %s", i, irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] < 1 || nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] > 2){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst %s", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT], irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIVISOR] != 1){
							log_err_m("incorrect number of dependence of type DIVISOR: %u for inst %s", nb_dependence[IR_DEPENDENCE_TYPE_DIVISOR], irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_IMUL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst IMUL", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_LEA 		: {
						break;
					}
					case IR_MOV 		: {
						break;
					}
					case IR_MOVZX 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst MOVZX", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_MUL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst MUL", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_NEG 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst NEG", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_NOT 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst NOT", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_OR 			: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst OR", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_PART1_8 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst PART1_8", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_PART2_8 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst PART2_8", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_PART1_16 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst PART1_16", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_ROL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst ROL", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst ROL", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst ROL", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_ROR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst ROR", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst ROR", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst ROR", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SBB 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SUBSTITUTE && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SBB", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SBB", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SUBSTITUTE] != 1){
							log_err_m("incorrect number of dependence of type SUBSTITUTE: %u for inst SBB", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SHL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SHL", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SHL", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst SHL", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SHLD 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_ROUND_OFF && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SHLD", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SHLD", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_ROUND_OFF] != 1){
							log_err_m("incorrect number of dependence of type ROUND_OFF: %u for inst SHLD", nb_dependence[IR_DEPENDENCE_TYPE_ROUND_OFF]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst SHLD", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SHR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SHR", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SHR", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst SHR", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SHRD 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_ROUND_OFF && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SHRD", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SHRD", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_ROUND_OFF] != 1){
							log_err_m("incorrect number of dependence of type ROUND_OFF: %u for inst SHRD", nb_dependence[IR_DEPENDENCE_TYPE_ROUND_OFF]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							log_err_m("incorrect number of dependence of type SHIFT_DISP: %u for inst SHRD", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_SUB 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SUBSTITUTE && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst SUB", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							log_err_m("incorrect number of dependence of type DIRECT: %u for inst SUB", nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SUBSTITUTE] != 1){
							log_err_m("incorrect number of dependence of type SUBSTITUTE: %u for inst SUB", nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP]);
							operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							result = 1;
						}
						break;
					}
					case IR_XOR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i == IR_DEPENDENCE_TYPE_MACRO){
								continue;
							}
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								log_err_m("incorrect dependence type %u for inst XOR", i);
								operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
								result = 1;
							}
						}
						break;
					}
					case IR_LOAD 		: {
						break;
					}
					case IR_STORE 		: {
						break;
					}
					case IR_JOKER 		: {
						break;
					}
					case IR_INVALID 	: {
						break;
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				/* Check input edge(s) */
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					dependence = ir_edge_get_dependence(edge_cursor);

					if (dependence->type != IR_DEPENDENCE_TYPE_MACRO || !IR_DEPENDENCE_MACRO_DESC_IS_INPUT(dependence->dependence_type.macro)){
						log_err("symbol has an incorrect type of dst dependence");
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
						break;
					}
				}

				/* Check output edge(s) */
				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					dependence = ir_edge_get_dependence(edge_cursor);

					if (dependence->type != IR_DEPENDENCE_TYPE_MACRO || !IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(dependence->dependence_type.macro)){
						log_err("symbol has an incorrect type of src dependence");
						operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
						result = 1;
						break;
					}
				}

				break;
			}
			case IR_OPERATION_TYPE_NULL 	: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst){
					log_err_m("NULL operation has %u dst edge(s)", node_cursor->nb_edge_dst);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src){
					log_err("NULL operation has no src edge");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				break;
			}
		}
	}

	return result;
}

static uint32_t ir_check_acyclic_recursive(struct node* node){
	struct irOperation* 	operation;
	struct edge*			edge_cursor;
	struct node*			operand_node;
	struct irOperation*		operand_operation;
	uint32_t 				result = 0;

	operation = ir_node_get_operation(node);
	operation->status_flag |= IR_OPERATION_STATUS_FLAG_TESTING;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand_node = edge_get_src(edge_cursor);
		operand_operation = ir_node_get_operation(operand_node);

		if (!(operand_operation->status_flag & (IR_OPERATION_STATUS_FLAG_TEST | IR_OPERATION_STATUS_FLAG_TESTING))){
			result |= ir_check_acyclic_recursive(operand_node);
		}
		else if (operand_operation->status_flag & IR_OPERATION_STATUS_FLAG_TESTING){
			log_err("cycle detected in graph");
			operand_operation->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
			result = 1;
		}
	}

	operation->status_flag = IR_OPERATION_STATUS_FLAG_TEST | (operation->status_flag & (~IR_OPERATION_STATUS_FLAG_TESTING));

	return result;
}

uint32_t ir_check_acyclic(struct ir* ir){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	uint32_t 				result = 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		operation_cursor->status_flag &= ~(IR_OPERATION_STATUS_FLAG_TEST | IR_OPERATION_STATUS_FLAG_TESTING);
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_TEST){
			continue;
		}
		result |= ir_check_acyclic_recursive(node_cursor);
	}

	return result;
}

uint32_t ir_check_order(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	int8_t 				direction 		= 0;
	struct edge* 		edge_cursor;
	uint32_t 			result 			= 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		operation_cursor->status_flag &= ~IR_OPERATION_STATUS_FLAG_TEST;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_TEST;

		if (direction == 0){
			if (node_cursor->nb_edge_dst == 0 && node_cursor->nb_edge_src > 0){
				direction = 1;
			}
			else if (node_cursor->nb_edge_dst > 0 && node_cursor->nb_edge_src == 0){
				direction = -1;
			}
			else if (node_cursor->nb_edge_dst > 0 && node_cursor->nb_edge_src > 0){
				log_err("incorrect first node (it has both input and output edges)");
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
		}
		else if (direction < 0){
			for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_OPERATION_STATUS_FLAG_TEST){
					log_err("direction src -> dst, but found tagged dst");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
			}
			for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (!(ir_node_get_operation(edge_get_dst(edge_cursor))->status_flag & IR_OPERATION_STATUS_FLAG_TEST)){
					log_err("direction src -> dst, but found untagged src");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
			}
		}
		else{
			for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (!(ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_OPERATION_STATUS_FLAG_TEST)){
					log_err("direction dst -> src, but found untagged dst");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
			}
			for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_node_get_operation(edge_get_dst(edge_cursor))->status_flag & IR_OPERATION_STATUS_FLAG_TEST){
					log_err("direction dst -> src, but found tagged src");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
			}
		}
	}

	return result;
}

uint32_t ir_check_instruction_index(struct ir* ir){
	struct node* 		node_cursor;
	struct edge*		edge_cursor;
	struct irOperation* operation_cursor;
	struct irOperation* operand;
	uint32_t 			result = 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->index == IR_OPERATION_INDEX_ADDRESS){
			continue;
		}

		if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
			if (operation_cursor->index != IR_OPERATION_INDEX_IMMEDIATE){
				log_err("incorrect index value for immediate node");
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
		}

		for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = ir_node_get_operation(edge_get_src(edge_cursor));

			if (operand->index != IR_OPERATION_INDEX_IMMEDIATE && operand->index != IR_OPERATION_INDEX_UNKOWN && operand->index != IR_OPERATION_INDEX_ADDRESS){
				if (operand->index > operation_cursor->index){
					log_err_m("a node (index: %u) has an operand with a higher index (%u)", operation_cursor->index, operand->index);
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
					break;
				}
			}
		}
	}

	return result;
}

uint32_t ir_check_memory(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct node* 		root 				= NULL;
	uint32_t 			result 				= 0;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
			operation_cursor->status_flag &= ~IR_OPERATION_STATUS_FLAG_TEST;
			root = node_cursor;
		}
	}

	if (root != NULL){
		for (node_cursor = root; ir_node_get_operation(node_cursor)->operation_type.mem.prev != NULL; ){
			operation_cursor = ir_node_get_operation(node_cursor);
			if (operation_cursor->operation_type.mem.order <= ir_node_get_operation(operation_cursor->operation_type.mem.prev)->operation_type.mem.order){
				log_err_m("memory access linked list is not ordered: %u <- %u", ir_node_get_operation(operation_cursor->operation_type.mem.prev)->operation_type.mem.order, operation_cursor->operation_type.mem.order);
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			if (ir_node_get_operation(operation_cursor->operation_type.mem.prev)->operation_type.mem.next != node_cursor){
				log_err("memory access linked list is broken (a->prev->next != a)");
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev;
			ir_node_get_operation(node_cursor)->status_flag |= IR_OPERATION_STATUS_FLAG_TEST;
		}

		for (node_cursor = root; ir_node_get_operation(node_cursor)->operation_type.mem.next != NULL; ){
			operation_cursor = ir_node_get_operation(node_cursor);
			if (operation_cursor->operation_type.mem.order >= ir_node_get_operation(operation_cursor->operation_type.mem.next)->operation_type.mem.order){
				log_err_m("memory access linked list is not ordered: %u -> %u", operation_cursor->operation_type.mem.order, ir_node_get_operation(operation_cursor->operation_type.mem.next)->operation_type.mem.order);
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			if (ir_node_get_operation(operation_cursor->operation_type.mem.next)->operation_type.mem.prev != node_cursor){
				log_err("memory access linked list is broken (a->next->prev != a)");
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.next;
			ir_node_get_operation(node_cursor)->status_flag |= IR_OPERATION_STATUS_FLAG_TEST;
		}

		for (node_cursor = node_get_next(root); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			operation_cursor = ir_node_get_operation(node_cursor);
			if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
				if ((operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_TEST) == 0){
					log_err("memory access linked list is not connected");
					operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
					result = 1;
				}
				else{
					operation_cursor->status_flag &= ~IR_OPERATION_STATUS_FLAG_TEST;
				}
			}
		}
	}

	return result;
}

static uint32_t ir_check_memory_adv_dst(struct node* node, uint32_t order){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	uint32_t 			result;
	struct irOperation* operation;

	operation = ir_node_get_operation(node);

	if (operation->type == IR_OPERATION_TYPE_IN_MEM){
		if (operation->operation_type.mem.order > order){
			log_err_m("memory order is incorrect along path starting at %p ...", (void*)node);
			operation->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
			return 1;
		}
		else{
			if ((operation->status_flag & IR_OPERATION_STATUS_FLAG_TEST) || (operation->status_flag & IR_OPERATION_STATUS_FLAG_TEST1)){
				return 0;
			}
			order = operation->operation_type.mem.order;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node), result = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		if (ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_INST){
			result |= ir_check_memory_adv_dst(node_cursor, order);
		}
	}

	operation->status_flag |= IR_OPERATION_STATUS_FLAG_TEST1;

	return result;
}

static uint32_t ir_check_memory_adv_src(struct node* node, uint32_t order){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	uint32_t 			result;
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_IN_MEM || operation->type == IR_OPERATION_TYPE_OUT_MEM){
		if (operation->operation_type.mem.order < order){
			log_err_m("memory order is incorrect along path ending at %p ...", (void*)node);
			operation->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
			return 1;
		}
		else{
			if ((operation->status_flag & IR_OPERATION_STATUS_FLAG_TEST) || (operation->status_flag & IR_OPERATION_STATUS_FLAG_TEST2)){
				return 0;
			}
			order = operation->operation_type.mem.order;
		}
	}

	for (edge_cursor = node_get_head_edge_src(node), result = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		node_cursor = edge_get_dst(edge_cursor);
		if (ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_INST){
			result |= ir_check_memory_adv_src(node_cursor, order);
		}
	}

	operation->status_flag |= IR_OPERATION_STATUS_FLAG_TEST2;

	return result;
}

uint32_t ir_check_memory_advanced(struct ir* ir){
	uint32_t 			result = 0;
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		ir_node_get_operation(node_cursor)->status_flag &= ~IR_OPERATION_STATUS_FLAG_TEST & ~IR_OPERATION_STATUS_FLAG_TEST1 & ~IR_OPERATION_STATUS_FLAG_TEST2;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
			if (ir_check_memory_adv_dst(node_cursor, operation_cursor->operation_type.mem.order)){
				log_err_m("... and ending at %p", (void*)node_cursor);
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			if (ir_check_memory_adv_src(node_cursor, operation_cursor->operation_type.mem.order)){
				log_err_m("... and starting at %p", (void*)node_cursor);
				operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
				result = 1;
			}
			operation_cursor->status_flag |= IR_OPERATION_STATUS_FLAG_TEST;
		}
	}

	return result;
}

void ir_check_clean_error_flag(struct ir* ir){
	struct node* node_cursor;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		ir_node_get_operation(node_cursor)->status_flag &= ~IR_OPERATION_STATUS_FLAG_ERROR;
	}
}
