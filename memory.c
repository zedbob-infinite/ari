#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "objcode.h"
#include "object.h"

void free_object(void *obj, objtype type)
{
    switch (type) {
        case OBJ_PRIMITIVE:
        {
            objprim *prim = (objprim*)obj;
            if (prim->ptype == PRIM_STRING) {
                FREE(char, PRIM_AS_STRING(prim));
            }
            FREE(objprim, prim);
            break;
        }
        case OBJ_CODE:
        {
            objcode *codeobj = (objcode*)obj;
            for (int i = 0; i < codeobj->argcount; i++)
                free_object(codeobj->arguments[i], OBJ_PRIMITIVE);
            FREE(objprim*, codeobj->arguments);
            reset_frame(&codeobj->localframe);
            reset_instruct(&codeobj->instructs);
            FREE(objcode, codeobj);
        }
        default:
            break;
    }
}

void *reallocate(void *previous, size_t oldsize, size_t newsize)
{
    if (newsize == 0) {
        free(previous);
        return NULL;
    }

    return realloc(previous, newsize);
}
