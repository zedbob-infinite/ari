#ifndef ari_module_t
#define ari_module_t

#include "instruct.h"
#include "object.h"
#include "objhash.h"

typedef struct module_t
{
    object obj;
    objhash locals;
    instruct instructs;
} module;

void init_module(module *mod);
void reset_module(module *mod);

#endif
