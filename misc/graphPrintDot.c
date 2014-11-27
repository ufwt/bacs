#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "graphPrintDot.h"

#define graphPrintDot_print_prologue(graph, file, arg) 																\
	if ((graph)->dotPrint_prologue != NULL){  																		\
		(graph)->dotPrint_prologue((file), (arg)); 																	\
	}

#define graphPrintDot_print_node_data(graph, node, file, arg) 														\
	if ((graph)->dotPrint_node_data != NULL){ 																		\
		(graph)->dotPrint_node_data(&((node)->data), (file), (arg)); 												\
	}

#define graphPrintDot_print_edge_data(graph, edge, file, arg) 														\
	if ((graph)->dotPrint_edge_data != NULL){ 																		\
		(graph)->dotPrint_edge_data(&((edge)->data), (file), (arg)); 												\
	}

#define graphPrintDot_print_epilogue(graph, file, arg) 																\
	if ((graph)->dotPrint_epilogue != NULL){  																		\
		(graph)->dotPrint_epilogue((file), (arg)); 																	\
	}

int32_t graphPrintDot_print(struct graph* graph, const char* name, struct graphPrintDotFilter* filters, void* arg){
	FILE* 			file;
	struct node* 	node;
	struct edge* 	edge;
	
	if (graph != NULL){
		if (name == NULL){
			#ifdef VERBOSE
			printf("WARNING: in %s, no file name specified, using default file name: \"%s\"\n", __func__, GRAPHPRINTDOT_DEFAULT_FILE_NAME);
			#endif
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(name, "w");
		}

		if (file != NULL){
			fprintf(file, "digraph G {\n");

			graphPrintDot_print_prologue(graph, file, arg)
			
			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				if (filters == NULL || (filters != NULL && (filters->node_filter == NULL || (filters->node_filter != NULL && filters->node_filter(node, arg))))){
					fprintf(file, "%u ", (uint32_t)node);
					graphPrintDot_print_node_data(graph, node, file, arg)
					fprintf(file, "\n");
				}
			}

			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				for (edge = node_get_head_edge_src(node); edge != NULL; edge = edge_get_next_src(edge)){
					if (filters == NULL || (filters != NULL && (filters->edge_filter == NULL || (filters->edge_filter != NULL && filters->edge_filter(edge, arg))))){
						fprintf(file, "%u -> %u ", (uint32_t)edge->src_node, (uint32_t)edge->dst_node);
						graphPrintDot_print_edge_data(graph, edge, file, arg)
						fprintf(file, "\n");
					}
				}
			}

			graphPrintDot_print_epilogue(graph, file, arg)

			fprintf(file, "}\n");

			fclose(file);
		}
		else{
			printf("ERROR: in %s, unable to open file\n", __func__);
			return -1;
		}
	}

	return 0;
}