#include <stdlib.h>

#include "memory.h"

void *reallocate(void *previous, size_t oldsize, size_t newsize)
{
    if (newsize == 0) {
        free(previous);
        return NULL;
    }

    return realloc(previous, newsize);
}
