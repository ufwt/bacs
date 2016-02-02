#ifndef ASSEMBLYSCAN_H
#define ASSEMBLYSCAN_H

#include "assembly.h"

#define ASSEMBLYSCAN_FILTER_BBL_SIZE 	0x00000001
#define ASSEMBLYSCAN_FILTER_BBL_RATIO 	0x00000002
#define ASSEMBLYSCAN_FILTER_BBL_EXEC 	0x00000004
#define ASSEMBLYSCAN_FILTER_FUNC_LEAF 	0x00000008

void assemblyScan_scan(const struct assembly* assembly, void* call_graph, uint32_t filters);

#endif