#include <stddef.h>

#include "frame.h"
#include "instruct.h"
#include "memory.h"
#include "objclass.h"
#include "objhash.h"

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

objclass *init_objclass(void)
{
    objclass *classobj = ALLOCATE(objclass, 1);
    init_object(&classobj->header, OBJ_CLASS);
    classobj->name = NULL;
    init_frame(&classobj->localframe);
    classobj->header.__attrs__ = &classobj->localframe.locals;
    init_objhash(&classobj->methods, DEFAULT_HT_SIZE);
    init_instruct(&classobj->instructs);
    return classobj;
}

objinstance *init_objinstance(objclass *class)
{
    objinstance *instobj = ALLOCATE(objinstance, 1);
    init_object(&instobj->header, OBJ_INSTANCE);
    init_frame(&instobj->localframe);
    instobj->header.__attrs__ = &instobj->localframe.locals;
    return instobj;
}
