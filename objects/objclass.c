#include <stddef.h>

#include "objclass.h"

/*typedef struct
{
    object obj;
    objprim *name;
    objhash methods;
} objclass;*/

/*typedef struct
{
    object obj;
    objclass *oclass;
    objhash fields;
} objinstance;*/

void init_objclass(objclass *ocls)
{
    init_object(&ocls->obj, OBJ_CLASS);
    ocls->name = NULL;
    init_objhash(&ocls->methods, DEFAULT_HT_SIZE);
}
