#ifndef GRAPHMINPATH_H
#define GRAPHMINPATH_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define GRAPHMINPATH_INVALID_DST 0xffffffff

enum minPathDirection{
	PATH_SRC_TO_DST,
	PATH_DST_TO_SRC,
	PATH_INVALID
};

#define minPathDirection_invert(dir) ((dir == PATH_SRC_TO_DST) ? PATH_DST_TO_SRC : PATH_SRC_TO_DST)

struct minPathStep{
	struct edge* 			edge;
	enum minPathDirection 	dir;
};

struct minPath{
	struct array* 	step_array; 	/* Must be first because there is a cast to an array in the minCoverage computation */
	struct node* 	reached_node;
};

int32_t graphMinPath_bfs(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct array* path_array, uint64_t(*get_mask)(uint64_t,struct node*,struct edge*,enum minPathDirection));

#define minPath_init(path) ((path)->step_array = array_create(sizeof(struct minPathStep)))
int32_t minPath_check(struct minPath* path);
uint32_t minPath_get_nb_dir_change(struct minPath* path);
#define minPath_clean(path) array_delete((path)->step_array)

#endif
