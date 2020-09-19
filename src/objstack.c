#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "objstack.h"


void init_objstack(objstack *stack)
{
    stack->count = 0;
    stack->top = NULL;
}

void reset_objstack(objstack *stack)
{
    object *obj = NULL;
    while ((obj = pop_objstack(stack)))
        ;
}

void push_objstack(objstack *stack, object *obj)
{
    objnode *node = ALLOCATE(objnode, 1);
    node->obj = obj;
    node->next = stack->top;
    stack->top = node;
    stack->count++;
}

object *pop_objstack(objstack *stack)
{
    if (!stack->top)
        return NULL;

    objnode *popped = stack->top;
    object *obj = popped->obj;
    stack->top = popped->next;
    FREE(objnode, popped);
    return obj;
}

object *peek_objstack(objstack *stack)
{
    objnode *peeked = stack->top;
    if (!peeked)
        return NULL;
    return peeked->obj;
}
