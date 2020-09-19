#ifndef ari_instruct_h
#define ari_instruct_h

#include <stdint.h>

#include "object.h"

#define VAL_IS_EMPTY(value)     (value->type == VAL_INT)
#define VAL_IS_BOOL(value)      (value->type == VAL_BOOL)
#define VAL_IS_INT(value)       (value->type == VAL_INT)
#define VAL_IS_DOUBLE(value)    (value->type == VAL_DOUBLE)
#define VAL_IS_STRING(value)    (value->type == VAL_STRING)
#define VAL_IS_NULL(value)      (value->type == VAL_NULL)
#define VAL_IS_OBJECT(value)    (value->type == VAL_OBJ)

#define VAL_AS_BOOL(value)      (value->val_int)
#define VAL_AS_INT(value)       (value->val_int)
#define VAL_AS_DOUBLE(value)    (value->val_double)
#define VAL_AS_STRING(value)    (value->val_string)
#define VAL_AS_NULL(value)      (value->val_int)
#define VAL_AS_OBJECT(value)    (value->val_obj)

#define NUMBER_VAL(val)         ((value){VAL_NUBMER, {.val_double = val}})
#define EMPTY_VAL               ((value){VAL_EMPTY, {.val_int = 0}})
#define NULL_VAL                ((value){VAL_NULL, {.val_int = 0}})

#define ALLOCATE_VAL_STRING(value, string)  \
    (value.val_string = ALLOCATE(char, sizeof(string) + 1))

#define COPY_VALUE_STRING(value, string, length)        (strncpy(value.val_string, string, length))

typedef enum 
{
    VAL_EMPTY,
    VAL_BOOL,
    VAL_INT,
    VAL_DOUBLE,
    VAL_STRING,
    VAL_NULL,
    VAL_OBJECT,
} valtype;

typedef struct value_t
{
    valtype type;
    union 
    {
        int val_int;
        double val_double;
        char *val_string;
        object *val_obj;
    };
} value;

typedef struct code8_t
{
    uint8_t bytecode;
    value operand;
    int line;
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
