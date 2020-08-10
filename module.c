#include "instruct.h"
#include "module.h"
#include "object.h"
#include "objhash.h"

void init_module(module *mod)
{
    mod->obj.type = OBJ_MODULE;
    init_instruct(&mod->instructs);
    init_objhash(&mod->locals, DEFAULT_HT_SIZE);
}
