#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "objstack.h"
#include "opcode.h"
#include "token.h"
#include "tokenizer.h"
#include "vm.h"

static inline void binary_comp(objstack *stack, tokentype optype)
{
    objprim *c = create_new_primitive(VAL_BOOL);
    objprim *b = (objprim*)pop_objstack(stack);
	objprim *a = (objprim*)pop_objstack(stack);

    switch (optype) {
        case TOKEN_EQUAL_EQUAL:
            PRIM_AS_BOOL(c) = (PRIM_AS_DOUBLE(a) == PRIM_AS_DOUBLE(b));
            break;
        case TOKEN_GREATER:
            PRIM_AS_BOOL(c) = (PRIM_AS_DOUBLE(a) > PRIM_AS_DOUBLE(b));
            break;
        case TOKEN_GREATER_EQUAL:
            PRIM_AS_BOOL(c) = (PRIM_AS_DOUBLE(a) >= PRIM_AS_DOUBLE(b));
            break;
        case TOKEN_LESS:
            PRIM_AS_BOOL(c) = (PRIM_AS_DOUBLE(a) < PRIM_AS_DOUBLE(b));
            break;
        case TOKEN_LESS_EQUAL:
            PRIM_AS_BOOL(c) = (PRIM_AS_DOUBLE(a) <= PRIM_AS_DOUBLE(b));
            break;
        default:
            // future error code here
            break;
    }
    push_objstack(stack, (object*)c);
}

static inline void binary_op(objstack *stack, char optype)
{

    objprim *c = create_new_primitive(VAL_DOUBLE);
    objprim *b = (objprim*)pop_objstack(stack);
	objprim *a = (objprim*)pop_objstack(stack);

    switch (optype) {
        case '+': PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) + PRIM_AS_DOUBLE(b); break;
        case '-': PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) - PRIM_AS_DOUBLE(b); break;
        case '*': PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) * PRIM_AS_DOUBLE(b); break;
        case '/': PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) / PRIM_AS_DOUBLE(b); break;
    }
    push_objstack(stack, (object*)c);
}

static inline void advance(instruct *instructs)
{
	instructs->current++;
}

void execute(VM *vm, instruct *instructs)
{
	printf("Entering execute()...\n");
    while (instructs->current < instructs->count) {
		int current = instructs->current;
		code8 *code = instructs->code[current];
        objstack *stack = &vm->evalstack;
        object *operand = code->operand;

        switch (code->bytecode) {
            case OP_LOOP:
                break;
            case OP_JMP_LOC:
			{
				objprim *jump = (objprim*)operand;
                instructs->current = instructs->current + PRIM_AS_INT(jump);
                break;
			}
            case OP_JMP_AFTER:
			{
				objprim *jump = (objprim*)operand;
                instructs->current = instructs->current + PRIM_AS_INT(jump);
                break;
			}
            case OP_JMP_FALSE:
			{
				objprim *jump = (objprim*)operand;
                instructs->current = instructs->current + PRIM_AS_INT(jump);
                break;
			}
            case OP_LOAD_CONSTANT:
				push_objstack(stack, operand);
                advance(instructs);
                break;
            case OP_LOAD_NAME:
                advance(instructs);
                break;
            case OP_CALL_FUNCTION:
                advance(instructs);
                break;
            case OP_MAKE_FUNCTION:
                advance(instructs);
                break;
            case OP_STORE_NAME:
                advance(instructs);
                break;
            case OP_COMPARE:
                advance(instructs);
                break;
            case OP_BINARY_ADD:
				binary_op(stack, '+');
                advance(instructs);
                break;
            case OP_BINARY_SUB:
                advance(instructs);
                break;
            case OP_BINARY_MULT:
                advance(instructs);
                break;
            case OP_BINARY_DIVIDE:
                advance(instructs);
                break;
            case OP_NEGATE:
                advance(instructs);
                break;
            case OP_POP:
                pop_objstack(stack);
                advance(instructs);
                break;
            case OP_RETURN:
                print_object(pop_objstack(stack));
                advance(instructs);
                break;
        }
    }
	printf("exiting execute...\n");
}

void free_vm(VM *vm)
{
    reset_parser(&vm->analyzer);
    reset_scanner(&vm->analyzer.scan);
	reset_objstack(&vm->evalstack);
    FREE_OBJECT(VM, vm);
}

VM *init_vm(void)
{
    VM *vm = ALLOCATE(VM, 1);
    init_parser(&vm->analyzer);
    init_scanner(&vm->analyzer.scan);
    init_objstack(&vm->evalstack);

    return vm;
}
