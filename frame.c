#include <stddef.h>

#include "frame.h"
#include "objhash.h"

void init_frame(frame *f)
{
    init_objhash(&f->locals, DEFAULT_HT_SIZE);
    f->pc = 0;
    f->next = NULL;
}

void reset_frame(frame *f)
{
    reset_objhash(&f->locals);
    f->pc = 0;
    f->next = NULL;
}

void push_frame(frame **top, frame *newframe)
{
    frame *previous = *top;
    newframe->next = previous;
    *top = newframe;
}

frame *pop_frame(frame **top)
{
    frame *oldtop = *top;
    frame *next = oldtop->next;
    *top = next;
    return oldtop;
}
