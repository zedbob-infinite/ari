#include <stdlib.h>

#include "compiler.h"
#include "instruct.h"
#include "opcode.h"
#include "token.h"
#include "tokenizer.h"
#include "value.h"
#include "valstack.h"
#include "vm.h"

static inline void binary_comp(valstack *stack, tokentype optype)
{
    value *c = create_new_value(VAL_BOOL);
    value *b = pop_valstack(stack);
    value *a = pop_valstack(stack);

    switch (optype) {
        case TOKEN_EQUAL_EQUAL:
            AS_BOOL(c) = (AS_NUMBER(a) == AS_NUMBER(b));
            break;
        case TOKEN_GREATER:
            AS_BOOL(c) = (AS_NUMBER(a) > AS_NUMBER(b));
            break;
        case TOKEN_GREATER_EQUAL:
            AS_BOOL(c) = (AS_NUMBER(a) >= AS_NUMBER(b));
            break;
        case TOKEN_LESS:
            AS_BOOL(c) = (AS_NUMBER(a) < AS_NUMBER(b));
            break;
        case TOKEN_LESS_EQUAL:
            AS_BOOL(c) = (AS_NUMBER(a) <= AS_NUMBER(b));
            break;
        default:
            // future error code here
            break;
    }
    push_valstack(stack, c);
}

static inline void binary_op(valstack *stack, char optype)
{
    value *c = create_new_value(VAL_NUMBER);
    value *b = pop_valstack(stack);
    value *a = pop_valstack(stack);

    switch (optype) {
        case '+': AS_NUMBER(c) = AS_NUMBER(a) + AS_NUMBER(b); break;
        case '-': AS_NUMBER(c) = AS_NUMBER(a) - AS_NUMBER(b); break;
        case '*': AS_NUMBER(c) = AS_NUMBER(a) * AS_NUMBER(b); break;
        case '/': AS_NUMBER(c) = AS_NUMBER(a) / AS_NUMBER(b); break;
    }
    push_valstack(stack, c);
}

static inline void advance(instruct *instructs)
{
    instructs->current++;
}

/*static void execute(VM *vm, instruct *instructs)
{
    while (instructs->current) {
        valstack *stack = &vm->evalstack;
        value *val = instructs->current->val;

        switch (instructs->current->bytecode) {
            case OP_LOOP:
                break;
            case OP_JMP_LOC:
                instructs->current = instructs->current + (int)AS_NUMBER(val);
                break;
            case OP_JMP_AFTER:
                instructs->current = instructs->current + (int)AS_NUMBER(val);
                break;
            case OP_JMP_FALSE:
                instructs->current = instructs->current + (int)AS_NUMBER(val);
                break;
            case OP_LOAD_CONSTANT:
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
                pop_valstack(stack);
                advance(instructs);
                break;
            case OP_RETURN:
                print_value(pop_valstack(stack));
                advance(instructs);
                break;
        }
    }
}

void interpet(VM *vm, instruct *instructs)
{

}*/


VM *init_vm(void)
{
    VM *vm = malloc(sizeof(VM));
    init_scanner(&vm->lexer);
    init_parser(&vm->analyzer);
    init_valstack(&vm->evalstack);

    return vm;
}
