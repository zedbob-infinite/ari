#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "memory.h"
#include "objclass.h"
#include "objcode.h"
#include "object.h"
#include "objhash.h"


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
            break;
        }

        case OBJ_CLASS:
        {
            objclass *classobj = (objclass*)obj;
            FREE(char, classobj->name);
            reset_frame(&classobj->localframe);
            reset_objhash(&classobj->methods);
            reset_instruct(&classobj->instructs);
            FREE(objclass, classobj);
            break;
        }
        case OBJ_INSTANCE:
        {
            objinstance *instobj = (objinstance*)obj;
            reset_frame(&instobj->localframe);
            FREE(objinstance, instobj);
            break;
        }
        case OBJ_BUILTIN:
        {
            objbuiltin *builtin_obj = (objbuiltin*)obj;
            FREE(objbuiltin, builtin_obj);
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
