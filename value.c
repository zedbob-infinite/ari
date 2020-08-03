#include <stdio.h>
#include <stdlib.h>

#include "value.h"

value *create_new_value(valtype type)
{
    value *val = malloc(sizeof(value));
    val->type = type;
    return val;
}

void print_value(value *val)
{
    switch (val->type) {
        case VAL_BOOL:      printf(AS_BOOL(val) ? "true" : "false"); break;
        case VAL_NULL:      printf("null"); break;
        case VAL_NUMBER:    printf("%g", AS_NUMBER(val)); break;
        case VAL_STRING:    printf("%s", AS_STRING(val)); break;
    }
}

