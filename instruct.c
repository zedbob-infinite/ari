
#include "instruct.h"
#include "memory.h"
#include "objprim.h"


void init_instruct(instruct* instructs)
{
    instructs->count = 0;
    instructs->capacity = 0;
	instructs->current = 0;
    instructs->code = NULL;
}

void reset_instruct(instruct *instructs)
{
    for (int i = 0; i < instructs->capacity; ++i) {
        code8 *code = instructs->code[i];
        if (code) {
            value operand = code->operand;
            if (VAL_IS_STRING(operand))
                FREE(char, VAL_AS_STRING(operand));
            FREE(code8*, code);
        }
    }
    FREE(code8**, instructs->code);
	init_instruct(instructs);
}
