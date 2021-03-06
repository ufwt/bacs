#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../graph.h"
#include "../graphPrintDot.h"
#include "../base.h"

#define NODE_DESCRIPTION_LENGTH 32
#define EDGE_DESCRIPTION_LENGTH 16

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void asterix_dotPrint_node(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void asterix_dotPrint_edge(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int32_t asterix_copy_node(void* data_dst, const void* data_src, void* arg){
	memcpy(data_dst, data_src, NODE_DESCRIPTION_LENGTH);
	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int32_t asterix_copy_edge(void* data_dst, const void* data_src, void* arg){
	memcpy(data_dst, data_src, EDGE_DESCRIPTION_LENGTH);
	return 0;
}

int main(){
	struct graph* 	graph;
	struct graph* 	clone;
	char 			node_desc[NODE_DESCRIPTION_LENGTH];
	char 			edge_desc[EDGE_DESCRIPTION_LENGTH];
	struct node* 	node_asterix;
	struct node* 	node_obelix;
	struct node* 	node_idefix;
	struct node* 	node_abraracourcix;

	graph = graph_create(NODE_DESCRIPTION_LENGTH, EDGE_DESCRIPTION_LENGTH);
	graph_register_dotPrint_callback(graph, NULL, asterix_dotPrint_node, asterix_dotPrint_edge, NULL)

	/* add nodes */
	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "asterix");
	node_asterix = graph_add_node(graph, &node_desc);
	if (node_asterix == NULL){
		log_err("unable to add node to graph");
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "obelix");
	node_obelix = graph_add_node(graph, &node_desc);
	if (node_obelix == NULL){
		log_err("unable to add node to graph");
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "idefix");
	node_idefix = graph_add_node(graph, &node_desc);
	if (node_idefix == NULL){
		log_err("unable to add node to graph");
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "abraracourcix");
	node_abraracourcix = graph_add_node(graph, &node_desc);
	if (node_abraracourcix == NULL){
		log_err("unable to add node to graph");
	}

	/* add edges */
	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "friend");
	if (graph_add_edge(graph, node_obelix, node_asterix, &edge_desc) == NULL){
		log_err("unable to add edge to graph");
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "obey");
	if (graph_add_edge(graph, node_idefix, node_obelix, &edge_desc) == NULL){
		log_err("unable to add edge to graph");
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "rule");
	if (graph_add_edge(graph, node_abraracourcix, node_obelix, &edge_desc) == NULL){
		log_err("unable to add edge to graph");
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "rule");
	if (graph_add_edge(graph, node_abraracourcix, node_asterix, &edge_desc) == NULL){
		log_err("unable to add edge to graph");
	}

	clone = graph_clone(graph, asterix_copy_node, asterix_copy_edge, NULL);

	graph_delete(graph);

	/* print graph */
	if (graphPrintDot_print(clone, "asterix.dot", NULL)){
		log_err("unable to print graph to dot format");
	}

	graph_delete(clone);

	return 0;
}
