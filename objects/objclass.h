#ifndef ari_objclass_h
#define ari_objclass_h

#include "frame.h"
#include "instruct.h"
#include "object.h"
#include "objhash.h"
#include "objprim.h"

typedef struct
{
    object header;
    primstring *name;
    frame localframe;
    objhash methods;
    // For compiling the class
    instruct instructs;
} objclass;

typedef struct
{
    object header;
    objclass *class;
    frame localframe;
} objinstance;

objclass *init_objclass(void);
objinstance *init_objinstance(objclass *class);

#endif
