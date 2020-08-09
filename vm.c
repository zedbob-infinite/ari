#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "objstack.h"
#include "opcode.h"
#include "token.h"
#include "tokenizer.h"
#include "vm.h"

//#define DEBUG_ARI

static inline void delete_value(value *val, valtype type)
{
    if (type == VAL_STRING)
        FREE(char, val->val_string);
}

static void vm_add_object(VM *vm, object *obj)
{
   if (vm->objs) {
       object *previous = vm->objs;
       vm->objs = obj;
       obj->next = previous;
       return;
   }
   vm->objs = obj;
   vm->num_objects++;
   obj->next = NULL;
}

static void runtime_error(objstack *stack, size_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "[line %ld] in script\n", line);

    reset_objstack(stack);
}

static char *take_string(value val, int length)
{
    char *buffer = ALLOCATE(char, length + 1);
    buffer = strncpy(buffer, VAL_AS_STRING(val), length);
    buffer[length] = '\0';
    return buffer;
}

static inline void binary_comp(VM *vm, objstack *stack, tokentype optype)
{
    objprim *c = create_new_primitive(PRIM_BOOL);
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
    object *obj = (object*)c;
    vm_add_object(vm, obj);
    push_objstack(stack, obj);
}

static inline void binary_op(VM *vm, objstack *stack, char optype)
{
    objprim *c = create_new_primitive(PRIM_DOUBLE); 
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

static inline void set_name(objhash *globals, value name, object *val)
{
    objhash_set(globals, VAL_AS_STRING(name), val);
}

static inline object *get_name(objhash *globals, value name)
{
    return objhash_get(globals, VAL_AS_STRING(name));
}

static inline void advance(instruct *instructs)
{
	instructs->current++;
}

static void print_bytecode(uint8_t bytecode)
{
    switch (bytecode) { 
        case OP_LOOP:
            printf("OP_LOOP");
            break;
        case OP_JMP_LOC:
            printf("OP_JMP_LOC");
            break;
        case OP_JMP_AFTER:
            printf("OP_JMP_AFTER");
            break;
        case OP_JMP_FALSE:
            printf("OP_JMP_FALSE");
            break;
        case OP_POP:
            printf("OP_POP");
            break;
        case OP_LOAD_CONSTANT:
            printf("OP_LOAD_CONSTANT");
            break;
        case OP_LOAD_NAME:
            printf("OP_LOAD_NAME");
            break;
        case OP_CALL_FUNCTION:
            printf("OP_CALL_FUNCTION");
            break;
        case OP_MAKE_FUNCTION:
            printf("OP_MAKE_FUNCTION");
            break;
        case OP_STORE_NAME:
            printf("OP_STORE_NAME");
            break;
        case OP_COMPARE:
            printf("OP_COMPARE");
            break;
        case OP_BINARY_ADD:
            printf("OP_BINARY_ADD");
            break;
        case OP_BINARY_SUB:
            printf("OP_BINARY_SUB");
            break;
        case OP_BINARY_MULT:
            printf("OP_BINARY_MULT");
            break;
        case OP_BINARY_DIVIDE:
            printf("OP_BINARY_DIVIDE");
            break;
        case OP_NEGATE:
            printf("OP_NEGATE");
            break;
        case OP_PRINT:
            printf("OP_PRINT");
            break;
        case OP_RETURN:
            printf("OP_RETURN");
            break;
    }
}

int execute(VM *vm, instruct *instructs)
{
    objstack *stack = &vm->evalstack;
    objhash *globals = &vm->globals;

    while (instructs->current < instructs->count) {
		int current = instructs->current;
		code8 *code = instructs->code[current];
        value operand = code->operand;
        valtype type = code->type; 
#ifdef DEBUG_ARI
        printf("|%d|    ", current);
        print_bytecode(code->bytecode);
        //print_value(operand);

        printf("\n");
#endif
        switch (code->bytecode) {
            case OP_LOOP:
                break;
            case OP_JMP_LOC:
			{
                instructs->current = VAL_AS_INT(operand);
                break;
			}
            case OP_JMP_AFTER:
			{
                instructs->current = instructs->current + VAL_AS_INT(operand);
                break;
			}
            case OP_JMP_FALSE:
			{
				objprim *condition = (objprim*)pop_objstack(stack);

                if (PRIM_AS_BOOL(condition))
                    advance(instructs);
                else {
                    instructs->current = VAL_AS_INT(operand);
                }
                break;
			}
            case OP_LOAD_CONSTANT:
            {
                objprim *prim = NULL;
                switch (type) {
                    case VAL_UNDEFINED:
                    {
                        runtime_error(stack, current, "No object found.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case VAL_INT:
                        prim = create_new_primitive(PRIM_INT);
                        PRIM_AS_INT(prim) = VAL_AS_INT(operand);
                        break;
                    case VAL_DOUBLE:
                        prim = create_new_primitive(PRIM_DOUBLE);
                        PRIM_AS_DOUBLE(prim) = VAL_AS_DOUBLE(operand);
                        break;
                    case VAL_STRING:
                        prim = create_new_primitive(PRIM_STRING);
                        PRIM_AS_STRING(prim) = take_string(operand, strlen(VAL_AS_STRING(operand)));
                        break;
                }
                object *obj = (object*)prim;
                vm_add_object(vm, obj);
				push_objstack(stack, obj);
                advance(instructs);
                break;
            }
            case OP_LOAD_NAME:
            {
                object *obj = get_name(globals, operand);
                if (obj)
                    push_objstack(stack, obj);
                else {
                    printf("Name %s not found...\n",
                            VAL_AS_STRING(operand));
                    return INTERPRET_RUNTIME_ERROR;
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
            {
                binary_comp(vm, stack, VAL_AS_INT(operand));
                advance(instructs);
                break;
            }
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
                objprim *obj = create_new_primitive(PRIM_DOUBLE);
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
    return INTERPRET_OK;
}

void free_vm(VM *vm)
{
    reset_objstack(&vm->evalstack);
    object *current = vm->objs;
    object *next = NULL;
    while (current) {
        next = current->next;
        FREE_OBJECT(current);
        printf("deleted object\n");
        current = next;
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
    init_objhash(&vm->globals, DEFAULT_HT_SIZE);
    vm->objs = NULL;
    vm->num_objects = 0;

    return vm;
}
