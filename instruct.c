
#include "instruct.h"
#include "memory.h"

void init_instruct(instruct* instructs)
{
    instruct->count = 0;
    new_instruct->capacity = 0;
	new_instruct->current = 0;
    new_instruct->code = NULL;
}

void reset_instructs(instruct *instructs)
{
    FREE_OBJECT(instructs, instruct);
	init_instruct(instructs);
}
