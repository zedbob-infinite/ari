#ifndef ari_module_t
#define ari_module_t

#include "frame.h"
#include "instruct.h"
#include "object.h"
#include "objhash.h"

typedef struct module_t
{
    object header;
    frame local;
    instruct instructs;
} module;

void init_module(module *mod);
void reset_module(module *mod);

#endif
