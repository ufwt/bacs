#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VERBOSE
#include <time.h>
#endif

#include "irNormalize.h"

#ifdef VERBOSE
#include "multiColumn.h"
#endif

#define IR_NORMALIZE_TRANSLATE_ROL_IMM 				1
#define IR_NORMALIZE_TRANSLATE_SUB_IMM 				1
#define IR_NORMALIZE_TRANSLATE_XOR_FF 				1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_ADD 			1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR 			1
#define IR_NORMALIZE_PROPAGATE_EXPRESSION 			1
#define IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS 		1
#define IR_NORMALIZE_DETECT_ROTATION 				1
#define IR_NORMALIZE_DETECT_FIRST_BYTE_EXTRACT 		1
#define IR_NORMALIZE_DETECT_SECOND_BYTE_EXTRACT 	1

static int32_t irNormalize_sort_node(struct ir* ir);
static void irNormalize_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator);

#ifdef VERBOSE

#define INIT_TIMER																																			\
	struct timespec 			timer_start_time; 																											\
	struct timespec 			timer_stop_time; 																											\
	double 						timer_elapsed_time; 																										\
	struct multiColumnPrinter* 	printer; 																													\
																																							\
	printer = multiColumnPrinter_create(stdout, 2, NULL, NULL, NULL); 																						\
	if (printer != NULL){ 																																	\
		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_DOUBLE); 																			\
		multiColumnPrinter_set_column_size(printer, 0, 32); 																								\
		multiColumnPrinter_set_title(printer, 0, "RULE"); 																									\
		multiColumnPrinter_set_title(printer, 1, "TIME"); 																									\
																																							\
		multiColumnPrinter_print_header(printer); 																											\
	} 																																						\
	else{ 																																					\
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__); 																				\
		return; 																																			\
	}

#define START_TIMER 																																		\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){ 																						\
		printf("ERROR: in %s, clock_gettime fails\n", __func__); 																							\
	}

#define STOP_TIMER 																																			\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){ 																						\
		printf("ERROR: in %s, clock_gettime fails\n", __func__); 																							\
	}

#define PRINT_TIMER(ctx_string) 																															\
	timer_elapsed_time = (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.; 			\
	multiColumnPrinter_print(printer, (ctx_string), timer_elapsed_time, NULL);

#define CLEAN_TIMER 																																		\
		multiColumnPrinter_delete(printer);

#else

#define INIT_TIMER
#define START_TIMER
#define STOP_TIMER
#define PRINT_TIMER(ctx_string)
#define CLEAN_TIMER

#endif

void ir_normalize(struct ir* ir){
	INIT_TIMER

	#if IR_NORMALIZE_TRANSLATE_ROL_IMM == 1
	{
		START_TIMER
		ir_normalize_translate_rol_imm(ir);
		STOP_TIMER
		PRINT_TIMER("Translate rol imm")
	}
	#endif

	#if IR_NORMALIZE_TRANSLATE_SUB_IMM == 1
	{
		START_TIMER
		ir_normalize_translate_sub_imm(ir);
		STOP_TIMER
		PRINT_TIMER("Translate sub imm")
	}
	#endif

	#if IR_NORMALIZE_TRANSLATE_XOR_FF == 1
	{
		START_TIMER
		ir_normalize_translate_xor_ff(ir);
		STOP_TIMER
		PRINT_TIMER("Translate xor ff")
	}
	#endif
	
	#if IR_NORMALIZE_PROPAGATE_EXPRESSION == 1 && IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
	{
		uint8_t modification;
		#ifdef VERBOSE
		double timer_1_elapsed_time = 0.0;
		double timer_2_elapsed_time = 0.0;
		#endif

		START_TIMER
		ir_normalize_propagate_expression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
		#endif

		START_TIMER
		ir_normalize_simplify_memory_access(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
		#endif

		while(modification){
			START_TIMER
			ir_normalize_propagate_expression(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif

			if (!modification){
				continue;
			}

			START_TIMER
			ir_normalize_simplify_memory_access(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
		}

		#ifdef VERBOSE
		multiColumnPrinter_print(printer, "Propagate expression", timer_1_elapsed_time, NULL);
		multiColumnPrinter_print(printer, "Simplify memory access", timer_2_elapsed_time, NULL);
		#endif
	}
	#elif IR_NORMALIZE_PROPAGATE_EXPRESSION == 1
	{
		uint8_t modification;
		START_TIMER
		ir_normalize_propagate_expression(ir, &modification);
		STOP_TIMER
		PRINT_TIMER("Propagate expression")
	}
	#elif IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
	{
		uint8_t modification;
		START_TIMER
		ir_normalize_simplify_memory_access(ir, &modification);
		STOP_TIMER
		PRINT_TIMER("Simplify memory access")
	}
	#endif

	#if IR_NORMALIZE_MERGE_ASSOCIATIVE_ADD == 1
	{
		START_TIMER
		ir_normalize_merge_associative_operation(ir, IR_ADD);
		STOP_TIMER
		PRINT_TIMER("Merge transitive add")
	}
	#endif
	
	#if IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR == 1
	{
		START_TIMER
		ir_normalize_merge_associative_operation(ir, IR_XOR);
		STOP_TIMER
		PRINT_TIMER("Merge transitive xor")
	}
	#endif

	#if IR_NORMALIZE_DETECT_ROTATION == 1
	{
		START_TIMER
		ir_normalize_detect_rotation(ir);
		STOP_TIMER
		PRINT_TIMER("Detect rotation")
	}
	#endif

	#if IR_NORMALIZE_DETECT_FIRST_BYTE_EXTRACT == 1
	{
		START_TIMER
		ir_normalize_detect_first_byte_extract(ir);
		STOP_TIMER
		PRINT_TIMER("Detect first byte extraction")
	}
	#endif

	#if IR_NORMALIZE_DETECT_SECOND_BYTE_EXTRACT == 1
	{
		START_TIMER
		ir_normalize_detect_second_byte_extract(ir);
		STOP_TIMER
		PRINT_TIMER("Detect second byte extraction")
	}
	#endif

	CLEAN_TIMER
}

void ir_normalize_translate_rol_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST){
			if (operation->operation_type.inst.opcode == IR_ROL){
				operation->operation_type.inst.opcode = IR_ROR;

				if (node_cursor->nb_edge_dst == 2){
					edge_cursor = node_get_head_edge_dst(node_cursor);
					while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
						edge_cursor = edge_get_next_dst(edge_cursor);
					}
					if (edge_cursor != NULL){
						imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
						#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
						imm_value->operation_type.imm.value = operation->size - ir_imm_operation_get_unsigned_value(imm_value);
					}
					else{
						printf("WARNING: in %s, this case (ROL with 2 operands but no IMM) is not supposed to happen\n", __func__);
					}
				}
				else{
					printf("WARNING: in %s, this case (ROL with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
				}
			}
		}
	}
}

/* WARNING this routine might be incorrect (see below). Furthermore it is imprecise since both arguments are not used in the smae way */
void ir_normalize_translate_sub_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_SUB){
			if (node_cursor->nb_edge_dst == 2){
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					imm_value->operation_type.imm.value = (uint64_t)(-imm_value->operation_type.imm.value); /* I am not sure if this is correct */
					operation->operation_type.inst.opcode = IR_ADD;
				}
			}
			else {
				printf("WARNING: in %s, this case (SUB with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
			}
		}
	}
}

void ir_normalize_translate_xor_ff(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst == 2){
			operation = ir_node_get_operation(node_cursor);

			if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_XOR){
				for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					imm = ir_node_get_operation(edge_get_src(edge_cursor));
					if (imm->type == IR_OPERATION_TYPE_IMM){
						#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
						if (ir_imm_operation_get_unsigned_value(imm) == (0xffffffffffffffffULL >> (64 - operation->size))){
							if (edge_get_src(edge_cursor)->nb_edge_src == 1){
								ir_remove_node(ir, edge_get_src(edge_cursor));
							}
							operation->operation_type.inst.opcode = IR_NOT;
							break;
						}
					}
				}
			}
		}
	}
}

void ir_normalize_propagate_expression(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	uint32_t 				i;
	uint8_t* 				taken;
	uint32_t 				max_nb_edge_dst = 0;
	uint32_t 				nb_match;
	struct node*			next_node_cursor2;

	*modification = 0;
	
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		if (node_cursor1->nb_edge_dst > max_nb_edge_dst){
			max_nb_edge_dst = node_cursor1->nb_edge_dst;
		}
	}

	if (max_nb_edge_dst == 0){
		return;
	}

	if (irNormalize_sort_node(ir)){
		printf("ERROR: in %s, unable to sort ir node(s)\n", __func__);
		return;
	}

	taken = (uint8_t*)alloca(sizeof(uint8_t) * max_nb_edge_dst);

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation1 = ir_node_get_operation(node_cursor1);
		if (operation1->type == IR_OPERATION_TYPE_INST){
			for(node_cursor2 = node_get_next(node_cursor1); node_cursor2 != NULL; node_cursor2 = next_node_cursor2){
				next_node_cursor2 = node_get_next(node_cursor2);

				operation2 = ir_node_get_operation(node_cursor2);
				if (operation2->type == IR_OPERATION_TYPE_INST){

					if (node_cursor1->nb_edge_dst == node_cursor2->nb_edge_dst && irOperation_equal(operation1, operation2)){
						memset(taken, 0, sizeof(uint8_t) * node_cursor1->nb_edge_dst);

						for (edge_cursor1 = node_get_head_edge_dst(node_cursor1); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
							for (edge_cursor2 = node_get_head_edge_dst(node_cursor2), i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
								if (!taken[i]){
									if (edge_get_src(edge_cursor1) == edge_get_src(edge_cursor2)){
										taken[i] = 1;
										break;
									}
									else if (ir_node_get_operation(edge_get_src(edge_cursor1))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
										if (irOperation_equal(ir_node_get_operation(edge_get_src(edge_cursor1)), ir_node_get_operation(edge_get_src(edge_cursor2)))){
											taken[i] = 1;
											break;
										}
									}
								}
							}

							if (edge_cursor2 == NULL){
								break;
							}
						}

						for (i = 0, nb_match = 0; i < node_cursor1->nb_edge_dst; i++){
							nb_match += taken[i];
						}

						if (nb_match == node_cursor1->nb_edge_dst){
							graph_transfert_src_edge(&(ir->graph), node_cursor1, node_cursor2);
							ir_remove_node(ir, node_cursor2);
							*modification = 1;
						}
					}
				}
			}
		}
	}
}

struct memoryAccessList{
	struct node* 	address;
	struct node* 	node;
};

int32_t compare_address_memoryAccessList(const void* arg1, const void* arg2){
	struct memoryAccessList* access1 = (struct memoryAccessList*)arg1;
	struct memoryAccessList* access2 = (struct memoryAccessList*)arg2;

	if (access1->address < access2->address){
		return -1;
	}
	else if (access1->address > access2->address){
		return 1;
	}
	else{
		return 0;
	}
}

int32_t compare_order_memoryAccessList(const void* arg1, const void* arg2){
	struct memoryAccessList* access1 = (struct memoryAccessList*)arg1;
	struct memoryAccessList* access2 = (struct memoryAccessList*)arg2;
	struct irOperation* op1 = ir_node_get_operation(access1->node);
	struct irOperation* op2 = ir_node_get_operation(access2->node);
	uint32_t order1;
	uint32_t order2;

	if (op1->type == IR_OPERATION_TYPE_IN_MEM){
		order1 = op1->operation_type.in_mem.order;
	}
	else{
		order1 = op1->operation_type.out_mem.order;	
	}

	if (op2->type == IR_OPERATION_TYPE_IN_MEM){
		order2 = op2->operation_type.in_mem.order;
	}
	else{
		order2 = op2->operation_type.out_mem.order;	
	}

	if (order1 < order2){
		return -1;
	}
	else if (order1 > order2){
		return 1;
	}
	else{
		return 0;
	}
}

struct memoryAddressList{
	uint32_t 			offset;
	uint32_t 			length;
};

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else{
		printf("ERROR: in %s, this case is not implemented (src=%u, dst=%u)\n", __func__, size_src, size_dst);
		return IR_PART1_8;
	}
}

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification){
	uint32_t 					nb_mem_access;
	uint32_t 					nb_address;
	struct node* 				node_cursor;
	struct edge* 				edge_cursor;
	struct memoryAccessList* 	access_list;
	struct memoryAddressList* 	address_list;
	uint32_t 					i;
	uint32_t 					j;

	*modification = 0;

	for(node_cursor = graph_get_head_node(&(ir->graph)), nb_mem_access = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_OUT_MEM){
			nb_mem_access ++;
		}
	}

	if (nb_mem_access == 0){
		return;
	}

	access_list = (struct memoryAccessList*)malloc(sizeof(struct memoryAccessList) * nb_mem_access);
	if (access_list == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	for(node_cursor = graph_get_head_node(&(ir->graph)), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_OUT_MEM){
			access_list[i].address = NULL;
			for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
					access_list[i].address = edge_get_src(edge_cursor);
				}
			}

			access_list[i].node = node_cursor;
			i ++;
		}
	}

	qsort(access_list, nb_mem_access, sizeof(struct memoryAccessList), compare_address_memoryAccessList);

	for (i = 1, nb_address = 1; i < nb_mem_access; i++){
		if (access_list[i].address != access_list[i - 1].address){
			nb_address ++;
		}
	}

	address_list = (struct memoryAddressList*)malloc(sizeof(struct memoryAddressList) * nb_address);
	if (address_list == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(access_list);
		return;
	}

	address_list[0].offset = 0;
	for (i = 1, j = 1; i < nb_mem_access; i++){
		if (access_list[i].address != access_list[i - 1].address){
			address_list[j - 1].length = i - address_list[j - 1].offset;
			address_list[j].offset = i;
			qsort(access_list + address_list[j - 1].offset, address_list[j - 1].length, sizeof(struct memoryAccessList), compare_order_memoryAccessList);
			j ++;
		}
	}
	address_list[nb_address - 1].length = nb_mem_access - address_list[nb_address - 1].offset;
	qsort(access_list + address_list[nb_address - 1].offset, address_list[nb_address - 1].length, sizeof(struct memoryAccessList), compare_order_memoryAccessList);

	for (i = 0; i < nb_address; i++){
		for (j = 1; j < address_list[i].length; j++){
			struct memoryAccessList* 	access_prev;
			struct memoryAccessList* 	access_next;
			struct irOperation* 		operation_prev;
			struct irOperation* 		operation_next;

			access_prev = access_list + (address_list[i].offset + j - 1);
			access_next = access_list + (address_list[i].offset + j);

			operation_prev = ir_node_get_operation(access_prev->node);
			operation_next = ir_node_get_operation(access_next->node);

			/* STORE -> LOAD */
			if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){
				struct node* stored_value = NULL;

				for (edge_cursor = node_get_head_edge_dst(access_prev->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
						stored_value = edge_get_src(edge_cursor);
					}
				}

				if (stored_value != NULL){
					if (operation_prev->size > operation_next->size){
						ir_convert_node_to_inst(access_next->node, ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
						graph_remove_edge(&(ir->graph), graph_get_edge(access_next->address, access_next->node));

						if (ir_add_dependence(ir, stored_value, access_next->node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
						}

						memcpy(access_next, access_prev, sizeof(struct memoryAccessList));
					}
					else if (operation_prev->size < operation_next->size){
						printf("WARNING: in %s, simplification of memory access of different size (case STORE -> LOAD)\n", __func__);
					}
					else{
						graph_transfert_src_edge(&(ir->graph), stored_value, access_next->node);
						ir_remove_node(ir, access_next->node);

						memcpy(access_next, access_prev, sizeof(struct memoryAccessList));
					}
					*modification = 1;
					continue;
				}
				else{
					printf("ERROR: in %s, incorrect memory access pattern in STORE -> LOAD\n", __func__);
				}
			}

			/* LOAD -> LOAD */
			if (operation_prev->type == IR_OPERATION_TYPE_IN_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){
				if (operation_prev->size > operation_next->size){
					ir_convert_node_to_inst(access_next->node, ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
					graph_remove_edge(&(ir->graph), graph_get_edge(access_next->address, access_next->node));

					if (ir_add_dependence(ir, access_prev->node, access_next->node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
					}

					memcpy(access_next, access_prev, sizeof(struct memoryAccessList));
				}
				else if (operation_prev->size < operation_next->size){
					ir_convert_node_to_inst(access_prev->node, ir_normalize_choose_part_opcode(operation_next->size, operation_prev->size), operation_prev->size)
					graph_remove_edge(&(ir->graph), graph_get_edge(access_prev->address, access_prev->node));

					if (ir_add_dependence(ir, access_next->node, access_prev->node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
					}
				}
				else{
					graph_transfert_src_edge(&(ir->graph), access_prev->node, access_next->node);
					ir_remove_node(ir, access_next->node);

					memcpy(access_next, access_prev, sizeof(struct memoryAccessList));
				}
				*modification = 1;
				continue;
			}

			/* STORE -> STORE */
			if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_OUT_MEM){
				if (operation_prev->size > operation_next->size){
					printf("WARNING: in %s, simplification of memory access of different size (case STORE (%u bits) -> STORE (%u bits))\n", __func__, operation_prev->size, operation_next->size);
				}

				ir_remove_node(ir, access_prev->node);
				continue;
			}
		}
	}

	free(address_list);
	free(access_list);
}

void ir_normalize_detect_rotation(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		or_dep1;
	struct node* 		or_dep2;
	struct node* 		node_shiftr;
	struct node* 		node_shiftl;
	struct node* 		shiftr_dep1;
	struct node* 		shiftr_dep2;
	struct node* 		shiftl_dep1;
	struct node* 		shiftl_dep2;
	struct node* 		node_immr;
	struct node* 		node_imml;
	struct node* 		node_common;
	struct irOperation* operation;
	enum irOpcode* 		opcode_ptr;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (node_cursor->nb_edge_dst == 2 && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_OR){
			opcode_ptr 	= &(operation->operation_type.inst.opcode);
			or_dep1 	= edge_get_src(node_get_head_edge_dst(node_cursor));
			or_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_cursor)));
			node_shiftl = NULL;
			node_shiftr = NULL;

			operation = ir_node_get_operation(or_dep1);
			if (operation->type == IR_OPERATION_TYPE_INST){
				if (operation->operation_type.inst.opcode == IR_SHR){
					node_shiftr = or_dep1;
				}
				else if (operation->operation_type.inst.opcode == IR_SHL){
					node_shiftl = or_dep1;
				}
			}

			operation = ir_node_get_operation(or_dep2);
			if (operation->type == IR_OPERATION_TYPE_INST){
				if (operation->operation_type.inst.opcode == IR_SHR){
					node_shiftr = or_dep2;
				}
				else if (operation->operation_type.inst.opcode == IR_SHL){
					node_shiftl = or_dep2;
				}
			}

			if (node_shiftl != NULL && node_shiftr != NULL){
				if (node_shiftl->nb_edge_dst == 2 && node_shiftr->nb_edge_dst == 2){

					shiftr_dep1 	= edge_get_src(node_get_head_edge_dst(node_shiftr));
					shiftr_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_shiftr)));
					shiftl_dep1 	= edge_get_src(node_get_head_edge_dst(node_shiftl));
					shiftl_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_shiftl)));
					node_immr 		= NULL;
					node_imml 		= NULL;
					node_common 	= NULL;

					if (shiftl_dep1 == shiftr_dep1 && shiftl_dep2 != shiftr_dep2){
						if (ir_node_get_operation(shiftl_dep2)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep2)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep2;
							node_imml = shiftl_dep2;
							node_common = shiftl_dep1;
						}
					}
					else if (shiftl_dep1 == shiftr_dep2 && shiftl_dep2 != shiftr_dep1){
						if (ir_node_get_operation(shiftl_dep2)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep1)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep1;
							node_imml = shiftl_dep2;
							node_common = shiftl_dep1;
						}
					}
					else if (shiftl_dep2 == shiftr_dep2 && shiftl_dep1 != shiftr_dep1){
						if (ir_node_get_operation(shiftl_dep1)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep1)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep1;
							node_imml = shiftl_dep1;
							node_common = shiftl_dep2;
						}
					}

					if (node_immr != NULL && node_imml != NULL && node_common != NULL){
						#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
						if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_immr)) + ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_imml)) == ir_node_get_operation(node_cursor)->size){

							*opcode_ptr = IR_ROR;
							if (ir_add_dependence(ir, node_common, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependence to the IR\n", __func__);
							}

							if (ir_add_dependence(ir, node_immr, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependence to the IR\n", __func__);
							}

							if (node_shiftl->nb_edge_src == 1){
								ir_remove_node(ir, node_shiftl);
							}
							else{
								graph_remove_edge(&(ir->graph), graph_get_edge(node_shiftl, node_cursor));
							}

							if (node_shiftr->nb_edge_src == 1){
								ir_remove_node(ir, node_shiftr);
							}
							else{
								graph_remove_edge(&(ir->graph), graph_get_edge(node_shiftr, node_cursor));
							}
						}
					}
				}
			}
		}
	}
}

void ir_normalize_merge_associative_operation(struct ir* ir, enum irOpcode opcode){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct node* 			merge_node;
	struct irOperation* 	operation;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == opcode){

			start:
			for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				merge_node = edge_get_src(edge_cursor);
				operation = ir_node_get_operation(merge_node);
				if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == opcode){
					if (merge_node->nb_edge_src == 1){
						graph_merge_node(&(ir->graph), node_cursor, merge_node);
						ir_remove_node(ir, merge_node);
						goto start;
					}
					/* Here starts the swamp of doom and death */
					else if (opcode == IR_XOR){
						graph_remove_edge(&(ir->graph), edge_cursor);
						for(edge_cursor = node_get_head_edge_dst(merge_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							if (graph_add_edge(&(ir->graph), edge_get_src(edge_cursor), node_cursor, &(edge_cursor->data)) == NULL){
								printf("ERROR: in %s, unable to add edge to IR\n", __func__);
							}
						}
						goto start;
					}
					else if (opcode == IR_ADD && merge_node->nb_edge_src == 2){
						struct edge* edge_cursor123;
						struct node* node_cursor123;

						for(edge_cursor123 = node_get_head_edge_src(merge_node); edge_cursor123 != NULL; edge_cursor123 = edge_get_next_src(edge_cursor123)){
							node_cursor123 = edge_get_dst(edge_cursor123);

							if (ir_node_get_operation(node_cursor123)->type == IR_OPERATION_TYPE_OUT_MEM){
								graph_remove_edge(&(ir->graph), edge_cursor);
								for(edge_cursor = node_get_head_edge_dst(merge_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
									if (graph_add_edge(&(ir->graph), edge_get_src(edge_cursor), node_cursor, &(edge_cursor->data)) == NULL){
										printf("ERROR: in %s, unable to add edge to IR\n", __func__);
									}
								}
								goto start;
							}
						}
					}
				}
			}
		}
	}
}

void ir_normalize_detect_first_byte_extract(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		node_child;
	struct node* 		node_imm_new;
	struct irOperation* operation;
	enum irOpcode* 		opcode_ptr;
	uint8_t 			size;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src == 1 && node_cursor->nb_edge_dst == 1){
			operation = ir_node_get_operation(node_cursor);
			if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_PART1_8){
				opcode_ptr = &(operation->operation_type.inst.opcode);
				node_child = edge_get_dst(node_get_head_edge_src(node_cursor));

				if (node_child->nb_edge_dst == 1){
					operation = ir_node_get_operation(node_child);
					size = operation->size;
					if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_MOVZX){
						*opcode_ptr = IR_AND;
						graph_transfert_src_edge(&(ir->graph), node_cursor, node_child);
						ir_remove_node(ir, node_child);

						node_imm_new = ir_add_immediate(ir, size, 0, 0x000000ff);
						if (node_imm_new != NULL){
							if (ir_add_dependence(ir, node_imm_new, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add edge to IR\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}
					}
				}
			}
		}
	}
}

void ir_normalize_detect_second_byte_extract(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		node_child;
	struct node* 		node_imm_new;
	struct irOperation* operation;
	enum irOpcode* 		opcode_ptr_cursor;
	uint8_t 			size;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src == 1 && node_cursor->nb_edge_dst == 1){
			operation = ir_node_get_operation(node_cursor);
			if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_PART2_8){
				opcode_ptr_cursor = &(operation->operation_type.inst.opcode);
				node_child = edge_get_dst(node_get_head_edge_src(node_cursor));

				if (node_child->nb_edge_dst == 1){
					operation = ir_node_get_operation(node_child);
					size = operation->size;
					if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_MOVZX){
						operation->operation_type.inst.opcode = IR_AND;

						node_imm_new = ir_add_immediate(ir, size, 0, 0x000000ff);
						if (node_imm_new != NULL){
							if (ir_add_dependence(ir, node_imm_new, node_child, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add edge to IR\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}

						*opcode_ptr_cursor = IR_SHR;

						node_imm_new = ir_add_immediate(ir, size, 0, size - 8);
						if (node_imm_new != NULL){
							if (ir_add_dependence(ir, node_imm_new, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add edge to IR\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}
					}
				}
			}
		}
	}
}

static int32_t irNormalize_sort_node(struct ir* ir){
	struct node** 	node_buffer;
	uint32_t 		i;
	struct node* 	primary_cursor;
	uint32_t 		generator = 1;

	node_buffer = (struct node**)malloc(ir->graph.nb_node * sizeof(struct node*));
	if (node_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for(primary_cursor = graph_get_head_node(&(ir->graph)); primary_cursor != NULL; primary_cursor = node_get_next(primary_cursor)){
		ir_node_get_operation(primary_cursor)->data = 0;
	}

	for(primary_cursor = graph_get_head_node(&(ir->graph)); primary_cursor != NULL; primary_cursor = node_get_next(primary_cursor)){
		if (ir_node_get_operation(primary_cursor)->data == 0){
			irNormalize_recursive_sort(node_buffer, primary_cursor, &generator);
		}
	}

	ir->graph.node_linkedList = node_buffer[0];
	node_buffer[0]->prev = NULL;
	if (ir->graph.nb_node > 1){
		node_buffer[0]->next = node_buffer[1];

		for (i = 1; i < ir->graph.nb_node - 1; i++){
			node_buffer[i]->prev = node_buffer[i - 1];
			node_buffer[i]->next = node_buffer[i + 1];
		}
			
		node_buffer[i]->prev = node_buffer[i - 1];
		node_buffer[i]->next = NULL;
	}
	else{
		node_buffer[0]->next = NULL;
	}

	free(node_buffer);

	return 0;
}

static void irNormalize_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator){
	struct edge* edge_cursor;
	struct node* parent_node;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		parent_node = edge_get_src(edge_cursor);

		if (ir_node_get_operation(parent_node)->data == 0){
			irNormalize_recursive_sort(node_buffer, parent_node, generator);
		}
	}

	ir_node_get_operation(node)->data = *generator;
	node_buffer[*generator - 1] = node;
	*generator = *generator + 1;
}