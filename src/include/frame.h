#ifndef ari_frame_h
#define ari_frame_h

#include <stdbool.h>
#include <stddef.h>

#include "objhash.h"

typedef struct frame_t
{
    objhash locals;
    size_t pc;
    struct frame_t *next;
    bool is_adhoc;
} frame;

void init_frame(frame *f);
void reset_frame(frame *f);
void push_frame(frame **top, frame *newframe);
frame *pop_frame(frame **top);

#endif
