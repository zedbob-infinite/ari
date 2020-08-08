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
        FREE_OBJECT(obj);
}

void push_objstack(objstack *stack, object *obj)
{
    objnode *node = ALLOCATE(objnode, 1);
    node->obj = obj;
    if (!stack->top) {
        node->next = NULL;
        stack->top = node;
    }
    else {
        node->next = stack->top;
        stack->top = node;
    }
	stack->count++;
}

object *pop_objstack(objstack *stack)
{
    objnode *popped = NULL;
    object *obj = NULL;

    if (!stack->top)
        return NULL;

    popped = stack->top;
    obj = popped->obj;
    stack->top = popped->next;
    FREE(objnode, popped);
    return obj;
}

object *peek_objstack(objstack *stack)
{
    objnode *peeked = NULL;
    object *obj = NULL;

    peeked = stack->top;
    if (peeked)
        obj = peeked->obj;
    return obj;
}
