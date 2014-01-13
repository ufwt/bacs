#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "instruction.h"
#include "graphPrintDot.h"
#include "argBuffer.h"
#include "simpleTraceStat.h"
#include "argumentGraph.h"

struct trace* trace_create(const char* dir_name){
	struct trace* 	trace;
	char			file_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	trace->checker = ioChecker_create();
	if (trace->checker == NULL){
		printf("ERROR: in %s, unable to create ioChecker\n", __func__);
		free(trace);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_INS_FILE_NAME);
	if (traceReaderJSON_init(&(trace->ins_reader.json), file_name)){
		printf("ERROR: in %s, unable to init trace JSON reader\n", __func__);
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_CM_FILE_NAME);
	trace->code_map = cmReaderJSON_parse_trace(file_name);
	if (trace->code_map == NULL){
		printf("WARNING: in %s, continue without code map information\n", __func__);
	}

	if (array_init(&(trace->frag_array), sizeof(struct traceFragment))){
		printf("ERROR: in %s, unable to init traceFragment array\n", __func__);
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;
	}

	if (array_init(&(trace->arg_array), sizeof(struct argument))){
		printf("ERROR: in %s, unable to init argument array\n", __func__);
		array_clean(&(trace->frag_array));
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;

	}

	/*trace->call_tree = NULL;*/
	trace->loop_engine = NULL;
	trace->argument_graph = NULL;

	return trace;
}

void trace_delete(struct trace* trace){
	if (trace != NULL){
		if (trace->loop_engine != NULL){
			loopEngine_delete(trace->loop_engine);
		}
		/*if (trace->call_tree != NULL){
			graph_delete(trace->call_tree);
		}*/
		if (trace->argument_graph != NULL){
			graph_delete(trace->argument_graph);
		}

		trace_arg_clean(trace);
		array_clean(&(trace->arg_array));

		trace_frag_clean(trace);
		array_clean(&(trace->frag_array));

		traceReaderJSON_clean(&(trace->ins_reader.json));
		codeMap_delete(trace->code_map);
		ioChecker_delete(trace->checker);

		free(trace);
	}
}

/* ===================================================================== */
/* Trace functions						                                 */
/* ===================================================================== */

void trace_instruction_print(struct trace* trace){
	struct instruction* instruction;

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			instruction = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (instruction != NULL){
				instruction_print(NULL, instruction);
			}
		} while (instruction != NULL);
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

void trace_instruction_export(struct trace* trace){
	struct instruction* 	instruction;
	struct traceFragment 	fragment;

	if (traceFragment_init(&fragment)){
		printf("ERROR: in %s, unable to init traceFragment\n", __func__);
		return;
	}

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			instruction = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (instruction != NULL){
				if (traceFragment_add_instruction(&fragment, instruction) < 0){
					printf("ERROR: in %s, unable to add instruction to traceFragment\n", __func__);
					break;
				}
			}
		} while (instruction != NULL);

		traceFragment_set_tag(&fragment, "trace");

		if (array_add(&(trace->frag_array), &fragment) < 0){
			printf("ERROR: in %s, unable to add traceFragment to frag_array\n", __func__);
			traceFragment_clean(&fragment);
		}
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

/* ===================================================================== */
/* Calltree functions						                             */
/* ===================================================================== */

void trace_callTree_create(struct trace* trace){
	/*struct instruction*			ins;
	struct callTree_element		element;

	if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		printf("WARNING: in %s, deleting the current callTree\n", __func__);
	}
	trace->call_tree = graph_create();
	if (trace->call_tree != NULL){
		trace->call_tree->callback_node.create_data 		= callTree_create_node;
		trace->call_tree->callback_node.may_add_element 	= callTree_may_add_element;
		trace->call_tree->callback_node.add_element 		= callTree_add_element;
		trace->call_tree->callback_node.element_is_owned 	= callTree_element_is_owned;
		trace->call_tree->callback_node.print_dot 			= callTree_node_printDot;
		trace->call_tree->callback_node.delete_data 		= callTree_delete_node;

		element.cm = trace->code_map;
		if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
			do{
				ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
				if (ins != NULL){
					element.ins = ins;
					if (graph_add_element(trace->call_tree, &element)){
						printf("ERROR: in %s, unable to add instruction to call tree\n", __func__);
						break;
					}
				}
			} while (ins != NULL);
		}
		else{
			printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
			graph_delete(trace->call_tree);
			trace->call_tree = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to create graph\n", __func__);
	}*/
}

void trace_callTree_print_dot(struct trace* trace, char* file_name){
	/*if (trace->call_tree != NULL){
		if (file_name != NULL){
			if (graphPrintDot_print(trace->call_tree, file_name)){
				printf("ERROR: in %s, unable to print callTree in DOT format to file: \"%s\"\n", __func__, file_name);
			}
		}
		else{
			printf("ERROR: in %s, please specify a file name\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}*/
}

void trace_callTree_export(struct trace* trace){
	/*int32_t 				i;
	struct callTree_node* 	node;
	struct traceFragment 	fragment;

	if (trace->call_tree != NULL){
		for (i = 0; i < trace->call_tree->nb_node; i++){
			node = (struct callTree_node*)trace->call_tree->nodes[i].data;

			if (traceFragment_clone(&(node->fragment), &fragment)){
				printf("ERROR: in %s, unable to clone traceFragment\n", __func__);
				break;
			}

			traceFragment_set_tag(&fragment, node->name);

			if (array_add(&(trace->frag_array), &fragment) < 0){
				printf("ERROR: in %s, unable to add fragment to frag_array\n", __func__);
				break;
			}
		}

		#ifdef VERBOSE
		printf("CallTree: %d/%d traceFragment(s) have been exported\n", i, trace->call_tree->nb_node);
		#endif
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}*/
}

void trace_callTree_delete(struct trace* trace){
	/*if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		trace->call_tree = NULL;
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}*/
}

/* ===================================================================== */
/* Loop functions						                                 */
/* ===================================================================== */

void trace_loop_create(struct trace* trace){
	struct instruction* ins;

	if (trace->loop_engine != NULL){
		loopEngine_delete(trace->loop_engine);
	}

	trace->loop_engine = loopEngine_create();
	if (trace->loop_engine == NULL){
		printf("ERROR: in %s, unable to init loopEngine\n", __func__);
		return;
	}

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (ins != NULL){
				if (loopEngine_add(trace->loop_engine, ins)){
					printf("ERROR: in %s, loopEngine failed to add element\n", __func__);
					break;
				}
			}
		} while (ins != NULL);
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}

	if (loopEngine_process(trace->loop_engine)){
		printf("ERROR: in %s, unable to process loopEngine\n", __func__);
	}
}

void trace_loop_remove_redundant(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_remove_redundant_loop(trace->loop_engine)){
			printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_pack_epilogue(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_pack_epilogue(trace->loop_engine)){
			printf("ERROR: in %s, unable to pack epilogue\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_print(struct trace* trace){
	if (trace->loop_engine != NULL){
		loopEngine_print_loop(trace->loop_engine);
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_export(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_export_traceFragment(trace->loop_engine, &(trace->frag_array))){
			printf("ERROR: in %s, unable to export loop to traceFragment\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_delete(struct trace* trace){
	if (trace->loop_engine != NULL){
		loopEngine_delete(trace->loop_engine);
		trace->loop_engine = NULL;
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

/* ===================================================================== */
/* frag functions						                                 */
/* ===================================================================== */

void trace_frag_clean(struct trace* trace){
	uint32_t 				i;
	struct traceFragment* 	fragment;

	for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
		fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
		traceFragment_clean(fragment);
	}
	array_empty(&(trace->frag_array));
}

void trace_frag_print_stat(struct trace* trace, char* arg){
	uint32_t 					i;
	struct simpleTraceStat 		stat;
	struct multiColumnPrinter* 	printer = NULL;
	uint32_t 					index;
	struct traceFragment*		fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->frag_array))){
			fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);
			#ifdef VERBOSE
			printf("Print simpleTraceStat for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(trace->frag_array)));
			#endif
			simpleTraceStat_init(&stat);
			simpleTraceStat_process(&stat, fragment);
			simpleTraceStat_print(printer, 0, NULL, &stat);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
		}
	}
	else{
		printer = simpleTraceStat_init_MultiColumnPrinter();
		if (printer != NULL){
			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
				simpleTraceStat_init(&stat);
				simpleTraceStat_process(&stat, fragment);
				simpleTraceStat_print(printer, i, fragment->tag, &stat);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

void trace_frag_print_ins(struct trace* trace, char* arg){
	struct multiColumnPrinter* 	printer;
	struct traceFragment* 		fragment;
	uint32_t 					i;
	uint32_t 					index;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->frag_array))){
			printer = instruction_init_multiColumnPrinter();
			if (printer != NULL){
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);
				#ifdef VERBOSE
				printf("Print instructions for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(trace->frag_array)));
				#endif

				multiColumnPrinter_print_header(printer);

				for (i = 0; i < traceFragment_get_nb_instruction(fragment); i++){
					instruction_print(printer, traceFragment_get_instruction(fragment, i));
				}

				multiColumnPrinter_delete(printer);
			}
			else{
				printf("ERROR: in %s, init multiColumnPrinter fails\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
		}
	}
	else{
		printf("ERROR: in %s, an index value must be specified\n", __func__);
	}
}

void trace_frag_print_percent(struct trace* trace){
	uint32_t 					nb_opcode = 10;
	uint32_t 					opcode[10] = {XED_ICLASS_XOR, XED_ICLASS_SHL, XED_ICLASS_SHLD, XED_ICLASS_SHR, XED_ICLASS_SHRD, XED_ICLASS_NOT, XED_ICLASS_OR, XED_ICLASS_AND, XED_ICLASS_ROL, XED_ICLASS_ROR};
	uint32_t 					nb_excluded_opcode = 3;
	uint32_t 					excluded_opcode[3] = {XED_ICLASS_MOV, XED_ICLASS_PUSH, XED_ICLASS_POP};
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct traceFragment* 		fragment;
	double 						percent;

	#ifdef VERBOSE
	printf("Included opcode(s): ");
	for (i = 0; i < nb_opcode; i++){
		if (i == nb_opcode - 1){
			printf("%s\n", instruction_opcode_2_string(opcode[i]));
		}
		else{
			printf("%s, ", instruction_opcode_2_string(opcode[i]));
		}
	}
	printf("Excluded opcode(s): ");
	for (i = 0; i < nb_excluded_opcode; i++){
		if (i == nb_excluded_opcode - 1){
			printf("%s\n", instruction_opcode_2_string(excluded_opcode[i]));
		}
		else{
			printf("%s, ", instruction_opcode_2_string(excluded_opcode[i]));
		}
	}
	#endif

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, 24);
		multiColumnPrinter_set_column_size(printer, 2, 16);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_DOUBLE);

		multiColumnPrinter_set_title(printer, 0, (char*)"Index");
		multiColumnPrinter_set_title(printer, 1, (char*)"Tag");
		multiColumnPrinter_set_title(printer, 2, (char*)"Percent (%%)");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
			fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
			percent = traceFragment_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			multiColumnPrinter_print(printer, i, fragment->tag, percent*100, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}
}

void trace_frag_set_tag(struct trace* trace, char* arg){
	uint32_t 				i;
	uint32_t 				index;
	struct traceFragment* 	fragment;
	uint8_t 				found_space = 0;

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg);

			if (index < array_get_length(&(trace->frag_array))){
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);

				#ifdef VERBOSE
				printf("Setting tag value for arg %u: old tag: \"%s\", new tag: \"%s\"\n", index, fragment->tag, arg + i + 1);
				#endif

				strncpy(fragment->tag, arg + i + 1, ARGBUFFER_TAG_LENGTH);
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
			}
		}
		else{
			printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
	}
}

void trace_frag_extract_arg(struct trace* trace, char* arg){
	uint32_t 				i;
	struct traceFragment* 	fragment;
	struct argument 		argument;
	uint32_t 				start;
	uint32_t				stop;
	uint32_t 				index;
	uint8_t 				found_space = 0;
	struct array*(*extract_routine_read)(struct memAccess*,int);
	struct array*(*extract_routine_write)(struct memAccess*,int);

	#define ARG_A_DESC 	"arguments are made of adjacent memory access"
	#define ARG_AS_DESC "same as \"A\" with additional access size consideration (Mandatory for fragmentation)"

	start = 0;
	stop = array_get_length(&(trace->frag_array));

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg + i + 1);
		
			if (index < array_get_length(&(trace->frag_array))){
				start = index;
				stop = index + 1;
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
				return;
			}
		}

		if (!strncmp(arg, "A", i)){
			extract_routine_read = traceFragment_extract_mem_arg_adjacent_read;
			extract_routine_write = traceFragment_extract_mem_arg_adjacent_write;
			#ifdef VERBOSE
			printf("Selecting extraction routine \"A\" : %s\n", ARG_A_DESC);
			#endif
		}
		else if (!strncmp(arg, "AS", i)){
			extract_routine_read = traceFragment_extract_mem_arg_adjacent_size_read;
			extract_routine_write = traceFragment_extract_mem_arg_adjacent_size_write;
			#ifdef VERBOSE
			printf("Selecting extraction routine \"AS\" : %s\n", ARG_AS_DESC);
			#endif
		}
		else{
			printf("ERROR: in %s, bad extraction routine specifier of length %u\n", __func__, i);
			goto arg_error;
		}
		
	}
	else{
		printf("ERROR: in %s, an argument is expected to select the extraction routine\n", __func__);
		goto arg_error;
	}

	for (i = start; i < stop; i++){
		fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
		if (traceFragment_create_mem_array(fragment)){
			printf("ERROR: in %s, unable to create mem array for fragment %u\n", __func__, i);
			break;
		}

		/* traceFragment_remove_read_after_write(fragment); */

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0)){
			argument.input = extract_routine_read(fragment->read_memory_array, fragment->nb_memory_read_access);
			argument.output = extract_routine_write(fragment->write_memory_array, fragment->nb_memory_write_access);

			if (argument.input != NULL && argument.output != NULL){
				strncpy(argument.tag, fragment->tag, ARGBUFFER_TAG_LENGTH);
				if (array_add(&(trace->arg_array), &argument) < 0){
					printf("ERROR: in %s, unable to add arguement to arg array\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, read_mem_arg or write_mem_arg array is NULL\n", __func__);
			}
		}
		#ifdef VERBOSE
		else{
			printf("Skipping fragment %u/%u (tag: \"%s\"): no read and write mem access\n", i, array_get_length(&(trace->frag_array)), fragment->tag);
		}
		#endif
	}

	return;

	arg_error:
	printf("Expected extraction specifier:\n");
	printf(" - \"A\"  : %s\n", ARG_A_DESC);
	printf(" - \"AS\" : %s\n", ARG_AS_DESC);
	return;

	#undef ARG_A_DESC
	#undef ARG_AS_DESC
}

/* ===================================================================== */
/* arg functions						                                 */
/* ===================================================================== */

void trace_arg_clean(struct trace* trace){
	uint32_t 				i;
	struct argument* 		argument;

	for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
		argument = (struct argument*)array_get(&(trace->arg_array), i);
		argBuffer_delete_array(argument->input);
		argBuffer_delete_array(argument->output);
	}
	array_empty(&(trace->arg_array));
}

void trace_arg_print(struct trace* trace, char* arg){
	uint32_t 					index;
	uint32_t 					i;
	struct argument* 			argument;
	struct argBuffer* 			buffer;
	struct multiColumnPrinter* 	printer;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			argument = (struct argument*)array_get(&(trace->arg_array), index);
			#ifdef VERBOSE
			printf("Print argument %u (tag: \"%s\", nb argument: %u)\n", index, argument->tag, array_get_length(&(trace->arg_array)));
			#endif

			for (i = 0; i < array_get_length(argument->input); i++){
				buffer = (struct argBuffer*)array_get(argument->input, i);
				printf("Input %u/%u ", i + 1, array_get_length(argument->input));
				argBuffer_print_raw(buffer);
			}

			for (i = 0; i < array_get_length(argument->output); i++){
				buffer = (struct argBuffer*)array_get(argument->output, i);
				printf("Output %u/%u ", i + 1, array_get_length(argument->output));
				argBuffer_print_raw(buffer);
			}
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
		}
	}
	else{
		printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
		if (printer != NULL){

			multiColumnPrinter_set_column_size(printer, 0, 5);
			multiColumnPrinter_set_column_size(printer, 1, 24);

			multiColumnPrinter_set_title(printer, 0, "Index");
			multiColumnPrinter_set_title(printer, 1, "Tag");
			multiColumnPrinter_set_title(printer, 2, "Nb Input");
			multiColumnPrinter_set_title(printer, 3, "Nb Output");

			multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);

			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
				argument = (struct argument*)array_get(&(trace->arg_array), i);
				multiColumnPrinter_print(printer, i, argument->tag, array_get_length(argument->input), array_get_length(argument->output), NULL);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

void trace_arg_set_tag(struct trace* trace, char* arg){
	uint32_t 			i;
	uint32_t 			index;
	struct argument* 	argument;
	uint8_t 			found_space = 0;

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg);

			if (index < array_get_length(&(trace->arg_array))){
				argument = (struct argument*)array_get(&(trace->arg_array), index);

				#ifdef VERBOSE
				printf("Setting tag value for arg %u: old tag: \"%s\", new tag: \"%s\"\n", index, argument->tag, arg + i + 1);
				#endif

				strncpy(argument->tag, arg + i + 1, ARGBUFFER_TAG_LENGTH);
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			}
		}
		else{
			printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
	}
}

void trace_arg_fragment(struct trace* trace, char* arg){
	uint32_t 			start;
	uint32_t			stop;
	uint32_t 			index;
	uint32_t			i;
	struct argument* 	argument;
	struct array 		new_argument_array;

	start = 0;
	stop = array_get_length(&(trace->arg_array));

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			return;
		}
	}

	if (array_init(&new_argument_array, sizeof(struct argument))){
		printf("ERROR: in %s, unable to create array", __func__);
		return;
	}

	for (i = start; i < stop; i++){
		argument = (struct argument*)array_get(&(trace->arg_array), i);

		#ifdef VERBOSE
		printf("Fragmenting arg %u/%u (tag: \"%s\") ...\n", i, array_get_length(&(trace->arg_array)) - 1, argument->tag);
		#endif

		argument_fragment_input(argument, &new_argument_array);
	}

	if (array_copy(&new_argument_array, &(trace->arg_array), 0, array_get_length(&new_argument_array)) != (int32_t)array_get_length(&new_argument_array)){
		printf("ERROR: in %s, unable to copy arrays (may cause memory loss)\n", __func__);
	}

	array_clean(&new_argument_array);
}

void trace_arg_create_argumentGraph(struct trace* trace){
	uint32_t i;

	if (trace->argument_graph != NULL){
		graph_delete(trace->argument_graph);
		printf("WARNING: in %s, deleting the current argumentGraph\n", __func__);
	}
	trace->argument_graph = argumentGraph_create();
	if (trace->argument_graph != NULL){
		for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
			if (argumentGraph_add_argument(trace->argument_graph, (struct argument*)array_get(&(trace->arg_array), i))){
				printf("ERROR: in %s, unable to add argument %u to argumentGraph\n", __func__, i);
				break;
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to create graph\n", __func__);
	}
}

void trace_arg_print_dot_argumentGraph(struct trace* trace, char* file_name){
	if (trace->argument_graph != NULL){
		if (file_name != NULL){
			if (graphPrintDot_print(trace->argument_graph, file_name)){
				printf("ERROR: in %s, unable to print argumentGraph in DOT format to file: \"%s\"\n", __func__, file_name);
			}
		}
		else{
			printf("ERROR: in %s, please specify a file name\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, argumentGraph is NULL\n", __func__);
	}
}

void trace_arg_delete_argumentGraph(struct trace* trace){
	if (trace->argument_graph != NULL){
		graph_delete(trace->argument_graph);
		trace->argument_graph = NULL;
	}
	else{
		printf("ERROR: in %s, argumentGraph is NULL\n", __func__);
	}
}

void trace_arg_search(struct trace* trace, char* arg){
	uint32_t 			i;
	struct argument* 	argument;
	uint32_t 			start;
	uint32_t			stop;
	uint32_t 			index;

	start = 0;
	stop = array_get_length(&(trace->arg_array));

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			return;
		}
	}

	for (i = start; i < stop; i++){
		argument = (struct argument*)array_get(&(trace->arg_array), i);

		#ifdef VERBOSE
		printf("Searching argument %u/%u (tag: \"%s\") ...\n", i, array_get_length(&(trace->arg_array)) - 1, argument->tag);
		#endif

		ioChecker_submit_argBuffers(trace->checker, argument->input, argument->output);
	}
}