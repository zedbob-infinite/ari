#ifndef ari_instruct_h
#define ari_instruct_h

#include <stdint.h>

#include "object.h"

typedef struct code8_t
{
    uint8_t bytecode;
	object *operand;
} code8;

typedef struct instruct_t
{
	int current;
    int count;
    int capacity;
    code8 **code;
} instruct;

instruct *init_instructs(void);
void destroy_instructs(instruct *instructs);

#endif
