#ifndef SUBGRAPHISOMORPHISM_H
#define SUBGRAPHISOMORPHISM_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define SUBGRAPHISOMORPHISM_JOKER_LABEL 0xffffffff

#define SUBGRAPHISOMORPHISM_OPTIM_FAST_ERROR 	1
#define SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY 	1

struct nodeTab{
	uint32_t 				label;
	uint32_t 				connectivity;
	struct node*			node;
};

struct labelTab{
	uint32_t 				label;
	struct node* 			node;
};

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
struct connectivityTab{
	uint32_t 				connectivity;
	struct node*			node;
};
#endif

struct labelFastAccess{
	uint32_t 				label;
	uint32_t 				offset;
	uint32_t 				size;
};

struct graphIsoHandle{
	struct graph* 			graph;
	uint32_t 				nb_label;
	struct labelFastAccess* label_fast;
	struct labelTab*		label_tab;
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	struct connectivityTab* connectivity_tab;
	#endif
	uint32_t(*edge_get_label)(struct edge*);
};

struct subGraphIsoHandle{
	struct graph* 			graph;
	struct nodeTab* 		node_tab;
	uint32_t(*edge_get_label)(struct edge*);
};

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*));
struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*));

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle);

void graphIso_delete_graph_handle(struct graphIsoHandle* handle);
void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle);

#endif
