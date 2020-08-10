#include "module.h"

void init_module(module *mod)
{
    mod->obj->type = OBJ_MODULE;
    init_instruct(&mod->instructs);
    init_objhash(&mod->locals, );
}
