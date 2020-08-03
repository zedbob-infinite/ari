#include <stdlib.h>

#include "valstack.h"
#include "value.h"

void init_valstack(valstack *stack)
{
    stack->count = 0;
    stack->capacity = 8;
    stack->top = NULL;
}

void destroy_valstack(valstack *stack)
{
    for (int i = 0; i < stack->capacity; ++i)
        pop_valstack(stack);
}

void push_valstack(valstack *stack, value *val)
{
    vsnode *node = malloc(sizeof(vsnode));
    node->val = val;
    if (!stack->top) {
        node->next = NULL;
        stack->top = node;
    }
    else {
        node->next = stack->top;
        stack->top = node;
    }
}

value *pop_valstack(valstack *stack)
{
    vsnode *popped;
    value *val;

    if (!stack->top)
        return NULL;

    popped = stack->top;
    val = popped->val;
    stack->top = popped->next;
    free(popped);
    return val;
}

value *peek_valstack(valstack *stack)
{
    vsnode *peeked;
    value *val = NULL;

    peeked = stack->top;
    if (peeked)
        val = peeked->val;
    return val;
}
