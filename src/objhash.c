#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "objhash.h"
#include "objprim.h"


static void copy_primstring(primstring *dest, primstring *source)
{
    dest->length = source->length;
    dest->hash = source->hash;
    dest->_string_ = ALLOCATE(char, source->length + 1);
    memcpy(dest->_string_, source->_string_, dest->length);
    dest->_string_[dest->length] = '\0';
}

static bool is_key(primstring *key, primstring *compare)
{
    if ((!key) || (!compare))
        return false;
    if (key->length != compare->length)
        return false;
    if (key->hash != compare->hash)
        return false;
    if (strncmp(key->_string_, compare->_string_, key->length))
        return false;
    return true;
}

static objentry *objhash_newpair(primstring *key, object *value)
{
    objentry *newpair = ALLOCATE(objentry, 1);
    if (newpair) {
        primstring *newkey = ALLOCATE(primstring, 1);
        copy_primstring(newkey, key);
        newpair->key = newkey;
        newpair->value = value;
    }
    return newpair;
}

static objentry *objhash_find_entry(objentry **entries, uint32_t size, 
        primstring *key, uint32_t *binnum)
{
    uint32_t bin = key->hash & (size - 1);
    objentry *next = entries[bin];

    while (next && next->key && is_key(key, next->key) == false) {
        if (bin < size)
            bin++;
        else
            bin = 0;
        next = entries[bin];
    }

    *binnum = bin;
    if (next && next->key && is_key(key, next->key) == true)  {
        return next;
    }

    return NULL;
}

static inline void objhash_remove_entry(objentry *entry)
{
    if (entry)
        if (entry->key)
            free_primstring(entry->key);
    FREE(objentry, entry);
}


static void check_capacity(objhash* ht)
{
    int oldsize = ht->capacity;
    ht->capacity = GROW_CAPACITY(ht->capacity);

    int newsize = ht->capacity;
    objentry **entries = ALLOCATE(objentry*, newsize);

    for (int i = 0; i < newsize; ++i)
        entries[i] = NULL;

    ht->count = 0;
    uint32_t bin = 0;
    for (int i = 0; i < oldsize; ++i) {
        objentry *entry = ht->table[i];
        if (!entry) continue;

        objentry *dest = objhash_find_entry(entries, newsize, entry->key,
                                         &bin);
        dest->key = entry->key;
        dest->value = entry->value;
        ht->count++;
        FREE(objentry, entry);
    }
    FREE(objentry*, ht->table);
    ht->table = entries;
}

void init_objhash(objhash* ht, uint32_t size)
{
    if (size < 1)
        return;

    ht->table = NULL;
    // Given the possibility of memory allocation failures
    // probably should test within the memory allocation module itself
    // and somehow generate an abortive error (future stuff)
    if (!(ht->table = ALLOCATE(objentry*, size)))
        return;

    for (int i = 0; i < size; ++i)
        ht->table[i] = NULL;

    ht->count = 0;
    ht->capacity = size;
}

void reset_objhash(objhash *ht)
{
    objentry *entry = NULL;
    for (int i = 0; i < ht->capacity; ++i) {
        entry = ht->table[i];
        objhash_remove_entry(entry);
    }
    FREE(objentry*, ht->table);
}

bool objhash_remove(objhash *ht, primstring *key)
{
    uint32_t bin = 0;
    objentry *entry = objhash_find_entry(ht->table, ht->capacity, key, &bin);

    if (!entry)
        return false;
    
    objhash_remove_entry(entry);
    ht->count--;

    return true;
}

void objhash_set(objhash *ht, primstring *key, object *value)
{
    if (ht->count + 1 > ht->capacity * TABLE_MAX_LOAD)
        check_capacity(ht);

    uint32_t bin = 0;
    uint32_t size = ht->capacity; 
    objentry *find = objhash_find_entry(ht->table, size, key, &bin);
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

object *objhash_get(objhash *ht, primstring *key)
{
    uint32_t bin = 0;
    uint32_t size = ht->capacity;
    objentry *entry = objhash_find_entry(ht->table, size, key, &bin);
    
    if (entry)
        return entry->value;

    return NULL;
}
