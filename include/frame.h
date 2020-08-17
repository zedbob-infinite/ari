#ifndef ari_frame_h
#define ari_frame_h

#include "objhash.h"

typedef struct frame_t
{
    objhash locals;
    struct frame_t *next;
} frame;

void init_frame(frame *f);
void reset_frame(frame *f);
void push_frame(frame **top, frame *newframe);
void pop_frame(frame **top);

#endif
