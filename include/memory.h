#ifndef ari_memory_h
#define ari_memory_h

#define DEFAULT_CAPACITY 8

#define GROW_CAPACITY(capacity) \
    ((capacity) < DEFAULT_CAPACITY ? DEFAULT_CAPACITY : (capacity) * 2)

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_ARRAY(previous, type, oldcount, count) \
    (type*)reallocate(previous, sizeof(type) * (oldcount), \
            sizeof(type) * (count))

#define FREE_ARRAY(type, pointer, oldcount) \
    reallocate(pointer, sizeof(type) * (oldcount), 0)

void *reallocate(void *previous, size_t oldsize, size_t newsize);

#endif
