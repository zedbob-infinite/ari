#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "objhash.h"

#define ALLOCATE_OBJENTRY(a)    objentry *a = ALLOCATE(objentry, 1);

static int objhash_hash(int size, char *key)
{
    unsigned long int hashval = 0;
    int i = 0;

    // Convert string to integer
    while (hashval < ULONG_MAX && i < strlen(key)) {
        hashval <<= 8;
        hashval += key[i];
        i++;
    }
    return hashval % size;
}

static objentry *objhash_newpair(char *key, object *value)
{
    int keylength = strlen(key) + 1;
    ALLOCATE_OBJENTRY(newpair);
    if (newpair) {
        newpair->key = ALLOCATE(char, keylength + 1);
        strncpy(newpair->key, key, keylength);
        newpair->value = value;
    }
    return newpair;
}

static objentry *objhash_find_entry(objhash *ht, char *key, int *binnum)
{
    int bin = objhash_hash(ht->size, key);
    objentry *next = ht->table[bin];

    while (next && next->key && strcmp(key, next->key) > 0) {
        if (bin < ht->size)
            bin++;
        else
            bin = 0;
        next = ht->table[bin];
    }

    *binnum = bin;
    if (next && next->key && strcmp(key, next->key) == 0)  {
        return next;
    }

    return NULL;
}

static inline void objhash_remove_entry(objentry *entry)
{
    if (entry) {
        if (entry->key)
            FREE(char, entry->key);
    }
    FREE(objentry, entry);
}

void init_objhash(objhash* hashtable, int size)
{
    if (size < 1)
        return;

    // Given the possibility of memory allocation failures
    // probably should test within the memory allocation module itself
    // and somehow generate an abortive error (future stuff)
    if (!(hashtable->table = ALLOCATE(objentry*, size)))
        return;

    for (int i = 0; i < size; ++i)
        hashtable->table[i] = NULL;

    hashtable->size = size;
}

void reset_objhash(objhash *hashtable)
{
    objentry *entry = NULL;
    for (int i = 0; i < hashtable->size; ++i) {
        entry = hashtable->table[i];
        objhash_remove_entry(entry);
    }
    FREE(objentry*, hashtable->table);
}

bool objhash_remove(objhash *ht, char *key)
{
    int bin = 0;
    objentry *e = objhash_find_entry(ht, key, &bin);

    if (!e)
        return false;
    
    objhash_remove_entry(e);
    ht->count--;

    return true;
}

void objhash_set(objhash *ht, char *key, object *value)
{
    int bin = 0;
    objentry *find = objhash_find_entry(ht, key, &bin);
    objentry *newpair = objhash_newpair(key, value);

    if (!newpair) {
        // future error code here
        return;
    }
    if (find) {
        objhash_remove_entry(find);
        ht->count--;
    }

    ht->table[bin] = newpair;
    ht->count++;
}

object *objhash_get(objhash *ht, char *key)
{
    int bin = 0;
    objentry *entry = objhash_find_entry(ht, key, &bin);
    
    if (entry)
        return entry->value;

    return NULL;
}
