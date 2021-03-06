#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VERBOSE
#include <time.h>
#endif

#include "irNormalize.h"
#include "irExpression.h"
#include "irVariableRange.h"
#include "dagPartialOrder.h"
#include "base.h"

#if IR_NORMALIZE_FOLD_CONSTANT == 1

static uint64_t irNormalize_cst_gen_add(uint64_t arg1, struct irOperation* arg2){
	return arg1 + ir_imm_operation_get_unsigned_value(arg2);
}

static uint64_t irNormalize_cst_gen_and(uint64_t arg1, struct irOperation* arg2){
	return arg1 & ir_imm_operation_get_unsigned_value(arg2);
}

static uint64_t irNormalize_cst_gen_imul(uint64_t arg1, struct irOperation* arg2){
	return (int64_t)arg1 * ir_imm_operation_get_signed_value(arg2);
}

static uint64_t irNormalize_cst_gen_mul(uint64_t arg1, struct irOperation* arg2){
	return arg1 * ir_imm_operation_get_unsigned_value(arg2);
}

static uint64_t irNormalize_cst_gen_or (uint64_t arg1, struct irOperation* arg2){
	return arg1 | ir_imm_operation_get_unsigned_value(arg2);
}

static uint64_t irNormalize_cst_gen_shl(uint64_t arg1, uint64_t arg2){
	return arg1 << arg2;
}

static uint64_t irNormalize_cst_gen_shr(uint64_t arg1, uint64_t arg2){
	return arg1 >> arg2;
}

static uint64_t irNormalize_cst_gen_xor(uint64_t arg1, struct irOperation* arg2){
	return arg1 ^ ir_imm_operation_get_unsigned_value(arg2);
}

static int32_t irNormalize_cst_generic(struct ir* ir, struct node* node, uint64_t abs_el, uint32_t is_abs_el, uint64_t neu_el, uint64_t(*compute)(uint64_t,struct irOperation*)){
	uint32_t 			nb_imm_operand 		= 0;
	uint32_t 			nb_sym_operand 		= 0;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite 	= NULL;
	uint64_t 			value 				= neu_el;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = edge_get_src(edge_cursor);
		operand_operation = ir_node_get_operation(operand);

		if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			nb_imm_operand ++;

			value = compute(value, operand_operation);

			if (operand->nb_edge_src == 1){
				possible_rewrite = operand;
			}
		}
		else{
			nb_sym_operand ++;
		}
	}

	if (!nb_imm_operand){
		return 0;
	}

	value &= bitmask64(ir_node_get_operation(node)->size);

	if ((is_abs_el && value == abs_el) || nb_sym_operand == 0){
		ir_convert_operation_to_imm(ir, node, value);
		return 1;
	}

	if (value == neu_el && nb_sym_operand == 1){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
				ir_merge_equivalent_node(ir, edge_get_src(edge_cursor), node);
				return 1;
			}
		}

		#ifdef EXTRA_CHECK
		log_err("this case is not supposed to happen");
		#endif

		return 0;
	}

	if (value == neu_el || nb_imm_operand > 1){
		if (value != neu_el){
			if (possible_rewrite == NULL){
				if ((possible_rewrite = ir_insert_immediate(ir, graph_get_tail_node(&(ir->graph)), ir_node_get_operation(node)->size, value)) == NULL){
					log_err("unable to add immediate to IR");
					return 0;
				}
				ir_add_dependence_check(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT)
			}
			else{
				ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value;
				ir_node_get_operation(possible_rewrite)->size = ir_node_get_operation(node)->size;
			}
		}
		else{
			possible_rewrite = NULL;
		}

		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){
			edge_cursor_next = edge_get_next_dst(edge_cursor);
			operand = edge_get_src(edge_cursor);

			if (ir_node_get_operation(operand)->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){
				ir_remove_dependence(ir, edge_cursor);
			}
		}

		return 1;
	}

	return 0;
}

static int32_t irNormalize_cst_shift(struct ir* ir, struct node* node, uint64_t(*compute)(uint64_t,uint64_t)){
	struct edge* edge_cursor;
	struct node* op1 			= NULL;
	struct node* op2 			= NULL;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			op2 = edge_get_src(edge_cursor);
		}
		else{
			op1 = edge_get_src(edge_cursor);
		}
	}

	if (op1 != NULL && op2 != NULL && ir_node_get_operation(op2)->type == IR_OPERATION_TYPE_IMM){
		if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2)) == 0){
			ir_merge_equivalent_node(ir, op1, node);
			return 1;
		}
		else if (ir_node_get_operation(op1)->type == IR_OPERATION_TYPE_IMM){
			ir_convert_operation_to_imm(ir, node, compute(ir_imm_operation_get_unsigned_value(ir_node_get_operation(op1)), ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2))));
			return 1;
		}
	}

	return 0;
}

static int32_t irNormalize_cst_movzx(struct ir* ir, struct node* node){
	struct edge* 			edge;
	struct irOperation* 	op;

	if ((edge = node_get_head_edge_dst(node)) == NULL){
		return 0;
	}

	op = ir_node_get_operation(edge_get_src(edge));
	if (op->type != IR_OPERATION_TYPE_IMM){
		return 0;
	}

	ir_convert_operation_to_imm(ir, node, ir_imm_operation_get_unsigned_value(op));
	return 1;
}

static int32_t irNormalize_fold_constant(struct ir* ir){
	struct irNodeIterator 	it;
	struct irOperation* 	operation_cursor;
	int32_t 				result;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
		return 0;
	}

	for (irNodeIterator_get_first(ir, &it), result = 0; irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it)){
		operation_cursor = irNodeIterator_get_operation(it);
		if (operation_cursor->type == IR_OPERATION_TYPE_INST){
			switch(operation_cursor->operation_type.inst.opcode){
				case IR_ADD 		: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), 0, 0, 0, irNormalize_cst_gen_add);
					break;
				}
				case IR_AND 		: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), 0, 1, bitmask64(operation_cursor->size), irNormalize_cst_gen_and);
					break;
				}
				case IR_IMUL 		: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), 0, 1, 1, irNormalize_cst_gen_imul);
					break;
				}
				case IR_MUL 		: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), 0, 1, 1, irNormalize_cst_gen_mul);
					break;
				}
				case IR_MOVZX 		: {
					result |= irNormalize_cst_movzx(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_OR 			: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), bitmask64(operation_cursor->size), 1, 0, irNormalize_cst_gen_or);
					break;
				}
				case IR_SHL 		: {
					result |= irNormalize_cst_shift(ir, irNodeIterator_get_node(it), irNormalize_cst_gen_shl);
					break;
				}
				case IR_SHR 		: {
					result |= irNormalize_cst_shift(ir, irNodeIterator_get_node(it), irNormalize_cst_gen_shr);
					break;
				}
				case IR_XOR 		: {
					result |= irNormalize_cst_generic(ir, irNodeIterator_get_node(it), 0, 0, 0, irNormalize_cst_gen_xor);
					break;
				}
				default 			: {
					break;
				}
			}
		}
	}

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif

	return result;
}

#endif

#if IR_NORMALIZE_DETECT_CST_EXPRESSION == 1

static int32_t irNormalize_detect_cst_expression(struct ir* ir){
	uint32_t 				i;
	struct irNodeIterator 	it;
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct edge* 			edge_cursor;
	int32_t 				result;
	uint64_t* 				mask_buffer;
	struct variableRange 	local_range;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
		return 0;
	}

	if ((mask_buffer = (uint64_t*)malloc(ir->graph.nb_node * sizeof(uint64_t))) == NULL){
		log_err("unable to allocate memory");
		return 0;
	}

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	for (irNodeIterator_get_first(ir, &it), i = 0, result = 0; irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it), i++){
		node_cursor = irNodeIterator_get_node(it);
		operation_cursor = irNodeIterator_get_operation(it);

		if (!node_cursor->nb_edge_dst){
			continue;
		}

		node_cursor->ptr = (void*)(mask_buffer + i);
		if (node_cursor->nb_edge_src && !irOperation_is_final(operation_cursor)){
			for (edge_cursor = node_get_head_edge_src(node_cursor), mask_buffer[i] = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				switch (ir_edge_get_dependence(edge_cursor)->type){
					case IR_DEPENDENCE_TYPE_DIRECT 		: {
						mask_buffer[i] |= *(uint64_t*)(edge_get_dst(edge_cursor)->ptr);
						break;
					}
					case IR_DEPENDENCE_TYPE_ADDRESS 	: {
						mask_buffer[i] = bitmask64(operation_cursor->size);
						break;
					}
					case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
						mask_buffer[i] = bitmask64(operation_cursor->size);
						break;
					}
					case IR_DEPENDENCE_TYPE_DIVISOR 	: {
						mask_buffer[i] = bitmask64(operation_cursor->size);
						break;
					}
					case IR_DEPENDENCE_TYPE_ROUND_OFF 	: {
						mask_buffer[i] = bitmask64(operation_cursor->size);
						break;
					}
					case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {
						mask_buffer[i] = bitmask64(operation_cursor->size);
						break;
					}
					case IR_DEPENDENCE_TYPE_MACRO 		: {
						break;
					}
				}
			}
			mask_buffer[i] &= bitmask64(operation_cursor->size);
		}
		else{
			mask_buffer[i] = bitmask64(operation_cursor->size);
		}

		if (operation_cursor->type == IR_OPERATION_TYPE_INST){
			irVariableRange_compute(irNodeIterator_get_node(it), &(operation_cursor->operation_type.inst.range), ir->range_seed);
			memcpy(&local_range, &(operation_cursor->operation_type.inst.range), sizeof(struct variableRange));
			variableRange_and_value(&local_range, mask_buffer[i]);
			if (variableRange_is_cst(&local_range)){
				ir_convert_operation_to_imm(ir, irNodeIterator_get_node(it), variableRange_get_cst(&local_range));
				result = 1;
				continue;
			}

			switch(operation_cursor->operation_type.inst.opcode){
				case IR_ADC 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_ADD 		: {mask_buffer[i] = irInfluenceMask_operation_add(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_AND 		: {mask_buffer[i] = irInfluenceMask_operation_and(node_cursor, mask_buffer[i]); break;}
				case IR_CMOV 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_DIVQ 		: {mask_buffer[i] = bitmask64(2 * operation_cursor->size); break;}
				case IR_DIVR 		: {mask_buffer[i] = bitmask64(2 * operation_cursor->size); break;}
				case IR_IDIVQ 		: {mask_buffer[i] = bitmask64(2 * operation_cursor->size); break;}
				case IR_IDIVR 		: {mask_buffer[i] = bitmask64(2 * operation_cursor->size); break;}
				case IR_IMUL 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_LEA 		: {break;} /* importer */
				case IR_MOV 		: {break;} /* importer */
				case IR_MOVZX 		: {break;}
				case IR_MUL 		: {break;}
				case IR_NEG 		: {break;}
				case IR_NOT 		: {break;}
				case IR_OR 			: {mask_buffer[i] = irInfluenceMask_operation_or(node_cursor, mask_buffer[i]); break;}
				case IR_PART1_8 	: {break;}
				case IR_PART2_8 	: {mask_buffer[i] = irInfluenceMask_operation_part2_8(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_PART1_16 	: {break;}
				case IR_ROL 		: {mask_buffer[i] = irInfluenceMask_operation_rol(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_ROR 		: {mask_buffer[i] = irInfluenceMask_operation_ror(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_SBB 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_SHL 		: {mask_buffer[i] = irInfluenceMask_operation_shl(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_SHLD 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_SHR 		: {mask_buffer[i] = irInfluenceMask_operation_shr(node_cursor, mask_buffer[i], DIR_DST_TO_SRC); break;}
				case IR_SHRD 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_SUB 		: {mask_buffer[i] = bitmask64(operation_cursor->size); break;}
				case IR_XOR 		: {break;}
				case IR_LOAD 		: {break;} /* signature */
				case IR_STORE 		: {break;} /* signature */
				case IR_JOKER 		: {break;} /* signature */
				case IR_INVALID 	: {break;} /* specific */
			}
		}
	}

	free(mask_buffer);

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif

	return result;
}

#endif

#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1

static int32_t ir_normalize_simplify_instruction_rewrite_add    (struct ir* ir, struct node* node, uint8_t final);
static int32_t ir_normalize_simplify_instruction_rewrite_and    (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_movzx  (struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_mul    (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_or     (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_part2_8(struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_rol    (struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_shl    (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_shld   (struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_shr    (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_shrd   (struct ir* ir, struct node* node); 					/* + light */
static int32_t ir_normalize_simplify_instruction_rewrite_sub    (struct ir* ir, struct node* node);
static int32_t ir_normalize_simplify_instruction_rewrite_xor    (struct ir* ir, struct node* node, uint8_t final);

typedef int32_t(*simplify_instruction_ptr)(struct ir*,struct node*,uint8_t);

static const simplify_instruction_ptr rewrite_simplify[NB_IR_OPCODE] = {
	NULL, 																			/* 0  IR_ADC 			*/
	ir_normalize_simplify_instruction_rewrite_add, 									/* 1  IR_ADD 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_and, 		/* 2  IR_AND 			*/
	NULL, 																			/* 3  IR_CMOV 			*/
	NULL, 																			/* 4  IR_DIVQ  			*/
	NULL, 																			/* 5  IR_DIVR  			*/
	NULL, 																			/* 6  IR_IDIVQ 			*/
	NULL, 																			/* 7  IR_IDIVR 			*/
	NULL, 																			/* 8  IR_IMUL 			*/
	NULL, 																			/* 9  IR_LEA 			*/
	NULL, 																			/* 10 IR_MOV 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_movzx, 		/* 11 IR_MOVZX 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_mul, 		/* 12 IR_MUL 			*/
	NULL, 																			/* 13 IR_NEG 			*/
	NULL, 																			/* 14 IR_NOT 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_or, 		/* 15 IR_OR 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_part1_8, 	/* 16 IR_PART1_8 		*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_part2_8, 	/* 17 IR_PART2_8 		*/
	NULL, 																			/* 18 IR_PART1_16 		*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_rol, 		/* 19 IR_ROL 			*/
	NULL, 																			/* 20 IR_ROR 			*/
	NULL, 																			/* 21 IR_SBB 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_shl, 		/* 22 IR_SHL 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_shld, 		/* 23 IR_SHLD 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_shr, 		/* 24 IR_SHR 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_shrd, 		/* 25 IR_SHRD 			*/
	(simplify_instruction_ptr)ir_normalize_simplify_instruction_rewrite_sub, 		/* 26 IR_SUB 			*/
	ir_normalize_simplify_instruction_rewrite_xor, 									/* 27 IR_XOR 			*/
	NULL, 																			/* 28 IR_LOAD 			*/
	NULL, 																			/* 29 IR_STORE 			*/
	NULL, 																			/* 30 IR_JOKER 			*/
	NULL 																			/* 31 IR_INVALID 		*/
};

static int32_t irNormalize_simplify_instruction(struct ir* ir, uint8_t final){
	struct irNodeIterator 	it;
	struct irOperation* 	operation_cursor;
	int32_t 				result;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort ir node(s)");
		return 0;
	}

	for (irNodeIterator_get_first(ir, &it), result = 0; irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it)){
		operation_cursor = ir_node_get_operation(irNodeIterator_get_node(it));

		if (operation_cursor->type == IR_OPERATION_TYPE_INST){
			if (rewrite_simplify[operation_cursor->operation_type.inst.opcode] != NULL){
				result |= rewrite_simplify[operation_cursor->operation_type.inst.opcode](ir, irNodeIterator_get_node(it), final);
			}
		}
	}

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif

	return result;
}

static int32_t irNormalize_simplify_instruction_std(struct ir* ir){
	return irNormalize_simplify_instruction(ir, 0);
}

static int32_t irNormalize_simplify_instruction_fin(struct ir* ir){
	return  irNormalize_simplify_instruction(ir, 1);
}

#endif

#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1

static const uint32_t delete_subexpression[NB_IR_OPCODE] = {
	0, /* 0  IR_ADC 					*/
	1, /* 1  IR_ADD 					*/
	1, /* 2  IR_AND 					*/
	0, /* 3  IR_CMOV 					*/
	1, /* 4  IR_DIVQ 					*/
	1, /* 5  IR_DIVR 					*/
	1, /* 6  IR_IDIVQ 					*/
	1, /* 7  IR_IDIVR 					*/
	1, /* 8  IR_IMUL 					*/
	1, /* 9  IR_LEA 		importer 	*/
	1, /* 10 IR_MOV 		importer 	*/
	1, /* 11 IR_MOVZX 					*/
	1, /* 12 IR_MUL 					*/
	1, /* 13 IR_NEG 					*/
	1, /* 14 IR_NOT 					*/
	1, /* 15 IR_OR 						*/
	1, /* 16 IR_PART1_8  	specific 	*/
	1, /* 17 IR_PART2_8 	specific 	*/
	1, /* 18 IR_PART1_16 	specific 	*/
	1, /* 19 IR_ROL 					*/
	1, /* 20 IR_ROR 					*/
	0, /* 21 IR_SBB 					*/
	1, /* 22 IR_SHL 					*/
	1, /* 23 IR_SHLD 					*/
	1, /* 24 IR_SHR 					*/
	1, /* 25 IR_SHRD 					*/
	1, /* 26 IR_SUB 					*/
	1, /* 27 IR_XOR 					*/
	0, /* 28 IR_LOAD 		signature 	*/
	0, /* 29 IR_STORE 		signature 	*/
	0, /* 30 IR_JOKER 		signature 	*/
	0  /* 31 IR_INVALID 	specific 	*/
};

static int32_t irNormalize_remove_common_subexpression(struct ir* ir){
	struct commonOperand{
		struct edge* 	edge1;
		struct edge* 	edge2;
	};

	#define commonOperand_swap(cop1, cop2) 			\
		(cop1)->edge2 = (cop2)->edge1; 				\
		(cop2)->edge1 = (cop1)->edge1; 				\
		(cop1)->edge1 = (cop1)->edge2;

	struct node* 			main_cursor;
	#define IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND 512
	struct commonOperand 	operand_buffer[IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND];
	uint32_t 				i;
	uint32_t 				nb_match;
	struct node* 			new_intermediate_inst;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct node* 			node1;
	struct node* 			node2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	struct edge* 			operand_cursor;
	struct edge* 			prev_edge_cursor1;
	struct edge* 			prev_edge_cursor2;
	uint32_t 				init_operand_buffer;
	uint32_t 				nb_edge_node1;
	uint32_t 				nb_edge_node2;
	int32_t 				result;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		log_err("unable to sort DAG");
		return 0;
	}

	for (main_cursor = graph_get_head_node(&(ir->graph)), result = 0; main_cursor != NULL; main_cursor = node_get_next(main_cursor)){
		if (main_cursor->nb_edge_src < 2 && ir_node_get_operation(main_cursor)->type == IR_OPERATION_TYPE_SYMBOL){
			continue;
		}

		for (edge_cursor1 = node_get_head_edge_src(main_cursor), prev_edge_cursor1 = NULL; edge_cursor1 != NULL; ){
			node1 = edge_get_dst(edge_cursor1);
			operation1 = ir_node_get_operation(node1);
			init_operand_buffer = 0;

			if (operation1->type != IR_OPERATION_TYPE_INST || !delete_subexpression[operation1->operation_type.inst.opcode]){
				goto next_cursor1;
			}

			for (edge_cursor2 = edge_get_next_src(edge_cursor1), prev_edge_cursor2 = NULL; edge_cursor2 != NULL; ){
				node2 = edge_get_dst(edge_cursor2);
				operation2 = ir_node_get_operation(node2);

				if (operation2->type != IR_OPERATION_TYPE_INST || operation1->size != operation2->size || operation1->operation_type.inst.opcode != operation2->operation_type.inst.opcode || node1 == node2 || ir_edge_get_dependence(edge_cursor1)->type != ir_edge_get_dependence(edge_cursor2)->type){
					goto next_cursor2;
				}

				if (!init_operand_buffer){
					for (operand_cursor = node_get_head_edge_dst(node1), nb_edge_node1 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
							continue;
						}

						if (nb_edge_node1 == IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND){
							log_warn_m("IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND has been reached: %u for %s", IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND, irOpcode_2_string(operation1->operation_type.inst.opcode));
							goto next_cursor1;
						}

						operand_buffer[nb_edge_node1 ++].edge1 = operand_cursor;
					}
					init_operand_buffer = 1;
				}

				for (operand_cursor = node_get_head_edge_dst(node2), nb_match = 0, nb_edge_node2 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
						continue;
					}

					for (i = nb_match, nb_edge_node2 ++; i < nb_edge_node1; i++){
						if (ir_edge_get_dependence(operand_buffer[i].edge1)->type == ir_edge_get_dependence(operand_cursor)->type){
							if (edge_get_src(operand_cursor) == edge_get_src(operand_buffer[i].edge1)){
								commonOperand_swap(operand_buffer + nb_match, operand_buffer + i);
								operand_buffer[nb_match ++].edge2 = operand_cursor;
								break;
							}
							else if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(operand_buffer[i].edge1))->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_cursor))) == ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_buffer[i].edge1)))){
								commonOperand_swap(operand_buffer + nb_match, operand_buffer + i);
								operand_buffer[nb_match ++].edge2 = operand_cursor;
								break;
							}
						}
					}
				}

				/* CASE 1 */
				if (nb_match > 0 && nb_match == nb_edge_node1 && nb_match == nb_edge_node2){
					operation1->index = min(operation2->index, operation1->index);
					ir_merge_equivalent_node(ir, node1, node2);

					result = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 2 */
				else if (nb_match > 1 && nb_match == nb_edge_node1){
					for (i = 0; i < nb_match; i++){
						ir_remove_dependence(ir, operand_buffer[i].edge2);
					}

					ir_add_dependence_check(ir, node1, node2, IR_DEPENDENCE_TYPE_DIRECT)

					operation2->index = (operation1->index > operation2->index) ? operation2->index : IR_OPERATION_INDEX_UNKOWN;

					result = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 3 */
				else if (nb_match > 1 && nb_match == nb_edge_node2){
					for (i = 0; i < nb_match; i++){
						ir_remove_dependence(ir, operand_buffer[i].edge1);
					}

					ir_add_dependence_check(ir, node2, node1, IR_DEPENDENCE_TYPE_DIRECT)

					operation1->index = (operation2->index > operation1->index) ? operation1->index : IR_OPERATION_INDEX_UNKOWN;

					result = 1;
					edge_cursor1 = prev_edge_cursor1;
					goto next_cursor1;
				}
				/* CASE 4 */
				else if (nb_match > 1 && (operation1->operation_type.inst.opcode == IR_ADD || operation1->operation_type.inst.opcode == IR_XOR)){
					new_intermediate_inst = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation1->size, operation1->operation_type.inst.opcode, (operation1->operation_type.inst.dst == operation2->operation_type.inst.dst) ? operation1->operation_type.inst.dst : IR_OPERATION_DST_UNKOWN);
					if (new_intermediate_inst == NULL){
						log_err("unable to add instruction to IR");
					}
					else{
						for (i = 0; i < nb_match; i++){
							ir_add_dependence_check(ir, edge_get_src(operand_buffer[i].edge1), new_intermediate_inst, IR_DEPENDENCE_TYPE_DIRECT)

							ir_remove_dependence(ir, operand_buffer[i].edge2);
							ir_remove_dependence(ir, operand_buffer[i].edge1);
						}

						ir_add_dependence_check(ir, new_intermediate_inst, node1, IR_DEPENDENCE_TYPE_DIRECT)
						ir_add_dependence_check(ir, new_intermediate_inst, node2, IR_DEPENDENCE_TYPE_DIRECT)

						operation1->index = IR_OPERATION_INDEX_UNKOWN;
						operation2->index = IR_OPERATION_INDEX_UNKOWN;

						result = 1;
						edge_cursor1 = prev_edge_cursor1;
						goto next_cursor1;
					}
				}

				next_cursor2:
				prev_edge_cursor2 = edge_cursor2;
				if (edge_cursor2 == NULL){
					edge_cursor2 = edge_get_next_src(edge_cursor1);
				}
				else{
					edge_cursor2 = edge_get_next_src(edge_cursor2);
				}
			}

			next_cursor1:
			prev_edge_cursor1 = edge_cursor1;
			if (edge_cursor1 == NULL){
				edge_cursor1 = node_get_head_edge_src(main_cursor);
			}
			else{
				edge_cursor1 = edge_get_next_src(edge_cursor1);
			}
		}
	}

	return result;
}

#endif

#if IR_NORMALIZE_DISTRIBUTE_CST == 1

static int32_t irNormalize_distribute_cst(struct ir* ir){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor1;
	struct irOperation* 	operation_cursor2;
	struct edge* 			node1_imm_operand;
	struct edge*			node1_inst_operand;
	int32_t 				result;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)), result = 0; node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation_cursor1 = ir_node_get_operation(node_cursor1);

		if (operation_cursor1->type == IR_OPERATION_TYPE_INST && node_cursor1->nb_edge_dst == 2){
			for (edge_cursor = node_get_head_edge_dst(node_cursor1), node1_imm_operand = NULL, node1_inst_operand = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				node_cursor2 = edge_get_src(edge_cursor);
				operation_cursor2 = ir_node_get_operation(node_cursor2);

				if (operation_cursor2->type == IR_OPERATION_TYPE_IMM && node1_imm_operand == NULL){
					node1_imm_operand = edge_cursor;
				}
				else if (operation_cursor2->type == IR_OPERATION_TYPE_INST && node1_inst_operand == NULL){
					/* What happens if node1_inst_operand has other children? Nothing is broken but is it wise to perform this so called simplification? */
					node1_inst_operand = edge_cursor;
				}
				else{
					goto next;
				}
			}

			if (node1_imm_operand != NULL && node1_inst_operand != NULL){
				node_cursor2 = edge_get_src(node1_inst_operand);
				operation_cursor2 = ir_node_get_operation(node_cursor2);

				if ((operation_cursor1->operation_type.inst.opcode == IR_MUL && operation_cursor2->operation_type.inst.opcode == IR_ADD) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode == IR_ADD) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode == IR_AND) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHR && operation_cursor2->operation_type.inst.opcode == IR_AND) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode == IR_OR)  ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHR && operation_cursor2->operation_type.inst.opcode == IR_OR)  ||
					(operation_cursor1->operation_type.inst.opcode == IR_AND && operation_cursor2->operation_type.inst.opcode == IR_OR)){

					ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

					for (edge_cursor = node_get_head_edge_dst(node_cursor2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						if (ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM){
							break;
						}
						else if (operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode != IR_ADD){
							struct variableRange 	range;
							struct variableRange* 	range_ptr;

							if ((range_ptr = ir_operation_get_range(ir_node_get_operation(edge_get_src(edge_cursor)))) != NULL){
								irVariableRange_compute(edge_get_src(edge_cursor), range_ptr, ir->range_seed);
								memcpy(&range, range_ptr, sizeof(struct variableRange));
							}
							else{
								irVariableRange_compute(edge_get_src(edge_cursor), &range, ir->range_seed);
							}

							variableRange_shl_value(&range, ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(node1_imm_operand))), operation_cursor1->size);
							if (variableRange_is_cst(&range)){
								break;
							}

						}
						else if (operation_cursor1->operation_type.inst.opcode == IR_SHR){
							struct variableRange 	range;
							struct variableRange* 	range_ptr;

							if ((range_ptr = ir_operation_get_range(ir_node_get_operation(edge_get_src(edge_cursor)))) != NULL){
								irVariableRange_compute(edge_get_src(edge_cursor), range_ptr, ir->range_seed);
								memcpy(&range, range_ptr, sizeof(struct variableRange));
							}
							else{
								irVariableRange_compute(edge_get_src(edge_cursor), &range, ir->range_seed);
							}

							variableRange_shr_value(&range, ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(node1_imm_operand))));
							if (variableRange_is_cst(&range)){
								break;
							}
						}
						else if (operation_cursor1->operation_type.inst.opcode == IR_AND){
							struct variableRange 	range;
							struct variableRange* 	range_ptr;

							if ((range_ptr = ir_operation_get_range(ir_node_get_operation(edge_get_src(edge_cursor)))) != NULL){
								irVariableRange_compute(edge_get_src(edge_cursor), range_ptr, ir->range_seed);
								memcpy(&range, range_ptr, sizeof(struct variableRange));
							}
							else{
								irVariableRange_compute(edge_get_src(edge_cursor), &range, ir->range_seed);
							}

							variableRange_and_value(&range, ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(node1_imm_operand))));
							if (variableRange_is_cst(&range)){
								break;
							}
						}
					}

					if (edge_cursor != NULL){
						struct node* new_node;

						for (edge_cursor = node_get_head_edge_dst(node_cursor2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							if ((new_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation_cursor1->size, operation_cursor1->operation_type.inst.opcode, IR_OPERATION_DST_UNKOWN)) == NULL){
								log_err("unable to add inst node to IR");
								continue;
							}

							ir_add_dependence_check(ir, edge_get_src(node1_imm_operand), new_node, ir_edge_get_dependence(node1_imm_operand)->type)
							ir_add_dependence_check(ir, edge_get_src(edge_cursor), new_node, IR_DEPENDENCE_TYPE_DIRECT)
							ir_add_dependence_check(ir, new_node, node_cursor1, ir_edge_get_dependence(edge_cursor)->type)
						}
						operation_cursor1->operation_type.inst.opcode = operation_cursor2->operation_type.inst.opcode;
						operation_cursor1->size = operation_cursor2->size;

						ir_remove_dependence(ir, node1_imm_operand);
						ir_remove_dependence(ir, node1_inst_operand);

						result = 1;
					}
				}
			}
		}

		next:;
	}

	return result;
}

#endif

#if IR_NORMALIZE_MERGE_CST == 1

static int32_t irNormalize_merge_cst(struct ir* ir){
	struct node* 			node_cursor1;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct edge* 			current_edge;
	struct irOperation* 	operation_cursor1;
	struct irOperation* 	operation_cursor2;
	uint32_t 				is_imm_operand;
	int32_t 				result;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)), result = 0; node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation_cursor1 = ir_node_get_operation(node_cursor1);

		if (operation_cursor1->type != IR_OPERATION_TYPE_INST || node_cursor1->nb_edge_dst != 2){
			continue;
		}

		if (operation_cursor1->operation_type.inst.opcode != IR_ADD && operation_cursor1->operation_type.inst.opcode != IR_AND){ /* add new instruction here */
			continue;
		}

		for (edge_cursor1 = node_get_head_edge_dst(node_cursor1), is_imm_operand = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
			if (ir_node_get_operation(edge_get_src(edge_cursor1))->type == IR_OPERATION_TYPE_IMM){
				is_imm_operand = 1;
				break;
			}
		}

		if (!is_imm_operand){
			continue;
		}

		for (edge_cursor1 = node_get_head_edge_dst(node_cursor1); edge_cursor1 != NULL; ){
			current_edge = edge_cursor1;
			edge_cursor1 = edge_get_next_dst(edge_cursor1);
			operation_cursor2 = ir_node_get_operation(edge_get_src(current_edge));

			if (operation_cursor2->type == IR_OPERATION_TYPE_INST && operation_cursor2->operation_type.inst.opcode == operation_cursor1->operation_type.inst.opcode){
				for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(current_edge)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						if (graph_copy_dst_edge(&(ir->graph), node_cursor1, edge_get_src(current_edge))){
							log_err("unable to copy dst edge");
						}
						ir_remove_dependence(ir, current_edge);

						result = 1;
						break;
					}
				}
			}
		}
	}

	return result;
}

#endif

struct irOperand{
	struct node*	node;
	struct edge* 	edge;
};

static int32_t irOperand_compare_node(const void* arg1, const void* arg2);

#ifdef VERBOSE

#define timer_start() 																																		\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){ 																						\
		log_err("clock_gettime fails"); 																													\
	}

#define timer_stop(time) 																																	\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){ 																						\
		log_err("clock_gettime fails"); 																													\
	} 																																						\
	else{ 																																					\
		(time) += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.; 					\
	}

#define timer_print(str, time) printf("%-24s %3.3f\n", str, time)

#define ir_normalize_print_notification(name, round_counter) log_info_m("modification " name " @ %u", round_counter)

#else

#define timer_start()
#define timer_stop(time)
#define timer_print(str, time)

#define ir_normalize_print_notification(name, round_counter)

#endif

#ifdef IR_FULL_CHECK

#define ir_normalize_check_ir(ir) ir_check(ir)

#else

#define ir_normalize_check_ir(ir)

#endif

#if defined VERBOSE || defined IR_FULL_CHECK

#define ir_normalize_apply_rule(func, name, time) 																											\
	timer_start(); 																																			\
	modification = func(ir); 																																\
	timer_stop(time); 																																		\
																																							\
	if (modification){ 																																		\
		ir_normalize_print_notification(name, round_counter); 																								\
		ir_normalize_check_ir(ir); 																															\
		modification = 0; 																																	\
		modification_copy = 1; 																																\
	}

#else

#define ir_normalize_apply_rule(func, name, time) modification |= func(ir)

#endif

struct normalizationMechanism{
	int32_t(*func)(struct ir*);
	char 	name[64];
};

static const struct normalizationMechanism normalization_mechanism[] = {
	#if IR_NORMALIZE_FOLD_CONSTANT == 1
	{
		.func 	= irNormalize_fold_constant,
		.name 	= "Fold Constant",
	},
	#endif
	#if IR_NORMALIZE_DETECT_CST_EXPRESSION == 1
	{
		.func 	= irNormalize_detect_cst_expression,
		.name 	= "Detect Cst Expression",
	},
	#endif
	#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
	{
		.func 	= irNormalize_simplify_instruction_std,
		.name 	= "Simplify Instruction",
	},
	#endif
	#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
	{
		.func 	= irNormalize_remove_common_subexpression,
		.name 	= "Remove Subexpression",
	},
	#endif
	#if IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
	{
		.func 	= irMemory_simplify,
		.name 	= "Simplify Memory Access",
	},
	#endif
	#if IR_NORMALIZE_DISTRIBUTE_CST == 1
	{
		.func 	= irNormalize_distribute_cst,
		.name 	= "Distribute Cst",
	},
	#endif
	#if IR_NORMALIZE_EXPAND_VARIABLE == 1
	{
		.func 	= irNormalize_expand_variable,
		.name 	= "Expand Variable",
	},
	#endif
	#if IR_NORMALIZE_COALESCE_MEMORY_ACCESS == 1
	{
		.func 	= irMemory_coalesce,
		.name 	= "Coalesce Memory Access"
	},
	#endif
	#if IR_NORMALIZE_AFFINE_EXPRESSION == 1
	{
		.func 	= irNormalize_affine_expression,
		.name 	= "Affine Exp Simplification",
	},
	#endif
	#if IR_NORMALIZE_MERGE_CST == 1
	{
		.func 	= irNormalize_merge_cst,
		.name 	= "Merge Cst",
	},
	#endif
};

void ir_normalize(struct ir* ir){
	uint32_t 		i;
	uint32_t 		modif;
	#if defined VERBOSE || defined IR_FULL_CHECK
	uint32_t 		local_modif;
	struct timespec timer_start_time;
	struct timespec timer_stop_time;
	double 			timer[sizeof(normalization_mechanism) / sizeof(struct normalizationMechanism)] = {0};
	#endif
	#ifdef VERBOSE
	uint32_t 		round_counter = 0;
	#endif
	#if defined VERBOSE && defined IR_PRINT_STEP
	char 			file_name[128];
	#endif

	do {
		#ifdef VERBOSE
		printf("*** ROUND %u ***\n", round_counter);
		#endif

		for (i = 0, modif = 0; i < (sizeof(normalization_mechanism) / sizeof(struct normalizationMechanism)); i++){
			#if defined VERBOSE || defined IR_FULL_CHECK
			timer_start();
			local_modif = normalization_mechanism[i].func(ir);
			timer_stop(timer[i]);

			if (local_modif){
				#ifdef VERBOSE
				log_info_m("modification %s @ %u", normalization_mechanism[i].name, round_counter);
				#endif
				ir_normalize_check_ir(ir);
				modif = 1;
				#if defined VERBOSE && defined IR_PRINT_STEP
				snprintf(file_name, sizeof(file_name), "%u_%u.dot", round_counter, i);
				graphPrintDot_print(&(ir->graph), file_name, NULL);
				#endif
			}
			#else
			modif |= normalization_mechanism[i].func(ir);
			#endif
		}

		#ifdef VERBOSE
		round_counter ++;
		#endif
	} while(modif);

	#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
	do {
		/* We don't log the time for this mechanism */
		modif = irNormalize_simplify_instruction_fin(ir);

		#if defined VERBOSE || defined IR_FULL_CHECK
		if (modif){
			#ifdef VERBOSE
			log_info("modification Simplify Instruction @ FINAL");
			#endif
			ir_normalize_check_ir(ir);
		}
		#endif
	} while(modif);
	#endif

	#ifdef VERBOSE
	for (i = 0; i < (sizeof(normalization_mechanism) / sizeof(struct normalizationMechanism)); i++){
		printf("%-25s %3.3f s\n", normalization_mechanism[i].name, timer[i]);
	}
	#endif
}

#if IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
void ir_normalize_concrete(struct ir* ir){
	#if defined VERBOSE || defined IR_FULL_CHECK
	if (irMemory_simplify_concrete(ir)){
		#ifdef VERBOSE
		log_info("modification simplify concrete memory access @ START");
		#endif
		#ifdef IR_FULL_CHECK
		ir_normalize_check_ir(ir);
		#endif
	}
	#else
	irMemory_simplify_concrete(ir);
	#endif
}
#endif

#define ir_normalize_simplify_instruction_light_movzx ir_normalize_simplify_instruction_rewrite_movzx
#define ir_normalize_simplify_instruction_light_part1_8 ir_normalize_simplify_instruction_rewrite_part1_8
#define ir_normalize_simplify_instruction_light_part2_8 ir_normalize_simplify_instruction_rewrite_part2_8
#define ir_normalize_simplify_instruction_light_rol ir_normalize_simplify_instruction_rewrite_rol
#define ir_normalize_simplify_instruction_light_shld ir_normalize_simplify_instruction_rewrite_shld
#define ir_normalize_simplify_instruction_light_shrd ir_normalize_simplify_instruction_rewrite_shrd
static int32_t ir_normalize_simplify_instruction_light_xor(struct ir* ir, struct node* node);

static int32_t irNormalize_simplify_instruction_light(struct ir* ir){
	struct irNodeIterator 	it;
	struct irOperation* 	operation_cursor;
	int32_t 				result;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort ir node(s)");
		return 0;
	}

	for(irNodeIterator_get_first(ir, &it), result = 0; irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it)){
		operation_cursor = ir_node_get_operation(irNodeIterator_get_node(it));

		if (operation_cursor->type == IR_OPERATION_TYPE_INST){
			switch (operation_cursor->operation_type.inst.opcode){
				case IR_MOVZX 	: {
					result |= ir_normalize_simplify_instruction_light_movzx(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_PART1_8 : {
					result |= ir_normalize_simplify_instruction_light_part1_8(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_PART2_8 : {
					result |= ir_normalize_simplify_instruction_light_part2_8(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_ROL 	: {
					result |= ir_normalize_simplify_instruction_light_rol(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_SHLD 	: {
					result |= ir_normalize_simplify_instruction_light_shld(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_SHRD 	: {
					result |= ir_normalize_simplify_instruction_light_shrd(ir, irNodeIterator_get_node(it));
					break;
				}
				case IR_XOR 	: {
					result |= ir_normalize_simplify_instruction_light_xor(ir, irNodeIterator_get_node(it));
					break;
				}
				default 		: {
					break;
				}
			}
		}
	}

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif

	return result;
}

void ir_normalize_light(struct ir* ir){
	#if defined VERBOSE || defined IR_FULL_CHECK
	uint32_t 		modification_copy;
	#ifdef VERBOSE
	uint32_t 		round_counter 			= 0;
	double 			timer_1_elapsed_time 	= 0.0;
	double 			timer_2_elapsed_time 	= 0.0;
	struct timespec timer_start_time;
	struct timespec timer_stop_time;
	#endif
	#endif
	uint32_t 		modification;

	do {
		modification = 0;

		#if defined VERBOSE || defined IR_FULL_CHECK
		modification_copy = 0;
		#ifdef VERBOSE
		printf("*** ROUND %u ***\n", round_counter);
		#endif
		#endif

		#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
		ir_normalize_apply_rule(irNormalize_simplify_instruction_light, "simplify instruction", timer_1_elapsed_time);
		#endif

		#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
		ir_normalize_apply_rule(irNormalize_remove_common_subexpression, "remove subexpression", timer_2_elapsed_time);
		#endif

		#if defined VERBOSE || defined IR_FULL_CHECK
		modification = modification_copy;
		#ifdef VERBOSE
		round_counter ++;
		#endif
		#endif
	} while(modification);

	timer_print("Simplify instruction", timer_1_elapsed_time);
	timer_print("Remove subexpression", timer_2_elapsed_time);
}

#define skip_if_macro_operand(node) 																														\
	{ 																																						\
		struct edge* __edge_cursor__; 																														\
																																							\
		for (__edge_cursor__ = node_get_head_edge_dst(node); __edge_cursor__ != NULL; __edge_cursor__ = edge_get_next_dst(__edge_cursor__)){ 				\
			if (ir_edge_get_dependence(__edge_cursor__)->type == IR_DEPENDENCE_TYPE_MACRO){ 																\
				return result; 																																\
			} 																																				\
		} 																																					\
	}

#ifdef EXTRA_CHECK
#define assert_nb_dst_edge(node, value, node_desc) 																											\
	if ((node)->nb_edge_dst != (value)){ 																													\
		log_err_m("incorrect " node_desc " format (%u dst edge(s))", (node)->nb_edge_dst); 																	\
		return result; 																																		\
	}
#else
#define assert_nb_dst_edge(node, value, node_desc)
#endif

#ifdef EXTRA_CHECK
static inline struct edge* get_non_macro_operand(struct node* node){
	struct edge* edge_cursor;
	struct edge* result;

	for (edge_cursor = node_get_head_edge_dst(node), result = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_MACRO){
			if (result != NULL){
				log_err("several non-macro operand");
				return NULL;
			}
			else{
				result = edge_cursor;
			}
		}
	}

	if (result == NULL){
		log_err("zero non-macro operand");
	}

	return result;
}
#else
static inline struct edge* get_non_macro_operand(struct node* node){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_MACRO){
			return edge_cursor;
		}
	}

	return NULL;
}
#endif

static int32_t ir_normalize_simplify_instruction_rewrite_add(struct ir* ir, struct node* node, uint8_t final){
	struct edge* 		edge_cursor1;
	struct edge* 		edge_cursor2;
	struct edge* 		current_edge;
	struct irOperation* operand_operation;
	int32_t 			diff;
	int32_t 			result;

	for (edge_cursor1 = node_get_head_edge_dst(node), result = 0; edge_cursor1 != NULL; ){
		current_edge = edge_cursor1;
		edge_cursor1 = edge_get_next_dst(edge_cursor1);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_ADD){
			if (operand_operation->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN || ir_node_get_operation(node)->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN){
				log_warn("unknown instruction dst");
				diff = 0;
			}
			else{
				diff = operand_operation->operation_type.inst.dst - ir_node_get_operation(node)->operation_type.inst.dst;
			}

			if (!diff){
				if (edge_get_src(current_edge)->nb_edge_src == 1){
					graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
					ir_remove_node(ir, edge_get_src(current_edge));

					result = 1;
					continue;
				}
				else if (final){
					for (edge_cursor2 = node_get_head_edge_src(edge_get_src(current_edge)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_src(edge_cursor2)){
						if (edge_cursor2 != current_edge && (ir_node_get_operation(edge_get_dst(edge_cursor2))->type != IR_OPERATION_TYPE_OUT_MEM || ir_edge_get_dependence(edge_cursor2)->type == IR_DEPENDENCE_TYPE_ADDRESS)){
							break;
						}
					}
					if (edge_cursor2 == NULL){
						if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
							log_err("unable to copy dst edge");
						}
						ir_remove_dependence(ir, current_edge);

						result = 1;
						continue;
					}
				}
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_and(struct ir* ir, struct node* node){
	struct edge* 			imm_operand 	= NULL;
	struct edge* 			edge_cursor1;
	struct node* 			operand;
	uint64_t 				imm_value;
	struct node** 			operand_buffer;
	uint32_t 				nb_operand;
	int32_t 				result 			= 0;

	operand_buffer = (struct node**)alloca(sizeof(struct node*) * node->nb_edge_dst);

	for (edge_cursor1 = node_get_head_edge_dst(node), nb_operand = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
		operand = edge_get_src(edge_cursor1);

		if (ir_node_get_operation(operand)->type == IR_OPERATION_TYPE_IMM){
			imm_operand = edge_cursor1;
		}
		else{
			operand_buffer[nb_operand ++] = operand;
		}
	}

	if (imm_operand != NULL){
		imm_value = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(imm_operand)));
		if (variableRange_is_mask_compact(imm_value)){
			struct variableRange range;
			struct variableRange mask_range;

			ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

			irVariableRange_get_range_and_buffer(&range, operand_buffer, nb_operand, ir_node_get_operation(node)->size, ir->range_seed);
			variableRange_init_mask(&mask_range, imm_value, ir_node_get_operation(edge_get_src(imm_operand))->size);

			if (variableRange_is_range_include(&mask_range, &range)){
				ir_remove_dependence(ir, imm_operand);

				if (node->nb_edge_dst == 1){
					ir_merge_equivalent_node(ir, edge_get_src(node_get_head_edge_dst(node)), node);
					return 1;
				}

				result = 1;
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node){
	struct edge* 		operand_edge;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		node_imm_new;
	uint32_t 			result 				= 0;

	skip_if_macro_operand(node)
	assert_nb_dst_edge(node, 1, "MOVZX")

	operand_edge 		= node_get_head_edge_dst(node);
	operand 			= edge_get_src(operand_edge);
	operand_operation 	= ir_node_get_operation(operand);

	if (operand_operation->type == IR_OPERATION_TYPE_INST && (operand_operation->operation_type.inst.opcode == IR_PART1_8 || operand_operation->operation_type.inst.opcode == IR_PART1_16)){
		skip_if_macro_operand(operand)
		assert_nb_dst_edge(operand, 1, "PART1_*")

		if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size != ir_node_get_operation(node)->size){
			return result;
		}

		if ((node_imm_new = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, bitmask64(operand_operation->size))) == NULL){
			log_err("unable to add immediate to IR");
			return result;
		}

		if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
			log_err("unable to add dependency to IR");
			return result;
		}

		ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;
		graph_copy_dst_edge(&(ir->graph), node, operand);
		ir_remove_dependence(ir, operand_edge);
		result = 1;
	}
	else if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_PART2_8){
		skip_if_macro_operand(operand)
		assert_nb_dst_edge(operand, 1, "PART2_8")

		if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size != ir_node_get_operation(node)->size){
			return result;
		}

		if (operand->nb_edge_src == 1){
			if ((node_imm_new = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, 0x00000000000000ff)) == NULL){
				log_err("unable to add immediate to IR");
				return result;
			}

			if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add dependency to IR");
				return result;
			}

			ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

			operand_operation->operation_type.inst.opcode = IR_SHR;
			operand_operation->size = ir_node_get_operation(node)->size;

			if ((node_imm_new = ir_insert_immediate(ir, operand, 8, 8)) != NULL){
				ir_add_dependence_check(ir, node_imm_new, operand, IR_DEPENDENCE_TYPE_SHIFT_DISP)
				result = 1;
			}
			else{
				log_err("unable to add immediate to IR");
			}
		}
		else{
			log_warn("PART2_8 instruction is shared, this case is not implemented yet -> skip");
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_mul(struct ir* ir, struct node* node){
	struct edge* 			edge_cursor;
	struct irOperation*		operation_cursor;
	struct node* 			imm_node;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operation_cursor->type == IR_OPERATION_TYPE_IMM && __builtin_popcountll(ir_imm_operation_get_unsigned_value(operation_cursor)) == 1){
				if ((imm_node = ir_insert_immediate(ir, node, 8, __builtin_ctzll(ir_imm_operation_get_unsigned_value(operation_cursor)))) == NULL){
					log_err("unable to add immediate to IR");
					return 0;
				}

				ir_node_get_operation(node)->operation_type.inst.opcode = IR_SHL;

				ir_add_dependence_check(ir, imm_node, node, IR_DEPENDENCE_TYPE_SHIFT_DISP)
				ir_remove_dependence(ir, edge_cursor);

				return 1;
			}
		}
	}

	return 0;
}

static int32_t ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node){
	struct edge* 			edge_cursor;
	struct edge* 			current_edge;
	struct irOperation* 	operand_operation;
	struct irOperand*		operand_buffer;
	uint32_t 				nb_operand;
	uint32_t 				i;
	int32_t 				result 				= 0;

	operand_buffer = (struct irOperand*)alloca(sizeof(struct irOperand) * node->nb_edge_dst);

	for (edge_cursor = node_get_head_edge_dst(node), nb_operand = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand_buffer[nb_operand].node = edge_get_src(edge_cursor);
		operand_buffer[nb_operand].edge = edge_cursor;

		if (ir_node_get_operation(operand_buffer[nb_operand].node)->type != IR_OPERATION_TYPE_SYMBOL){
			nb_operand ++;
		}
	}

	if (nb_operand >= 2){
		qsort(operand_buffer, nb_operand, sizeof(struct irOperand), irOperand_compare_node);

		for (i = 1; i < nb_operand; i++){
			if (operand_buffer[i - 1].node == operand_buffer[i].node){
				ir_remove_dependence(ir, operand_buffer[i - 1].edge);
				result = 1;
			}
		}

		if (node->nb_edge_dst == 1){
			ir_merge_equivalent_node(ir, edge_get_src(node_get_head_edge_dst(node)), node);
			return 1;
		}
	}

	/* merge */
	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		current_edge = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_OR){
			if (edge_get_src(current_edge)->nb_edge_src == 1){
				graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
				ir_remove_node(ir, edge_get_src(current_edge));

				result= 1;
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node){
	struct edge* 		operand_edge;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	uint32_t 			result 				= 0;

	skip_if_macro_operand(node)
	assert_nb_dst_edge(node, 1, "PART1_8")

	operand_edge 		= node_get_head_edge_dst(node);
	operand 			= edge_get_src(operand_edge);
	operand_operation 	= ir_node_get_operation(operand);

	if (operand_operation->type == IR_OPERATION_TYPE_INST){
		if (operand_operation->operation_type.inst.opcode == IR_MOVZX){
			skip_if_macro_operand(operand)
			assert_nb_dst_edge(operand, 1, "MOVZX")

			if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size == ir_node_get_operation(node)->size){
				graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(operand)), node);
				ir_remove_node(ir, node);

				result = 1;
			}
		}
		else if (operand_operation->operation_type.inst.opcode == IR_PART1_16){
			skip_if_macro_operand(operand)
			assert_nb_dst_edge(operand, 1, "PART1_16")

			if (ir_add_dependence(ir, edge_get_src(node_get_head_edge_dst(operand)), node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add dependency to IR");
			}
			else{
				ir_remove_dependence(ir, operand_edge);

				result = 1;
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_part2_8(struct ir* ir, struct node* node){
	struct edge* 		operand_edge;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	uint32_t 			result 				= 0;

	skip_if_macro_operand(node)
	assert_nb_dst_edge(node, 1, "PART2_8")

	operand_edge 		= node_get_head_edge_dst(node);
	operand 			= edge_get_src(operand_edge);
	operand_operation 	= ir_node_get_operation(operand);

	if (operand_operation->type == IR_OPERATION_TYPE_INST){
		if (operand_operation->operation_type.inst.opcode == IR_PART1_16){
			skip_if_macro_operand(operand)
			assert_nb_dst_edge(operand, 1, "PART1_16")

			if (ir_add_dependence(ir, edge_get_src(node_get_head_edge_dst(operand)), node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add dependency to IR");
			}
			else{
				ir_remove_dependence(ir, operand_edge);

				result = 1;
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;
	struct node* 		new_imm;
	uint32_t 			result 				= 0;

	skip_if_macro_operand(node)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operand_operation->type == IR_OPERATION_TYPE_IMM && ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			if ((new_imm = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, ir_node_get_operation(node)->size - ir_imm_operation_get_unsigned_value(operand_operation))) == NULL){
				log_err("unable to add immediate to IR");
			}
			else{
				if (ir_add_dependence(ir, new_imm, node, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
					log_err("unable to add dependency to IR");
				}
				else{
					ir_remove_dependence(ir, edge_cursor);
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROR;

					result = 1;
				}
			}

			break;
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operation_cursor;
	struct edge*		edge_dire1 			= NULL;
	struct edge* 		edge_disp1 			= NULL;
	struct node* 		operand_dire2 		= NULL;
	struct node* 		operand_disp2 		= NULL;
	struct node*		new_imm;
	int32_t 			result 				= 0;

	assert_nb_dst_edge(node, 2, "SHL")

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT && operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR)){
			edge_dire1 = edge_cursor;
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operation_cursor->type == IR_OPERATION_TYPE_IMM){
			edge_disp1 = edge_cursor;
		}
	}

	if (edge_dire1 == NULL || edge_disp1 == NULL){
		return result;
	}

	assert_nb_dst_edge(edge_get_src(edge_dire1), 2, "SH*")

	for (edge_cursor = node_get_head_edge_dst(edge_get_src(edge_dire1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operation_cursor->type == IR_OPERATION_TYPE_IMM){
			operand_disp2 = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			operand_dire2 = edge_get_src(edge_cursor);
		}
	}

	if (operand_disp2 == NULL || operand_dire2 == NULL){
		return result;
	}

	if (ir_node_get_operation(edge_get_src(edge_dire1))->operation_type.inst.opcode == IR_SHL){
		if ((new_imm = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_disp1))) + ir_imm_operation_get_unsigned_value(ir_node_get_operation(operand_disp2)))) == NULL){
			log_err("unable to add immediate to IR");
			return result;
		}

		ir_add_dependence_check(ir, new_imm, node, IR_DEPENDENCE_TYPE_SHIFT_DISP)
		ir_add_dependence_check(ir, operand_dire2, node, IR_DEPENDENCE_TYPE_DIRECT)

		ir_remove_dependence(ir, edge_disp1);
		ir_remove_dependence(ir, edge_dire1);
	}
	else{
		uint64_t 		value_shl;
		uint64_t 		value_shr;
		struct node* 	new_ope;

		value_shl = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_disp1)));
		value_shr = ir_imm_operation_get_unsigned_value(ir_node_get_operation(operand_disp2));

		if (value_shl < value_shr){
			if ((new_ope = ir_insert_inst(ir, node, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(node)->size, IR_SHR, ir_node_get_operation(node)->operation_type.inst.dst)) == NULL){
				log_err("unable to add operation to IR");
				return result;
			}

			if ((new_imm = ir_insert_immediate(ir, new_ope, ir_node_get_operation(node)->size, value_shr - value_shl)) == NULL){
				log_err("unable to add immediate to IR");
				return result;
			}

			ir_add_dependence_check(ir, new_imm, new_ope, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, operand_dire2, new_ope, IR_DEPENDENCE_TYPE_DIRECT)
		}
		else if (value_shr < value_shl){
			if ((new_ope = ir_insert_inst(ir, node, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(node)->size, IR_SHL, ir_node_get_operation(node)->operation_type.inst.dst)) == NULL){
				log_err("unable to add operation to IR");
				return result;
			}

			if ((new_imm = ir_insert_immediate(ir, new_ope, ir_node_get_operation(node)->size, value_shl - value_shr)) == NULL){
				log_err("unable to add immediate to IR");
				return result;
			}

			ir_add_dependence_check(ir, new_imm, new_ope, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, operand_dire2, new_ope, IR_DEPENDENCE_TYPE_DIRECT)
		}
		else{
			new_ope = operand_dire2;
		}

		if ((new_imm = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, (bitmask64(ir_node_get_operation(node)->size)) << value_shl)) == NULL){
			log_err("unable to add immediate to IR");
			return result;
		}

		ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

		ir_add_dependence_check(ir, new_imm, node, IR_DEPENDENCE_TYPE_DIRECT)
		ir_add_dependence_check(ir, new_ope, node, IR_DEPENDENCE_TYPE_DIRECT)

		ir_remove_dependence(ir, edge_disp1);
		ir_remove_dependence(ir, edge_dire1);
	}

	result = 1;

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_shld(struct ir* ir, struct node* node){
	struct node* arg1 			= NULL;
	struct node* arg2 			= NULL;
	struct edge* edge_arg2 		= NULL;
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			arg1 = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ROUND_OFF){
			arg2 = edge_get_src(edge_cursor);
			edge_arg2 = edge_cursor;
		}
	}

	if (arg1 != NULL && arg1 == arg2){
		ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROL;
		ir_remove_dependence(ir, edge_arg2);

		return 1;
	}

	return 0;
}

static int32_t ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operation_cursor;
	struct edge*		edge_dire1 			= NULL;
	struct edge* 		edge_disp1 			= NULL;
	struct node* 		operand_dire2 		= NULL;
	struct node* 		operand_disp2 		= NULL;
	struct node*		new_imm;
	int32_t 			result 				= 0;

	assert_nb_dst_edge(node, 2, "SHR")

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT && operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR)){
			edge_dire1 = edge_cursor;
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operation_cursor->type == IR_OPERATION_TYPE_IMM){
			edge_disp1 = edge_cursor;
		}
	}

	if (edge_dire1 == NULL || edge_disp1 == NULL){
		return result;
	}

	assert_nb_dst_edge(edge_get_src(edge_dire1), 2, "SH*")

	for (edge_cursor = node_get_head_edge_dst(edge_get_src(edge_dire1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operation_cursor->type == IR_OPERATION_TYPE_IMM){
			operand_disp2 = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			operand_dire2 = edge_get_src(edge_cursor);
		}
	}

	if (operand_disp2 == NULL || operand_dire2 == NULL){
		return result;
	}

	if (ir_node_get_operation(edge_get_src(edge_dire1))->operation_type.inst.opcode == IR_SHR){
		if ((new_imm = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_disp1))) + ir_imm_operation_get_unsigned_value(ir_node_get_operation(operand_disp2)))) == NULL){
			log_err("unable to add immediate to IR");
			return result;
		}

		ir_add_dependence_check(ir, new_imm, node, IR_DEPENDENCE_TYPE_SHIFT_DISP)
		ir_add_dependence_check(ir, operand_dire2, node, IR_DEPENDENCE_TYPE_DIRECT)

		ir_remove_dependence(ir, edge_disp1);
		ir_remove_dependence(ir, edge_dire1);
	}
	else{
		uint64_t 		value_shl;
		uint64_t 		value_shr;
		struct node* 	new_ope;

		value_shr = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_disp1)));
		value_shl = ir_imm_operation_get_unsigned_value(ir_node_get_operation(operand_disp2));

		if (value_shl < value_shr){
			if ((new_ope = ir_insert_inst(ir, node, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(node)->size, IR_SHR, ir_node_get_operation(node)->operation_type.inst.dst)) == NULL){
				log_err("unable to add operation to IR");
				return result;
			}

			if ((new_imm = ir_insert_immediate(ir, new_ope, ir_node_get_operation(node)->size, value_shr - value_shl)) == NULL){
				log_err("unable to add immediate to IR");
				return result;
			}

			ir_add_dependence_check(ir, new_imm, new_ope, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, operand_dire2, new_ope, IR_DEPENDENCE_TYPE_DIRECT)
		}
		else if (value_shr < value_shl){
			if ((new_ope = ir_insert_inst(ir, node, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(node)->size, IR_SHL, ir_node_get_operation(node)->operation_type.inst.dst)) == NULL){
				log_err("unable to add operation to IR");
				return result;
			}

			if ((new_imm = ir_insert_immediate(ir, new_ope, ir_node_get_operation(node)->size, value_shl - value_shr)) == NULL){
				log_err("unable to add immediate to IR");
				return result;
			}

			ir_add_dependence_check(ir, new_imm, new_ope, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, operand_dire2, new_ope, IR_DEPENDENCE_TYPE_DIRECT)
		}
		else{
			new_ope = operand_dire2;
		}

		if ((new_imm = ir_insert_immediate(ir, node, ir_node_get_operation(node)->size, (bitmask64(ir_node_get_operation(node)->size)) >> value_shr)) == NULL){
			log_err("unable to add immediate to IR");
			return result;
		}

		ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

		ir_add_dependence_check(ir, new_imm, node, IR_DEPENDENCE_TYPE_DIRECT)
		ir_add_dependence_check(ir, new_ope, node, IR_DEPENDENCE_TYPE_DIRECT)

		ir_remove_dependence(ir, edge_disp1);
		ir_remove_dependence(ir, edge_dire1);
	}

	result = 1;

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_shrd(struct ir* ir, struct node* node){
	struct node* arg1 			= NULL;
	struct node* arg2 			= NULL;
	struct edge* edge_arg2 		= NULL;
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			arg1 = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ROUND_OFF){
			arg2 = edge_get_src(edge_cursor);
			edge_arg2 = edge_cursor;
		}
	}

	if (arg1 != NULL && arg1 == arg2){
		ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROR;
		ir_remove_dependence(ir, edge_arg2);

		return 1;
	}

	return 0;
}

static int32_t ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;
	struct node* 		new_imm;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SUBSTITUTE){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if ((new_imm = ir_insert_immediate(ir, node, operand_operation->size, (uint64_t)(-ir_imm_operation_get_signed_value(operand_operation)))) == NULL){
					log_err("unable to add immediate to IR");
					return 0;
				}

				ir_node_get_operation(node)->operation_type.inst.opcode = IR_ADD;

				ir_add_dependence_check(ir, new_imm, node, IR_DEPENDENCE_TYPE_DIRECT)
				ir_remove_dependence(ir, edge_cursor);

				return 1;
			}
		}
	}

	return 0;
}

static int32_t ir_normalize_simplify_instruction_light_xor(struct ir* ir, struct node* node){
	struct edge* 			edge_cursor;
	struct irOperand*		operand_list;
	uint32_t 				nb_operand;
	uint32_t 				i;
	int32_t 				result 			= 0;

	if (node->nb_edge_dst >= 2){
		operand_list = (struct irOperand*)alloca(sizeof(struct irOperand) * node->nb_edge_dst);

		for (edge_cursor = node_get_head_edge_dst(node), nb_operand = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_list[nb_operand].node = edge_get_src(edge_cursor);
			operand_list[nb_operand].edge = edge_cursor;

			if (ir_node_get_operation(operand_list[nb_operand].node)->type != IR_OPERATION_TYPE_SYMBOL){
				nb_operand ++;
			}
		}

		if (nb_operand >= 2){
			qsort(operand_list, nb_operand, sizeof(struct irOperand), irOperand_compare_node);

			for (i = 1; i < nb_operand; i++){
				if (operand_list[i - 1].node == operand_list[i].node){
					struct node* 	imm_zero;
					uint8_t 		size;

					size = ir_node_get_operation(operand_list[i].node)->size;
					ir_remove_dependence(ir, operand_list[i - 1].edge);
					ir_remove_dependence(ir, operand_list[i].edge);
					if ((imm_zero = ir_insert_immediate(ir, node, size, 0)) != NULL){
						ir_add_dependence_check(ir, imm_zero, node, IR_DEPENDENCE_TYPE_DIRECT)
					}
					else{
						log_err("unable to add immediate to IR");
					}

					i++;
					result = 1;
				}
			}

			if (result && node->nb_edge_dst == 1){
				edge_cursor = node_get_head_edge_dst(node);
				if (ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_cursor))) == 0){
					ir_merge_equivalent_node(ir, edge_get_src(edge_cursor), node);
				}
			}
		}
	}

	return result;
}

static int32_t ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t final){
	struct edge* 		edge_cursor;
	struct edge* 		current_edge;
	struct irOperation*	operand_operation;
	int32_t 			diff;
	struct irOperand*	operand_buffer;
	uint32_t 			nb_operand;
	uint32_t 			i;
	int32_t 			result 				= 0;

	operand_buffer = (struct irOperand*)alloca(sizeof(struct irOperand) * node->nb_edge_dst);

	for (edge_cursor = node_get_head_edge_dst(node), nb_operand = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand_buffer[nb_operand].node = edge_get_src(edge_cursor);
		operand_buffer[nb_operand].edge = edge_cursor;

		if (ir_node_get_operation(operand_buffer[nb_operand].node)->type != IR_OPERATION_TYPE_SYMBOL){
			nb_operand ++;
		}
	}

	if (nb_operand >= 2){
		qsort(operand_buffer, nb_operand, sizeof(struct irOperand), irOperand_compare_node);

		for (i = 1; i < nb_operand; i++){
			if (operand_buffer[i - 1].node == operand_buffer[i].node){
				ir_remove_dependence(ir, operand_buffer[i - 1].edge);
				ir_remove_dependence(ir, operand_buffer[i].edge);

				i++;
				result = 1;
			}
		}

		if (node->nb_edge_dst == 0){
			ir_convert_operation_to_imm(ir, node, 0);
			return 1;
		}
		if (node->nb_edge_dst == 1){
			ir_merge_equivalent_node(ir, edge_get_src(node_get_head_edge_dst(node)), node);
			return 1;
		}
	}

	if(node->nb_edge_dst == 2){
		for(edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (ir_imm_operation_get_unsigned_value(operand_operation) == (0xffffffffffffffffULL >> (64 - ir_node_get_operation(node)->size))){
					ir_remove_dependence(ir, edge_cursor);
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_NOT;

					return 1;
				}
			}
		}
	}

	/* merge */
	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		current_edge = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_XOR){
			if (operand_operation->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN || ir_node_get_operation(node)->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN){
				log_warn("unkown instruction dst");
				diff = 0;
			}
			else{
				diff = operand_operation->operation_type.inst.dst - ir_node_get_operation(node)->operation_type.inst.dst;
			}

			if (!diff){
				if (edge_get_src(current_edge)->nb_edge_src == 1){
					graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
					ir_remove_node(ir, edge_get_src(current_edge));

					result = 1;
					continue;
				}
				else if (final){
					if (edge_get_src(current_edge)->nb_edge_dst > 100){
						log_warn_m("uncommon number of dst edge (%u), do not develop", edge_get_src(current_edge)->nb_edge_dst);
					}
					else{
						if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
							log_err("unable to copy dst edge");
						}
						ir_remove_dependence(ir, current_edge);

						result = 1;
						continue;
					}
				}
			}
		}
	}

	return result;
}

/* ===================================================================== */
/* Sorting routines													     */
/* ===================================================================== */

static int32_t irOperand_compare_node(const void* arg1, const void* arg2){
	struct irOperand* operand1 = (struct irOperand*)arg1;
	struct irOperand* operand2 = (struct irOperand*)arg2;

	if (operand1->node > operand2->node){
		return 1;
	}
	else if (operand1->node < operand2->node){
		return -1;
	}
	else{
		return 0;
	}
}
