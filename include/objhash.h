#ifndef ari_objhash_h
#define ari_objhash_h

#include <stdbool.h>

#include "object.h"

typedef struct objentry_t
{
    char *key;
    object *value;
    int type;
} objentry;

typedef struct objhash_t
{
    int size;
    int count;
    objentry **table;
} objhash;

void init_objhash(objhash *hashtable, int size);
bool objhash_remove(objhash *ht, char *key);
void objhash_set(objhash *ht, char *key, object *value);
object *objhash_get(objhash *ht, char *key);
void reset_objhash(objhash *hashtable);

#endif
