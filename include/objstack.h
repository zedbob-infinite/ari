#ifndef ari_objstack_h
#define ari_objstack_h

typedef struct object_t object;

typedef struct objnode_t
{
    object *obj;
    struct objnode_t *next;
} objnode;

typedef struct valstack_t
{
    int count;
    objnode *top;
} objstack;

void init_objstack(objstack *stack);
void reset_objstack(objstack *stack);
void push_objstack(objstack *stack, object *obj);
object *pop_objstack(objstack *stack);
object *peek_objstack(objstack *stack);

#endif
