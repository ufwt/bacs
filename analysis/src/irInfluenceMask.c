#include <stdlib.h>
#include <stdio.h>

#include "irInfluenceMask.h"
#include "ir.h"
#include "base.h"

#ifdef EXTRA_CHECK
# 	define check_operation(node, opcode_) 																										\
	if (ir_node_get_operation(node)->type != IR_OPERATION_TYPE_INST || ir_node_get_operation(node)->operation_type.inst.opcode != (opcode_)){ 	\
		log_err("incorrect operand"); 																											\
	}
#else
# 	define check_operation(node, opcode_)
#endif

uint64_t irInfluenceMask_operation_and(struct node* node, uint64_t mask){
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	check_operation(node, IR_AND)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operand->type == IR_OPERATION_TYPE_IMM){
			mask &= ir_imm_operation_get_unsigned_value(operand);
		}
	}

	return mask;
}

uint64_t irInfluenceMask_operation_or(struct node* node, uint64_t mask){
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	check_operation(node, IR_OR)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operand->type == IR_OPERATION_TYPE_IMM){
			mask &= ~ir_imm_operation_get_unsigned_value(operand);
		}
	}

	return mask;
}

uint64_t irInfluenceMask_operation_part2_8(struct node* node, uint64_t mask, uint32_t dir){
	check_operation(node, IR_PART2_8)

	if (dir == DIR_SRC_TO_DST){
		return (mask >> 8) & bitmask64(ir_node_get_operation(node)->size);
	}
	else{
		return mask << 8;
	}
}

uint64_t irInfluenceMask_operation_rol(struct node* node, uint64_t mask, uint32_t dir){
	uint32_t 			disp = 0;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operand = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand->type == IR_OPERATION_TYPE_IMM){
				disp = (uint32_t)ir_imm_operation_get_unsigned_value(operand);
			}
			break;
		}
	}

	if (dir == DIR_SRC_TO_DST){
		return ((mask << disp) & bitmask64(ir_node_get_operation(node)->size)) | (mask >> (ir_node_get_operation(node)->size - disp));
	}
	else{
		return (mask >> disp) | ((mask << (ir_node_get_operation(node)->size - disp)) & bitmask64(ir_node_get_operation(node)->size));
	}
}

uint64_t irInfluenceMask_operation_shl(struct node* node, uint64_t mask, uint32_t dir){
	uint32_t 			disp = 0;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operand = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand->type == IR_OPERATION_TYPE_IMM){
				disp = (uint32_t)ir_imm_operation_get_unsigned_value(operand);
			}
			break;
		}
	}

	if (dir == DIR_SRC_TO_DST){
		return (mask << disp) & bitmask64(ir_node_get_operation(node)->size);
	}
	else{
		return mask >> disp;
	}
}
