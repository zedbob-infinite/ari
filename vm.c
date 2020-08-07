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

static void vm_add_object(VM *vm, object *obj)
{
   if (vm->objs) {
       object *previous = vm->objs;
       vm->objs = obj;
       obj->next = previous;
       return;
   }
   vm->objs = obj;
   obj->next = NULL;
}

static inline void binary_comp(VM *vm, objstack *stack, tokentype optype)
{
    objprim *c = create_new_primitive(VAL_BOOL);
    vm_add_object(vm, (object*)c);

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

static inline void binary_op(VM *vm, objstack *stack, char optype)
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
    object *obj = (object*)c;
    vm_add_object(vm, obj);
    push_objstack(stack, obj);
}

static void set_name(objhash *globals, object *name, object *value)
{
    objprim *prim = (objprim*)name;
    objhash_set(globals, PRIM_AS_STRING(prim), value);
}

static object *get_name(objhash *globals, object *name)
{
    objprim *prim = (objprim*)name;
    return objhash_get(globals, PRIM_AS_STRING(prim));
}

static inline void advance(instruct *instructs)
{
	instructs->current++;
}

void execute(VM *vm, instruct *instructs)
{
    objstack *stack = &vm->evalstack;
    objhash *globals = &vm->globals;

    while (instructs->current < instructs->count) {
		int current = instructs->current;
		code8 *code = instructs->code[current];
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
				objprim *condition = (objprim*)pop_objstack(stack);

                if (PRIM_AS_BOOL(condition))
                    advance(instructs);
                else {
                    objprim *jump = (objprim*)operand;
                    instructs->current = instructs->current + PRIM_AS_INT(jump);
                }
                break;
			}
            case OP_LOAD_CONSTANT:
                vm_add_object(vm, (object*)operand);
				push_objstack(stack, operand);
                advance(instructs);
                break;
            case OP_LOAD_NAME:
            {
                object *obj = get_name(globals, operand);
                if (obj)
                    push_objstack(stack, obj);
                else {
                    objprim *prim = (objprim*)operand;
                    printf("Name %s not found...\n",
                            PRIM_AS_STRING(prim));
                }
                advance(instructs);
                break;
            }
            case OP_CALL_FUNCTION:
                advance(instructs);
                break;
            case OP_MAKE_FUNCTION:
                advance(instructs);
                break;
            case OP_STORE_NAME:
            {
                set_name(globals, operand, pop_objstack(stack));
                advance(instructs);
                break;
            }
            case OP_COMPARE:
                advance(instructs);
                break;
            case OP_BINARY_ADD:
				binary_op(vm, stack, '+');
                advance(instructs);
                break;
            case OP_BINARY_SUB:
                binary_op(vm, stack, '-');
                advance(instructs);
                break;
            case OP_BINARY_MULT:
                binary_op(vm, stack, '*');
                advance(instructs);
                break;
            case OP_BINARY_DIVIDE:
                binary_op(vm, stack, '/');
                advance(instructs);
                break;
            case OP_NEGATE:
            {
                objprim *obj = create_new_primitive(VAL_DOUBLE);
                vm_add_object(vm, (object*)obj);
                push_objstack(stack, (object*)obj);
                binary_op(vm, stack, '*');
                advance(instructs);
                break;
            }
            case OP_POP:
                //printpop = pop_objstack(stack);
                advance(instructs);
                break;
            case OP_PRINT:
            {
                if (peek_objstack(stack)) {
                    print_object(pop_objstack(stack));
                    printf("\n");
                }
                advance(instructs);
                break;
            }
            case OP_RETURN:
                //if (printpop)
                //print_object(pop_objstack(stack));
                advance(instructs);
                break;
        }
    }
}

void free_vm(VM *vm)
{
    object *vmobj = NULL;
    while ((vmobj = vm->objs)) {
        vm->objs = vm->objs->next;
        FREE_OBJECT(vmobj);
        printf("deleted object\n");
    }
	init_objstack(&vm->evalstack);
    reset_parser(&vm->analyzer);
    reset_scanner(&vm->analyzer.scan);
    reset_objhash(&vm->globals);
    FREE(VM, vm);
}

VM *init_vm(void)
{
    VM *vm = ALLOCATE(VM, 1);
    init_parser(&vm->analyzer);
    init_scanner(&vm->analyzer.scan);
    init_objstack(&vm->evalstack);
    init_objhash(&vm->globals, 8);
    vm->objs = NULL;

    return vm;
}
