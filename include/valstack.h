#ifndef ari_valstack_h
#define ari_valstack_h

typedef struct value_t value;

typedef struct vsnode_t
{
    value *val;
    struct vsnode_t *next;
} vsnode;

typedef struct valstack_t
{
    int count;
    int capacity;
    vsnode *top;
} valstack;

void init_valstack(valstack *stack);
void destroy_valstack(valstack *stack);
void push_valstack(valstack *stack, value *val);
value *pop_valstack(valstack *stack);
value *peek_valstack(valstack *stack);

#endif
