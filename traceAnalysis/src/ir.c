#include <stdlib.h>
#include <stdio.h>

#include "ir.h"
#include "irImporterDynTrace.h"


void ir_dotPrint_node(void* data, FILE* file);
void ir_dotPrint_edge(void* data, FILE* file);

struct ir* ir_create(struct trace* trace){
	struct ir* ir;

	ir =(struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init(ir, trace)){
			printf("ERROR: in %s, unable to init ir\n", __func__);
			free(ir);
			ir = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return ir;
}

int32_t ir_init(struct ir* ir, struct trace* trace){
	ir->trace 				= trace;
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))
	graph_register_dotPrint_callback(&(ir->graph), ir_dotPrint_node, ir_dotPrint_edge)
	ir->input_linkedList 	= NULL;
	ir->output_linkedList 	= NULL;

	if (irImporterDynTrace_import(ir)){
		printf("ERROR: in %s, trace import has failed\n", __func__);
		return -1;
	}
	
	return 0;
}

struct node* ir_add_input(struct ir* ir, struct operand* operand){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_INPUT;
		operation->operation_type.input.operand 	= operand;
		operation->operation_type.input.prev 		= NULL;
		operation->operation_type.input.next 		= ir->input_linkedList;

		if (ir->input_linkedList != NULL){
			ir_node_get_operation(ir->input_linkedList)->operation_type.input.prev = node;
		}

		ir->input_linkedList = node;
	}

	return node;
}

struct node* ir_add_output(struct ir* ir, enum irOpcode opcode){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_OUTPUT;
		operation->operation_type.output.opcode 	= opcode;
		operation->operation_type.output.prev 		= NULL;
		operation->operation_type.output.next 		= ir->output_linkedList;

		if (ir->output_linkedList != NULL){
			ir_node_get_operation(ir->output_linkedList)->operation_type.output.prev = node;
		}

		ir->output_linkedList = node;
	}

	return node;
}

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), operation_src, operation_dst);
	if (edge == NULL){
		printf("ERROR: in %s, unable to add edge to the graph\n", __func__);
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type = type;
	}

	return edge;
}

void ir_operation_set_inner(struct ir* ir, struct node* node){
	enum irOpcode 			opcode;
	struct irOperation* 	operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_OUTPUT){
		opcode = operation->operation_type.output.opcode;
		operation->type = IR_OPERATION_TYPE_INNER;
		operation->operation_type.inner.opcode = opcode;

		if (operation->operation_type.output.prev == NULL){
			ir->output_linkedList = operation->operation_type.output.next;
		}
		else{
			ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
		}

		if (operation->operation_type.output.next != NULL){
			ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
		}
	}
}


/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

void ir_dotPrint_node(void* data, FILE* file){
	struct irOperation* operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT 		: {
			if (OPERAND_IS_MEM(*(operation->operation_type.input.operand))){
				#if defined ARCH_32
				fprintf(file, "[label=\"@%08x\"]", operation->operation_type.input.operand->location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				fprintf(file, "[label=\"@%llx\"]", operation->operation_type.input.operand->location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation->operation_type.input.operand))){
				fprintf(file, "[label=\"%s\"]", reg_2_string(operation->operation_type.input.operand->location.reg));
			}
			else{
				printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
				break;
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT 		: {
			fprintf(file, "[label=\"O:%s\"]", irOpcode_2_string(operation->operation_type.output.opcode));
			break;
		}
		case IR_OPERATION_TYPE_INNER 		: {
			fprintf(file, "[label=\"%s\"]", irOpcode_2_string(operation->operation_type.inner.opcode));
			break;
		}
	}
}

void ir_dotPrint_edge(void* data, FILE* file){
	struct irDependence* dependence = (struct irDependence*)data;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 	: {
			break;
		}
		case IR_DEPENDENCE_TYPE_BASE 	: {
			fprintf(file, "[label=\"base\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_INDEX 	: {
			fprintf(file, "[label=\"index\"]");
			break;
		}
	}
}

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_MOVZX 	: {return "movzx";}
		case IR_SHR 	: {return "shr";}
		case IR_XOR 	: {return "xor";}
	}

	return NULL;
}