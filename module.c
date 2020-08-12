#include "instruct.h"
#include "module.h"
#include "object.h"
#include "frame.h"

void init_module(module *mod)
{
    mod->obj.type = OBJ_MODULE;
    init_instruct(&mod->instructs);
    init_frame(&mod->local);
}
