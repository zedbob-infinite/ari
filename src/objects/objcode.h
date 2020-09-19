#ifndef ari_objcode_h
#define ari_objcode_h

#include <stddef.h>

#include "frame.h"
#include "instruct.h"
#include "object.h"
#include "objprim.h"

typedef struct objcode_t
{
    object header;
    char *name;
    size_t argcount;
    objprim **arguments;
    frame localframe;
    instruct instructs;
    size_t depth;
} objcode;

objcode *init_objcode(int argcount, objprim **arguments);

#endif
