#ifndef ari_instruct_h
#define ari_instruct_h

#include <stdint.h>

#include "object.h"

#define VAL_AS_INT(value)       (value.val_int)
#define VAL_AS_DOUBLE(value)    (value.val_double)
#define VAL_AS_STRING(value)    (value.val_string)

#define ALLOCATE_VAL_STRING(value, string)	\
    (value.val_string = ALLOCATE(char, sizeof(string) + 1))

#define COPY_VALUE_STRING(value, string, length)		(strncpy(value.val_string, string, length))

typedef enum 
{
    VAL_UNDEFINED = -1,
    VAL_INT,
    VAL_DOUBLE,
    VAL_STRING,
} valtype;

typedef union value_t
{
    int val_int;
    double val_double;
    char *val_string;
} value;

typedef struct code8_t
{
    uint8_t bytecode;
    valtype type;
	value operand;
} code8;

typedef struct instruct_t
{
	int current;
    int count;
    int capacity;
    code8 **code;
} instruct;

void init_instruct(instruct *instructs);
void reset_instruct(instruct *instructs);

#endif
