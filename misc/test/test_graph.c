#include <stdlib.h>
#include <stdio.h>

#include "../graph.h"
#include "../graphPrintDot.h"

#define NODE_DESCRIPTION_LENGTH 32
#define EDGE_DESCRIPTION_LENGTH 16

#pragma GCC diagnostic ignored "-Wunused-parameter"
void asterix_dotPrint_node(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void asterix_dotPrint_edge(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

int main(){
	struct graph* 	graph;
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
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "obelix");
	node_obelix = graph_add_node(graph, &node_desc);
	if (node_obelix == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "idefix");
	node_idefix = graph_add_node(graph, &node_desc);
	if (node_idefix == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}

	snprintf(node_desc, NODE_DESCRIPTION_LENGTH, "abraracourcix");
	node_abraracourcix = graph_add_node(graph, &node_desc);
	if (node_abraracourcix == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}

	/* add edges */
	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "friend");
	if (graph_add_edge(graph, node_obelix, node_asterix, &edge_desc) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "obey");
	if (graph_add_edge(graph, node_idefix, node_obelix, &edge_desc) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "rule");
	if (graph_add_edge(graph, node_abraracourcix, node_obelix, &edge_desc) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}

	snprintf(edge_desc, EDGE_DESCRIPTION_LENGTH, "rule");
	if (graph_add_edge(graph, node_abraracourcix, node_asterix, &edge_desc) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}

	/* print graph */
	if (graphPrintDot_print(graph, "asterix.dot", NULL)){
		printf("ERROR: in %s, unable to print graph to dot format\n", __func__);
	}

	graph_delete(graph);
	
	return 0;
}