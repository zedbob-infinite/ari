#ifndef ari_objclass_h
#define ari_objclass_h

#include "object.h"
#include "objhash.h"

typedef struct
{
    object obj;
    objprim *name;
    objhash methods;
} objclass;

typedef struct
{
    object obj;
    objclass *oclass;
    objhash fields;
} objinstance;

void init_objclass(objclass *ocls);

#endif
