#include <stdlib.h>
#include <stdio.h>

#include "../inputParser.h"
#include "../base.h"

static void print(uint32_t* ctx, char* string);
static void print_hello(uint32_t* ctx);

int main(int argc, char** argv){
	struct inputParser 	parser;
	uint32_t 			print_context = 0;

	if (inputParser_init(&parser)){
		log_err("unable to init inputTracer");
		return EXIT_FAILURE;
	}

	if (inputParser_add_cmd(&parser, "print", "Print a message", "Message to be printed", INPUTPARSER_CMD_TYPE_ARG, &print_context, (void(*)(void))print) < 0){
		log_err("unable to add cmd to inputParser");
	}
	if (inputParser_add_cmd(&parser, "helloworld", "Print \"Hello World\"", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, &print_context, (void(*)(void))print_hello) < 0){
		log_warn("we add two times the same cmd, the second'll not be reachable");
	}

	inputParser_exe(&parser, (uint32_t)(argc - 1), argv + 1);

	inputParser_clean(&parser);

	return EXIT_SUCCESS;
}

static void print(uint32_t* ctx, char* string){
	printf("%u - %s\n", *ctx, string);
	(*ctx) ++;
}

static void print_hello(uint32_t* ctx){
	printf("%u - Hello World\n", *ctx);
	(*ctx) ++;
}
