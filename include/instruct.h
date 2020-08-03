#ifndef ari_instruct_h
#define ari_instruct_h

#include <stdint.h>

#include "value.h"

typedef struct code8_t
{
    value val;
    uint8_t bytecode;
} code8;

typedef struct instruct_t
{
    int count;
    int capacity;
    code8* current;
} instruct;

instruct *init_instructs(void);
void destroy_instructs(instruct *instructs);

#endif
