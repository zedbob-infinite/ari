#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "object.h"

void free_object(void *obj, objtype type)
{
    switch (type) {
        case OBJ_PRIMITIVE:
        {
            objprim *prim = (objprim*)obj;
            if (prim->primtype == VAL_STRING) {
                FREE(char*, PRIM_AS_STRING(prim));
            }
            FREE(objprim, obj);
            break;
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
