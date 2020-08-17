#include <stddef.h>

#include "instruct.h"
#include "module.h"
#include "object.h"
#include "frame.h"

void init_module(module *mod)
{
    init_object(&mod->obj, OBJ_MODULE, NULL);
    init_instruct(&mod->instructs);
    init_frame(&mod->local);
}
