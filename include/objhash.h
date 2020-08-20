#ifndef ari_objhash_h
#define ari_objhash_h

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_HT_SIZE 32

#define TABLE_MAX_LOAD 0.75

struct object_t;

typedef struct objentry_t
{
    char *key;
    struct object_t *value;
    int type;
} objentry;

typedef struct objhash_t
{
    uint32_t count;
    uint32_t capacity;
    objentry **table;
} objhash;

void init_objhash(objhash *hashtable, uint32_t size);
bool objhash_remove(objhash *ht, char *key);
void objhash_set(objhash *ht, char *key, struct object_t *value);
struct object_t *objhash_get(objhash *ht, char *key);
void reset_objhash(objhash *hashtable);

#endif
