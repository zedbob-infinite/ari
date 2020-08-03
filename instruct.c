
#include "instruct.h"
#include "value.h"


instruct *init_instructs(void)
{
    instruct *new_instructs = malloc(sizeof(instruct));
    instruct->count = 0;
    instruct->capacity = 0;
    instruct->current = NULL;
}

void reset_instructs(instruct *instructs)
{
    for (int i = 0; i < instructs->count; i++)
        free(instructs->current);
    instruct->count = 0;
    instruct->capacity = 0;
    instruct->current = NULL;
}
