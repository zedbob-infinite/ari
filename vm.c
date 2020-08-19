#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "instruct.h"
#include "frame.h"
#include "memory.h"
#include "objcode.h"
#include "objstack.h"
#include "opcode.h"
#include "token.h"
#include "tokenizer.h"
#include "vm.h"

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

static inline void set_name(frame *localframe, char *name, object *val)
{
    objhash_set(&localframe->locals, name, val);
}

static inline object *get_name(frame *localframe, value name)
{
    frame *current = localframe;
    object *obj = NULL;
    do {
        obj = objhash_get(&current->locals, VAL_AS_STRING(name));
        current = current->next;
    } while ((!obj) && (current));
    return obj;
}

static inline void advance(frame *callframe)
{
    callframe->pc++;
}

static void print_bytecode(uint8_t bytecode)
{
	char *msg = NULL;
    switch (bytecode) { 
        case OP_POP_FRAME:
			msg = "POP_FRAME";
            break;
        case OP_PUSH_FRAME:
            msg ="PUSH_FRAME";
            break;
        case OP_JMP_LOC:
            msg = "JMP_LOC";
            break;
        case OP_JMP_AFTER:
            msg = "JMP_AFTER";
            break;
        case OP_JMP_FALSE:
            msg = "JMP_FALSE";
            break;
        case OP_POP:
            msg = "POP";
            break;
        case OP_LOAD_CONSTANT:
            msg = "LOAD_CONSTANT";
            break;
        case OP_LOAD_NAME:
            msg = "LOAD_NAME";
            break;
        case OP_CALL_FUNCTION:
            msg = "CALL_FUNCTION";
            break;
        case OP_MAKE_FUNCTION:
            msg = "MAKE_FUNCTION";
            break;
        case OP_STORE_NAME:
            msg = "STORE_NAME";
            break;
        case OP_COMPARE:
            msg = "COMPARE";
            break;
        case OP_BINARY_ADD:
            msg = "BINARY_ADD";
            break;
        case OP_BINARY_SUB:
            msg = "BINARY_SUB";
            break;
        case OP_BINARY_MULT:
            msg = "BINARY_MULT";
            break;
        case OP_BINARY_DIVIDE:
            msg = "BINARY_DIVIDE";
            break;
        case OP_NEGATE:
            msg = "NEGATE";
            break;
        case OP_PRINT:
            msg = "PRINT";
            break;
        case OP_RETURN:
            msg = "RETURN";
            break;
    }
	printf("%-20s", msg);
}

int execute(VM *vm, instruct *instructs)
{
    vm->callstackpos++;
#ifdef DEBUG_ARI
    printf("Current Frame is %p\n\n", vm->top);
    printf("Call Stack Position is %ld\n", vm->callstackpos);
#endif
    objstack *stack = &vm->evalstack;
    while (vm->top->pc < instructs->count) {
		int current = vm->top->pc;
		code8 *code = instructs->code[current];
        value operand = {VAL_EMPTY, {0}};
        operand = code->operand;
        valtype type = code->operand.type; 
#ifdef DEBUG_ARI
        printf("|%d|\t", current + 1);
        print_bytecode(code->bytecode);
        printf("\t(");
        print_value(operand, type);
        printf(")");
        printf("\n");
#endif
        switch (code->bytecode) {
            case OP_PUSH_FRAME:
            {
                frame *newframe = ALLOCATE(frame, 1);
                init_frame(newframe);
                push_frame(&vm->top, newframe); 
                advance(vm->top);
                break;
            }
            case OP_POP_FRAME:
            {
                frame *oldframe = pop_frame(&vm->top);
                reset_frame(oldframe);
                FREE(frame, oldframe);
                advance(vm->top);
                break;
            }
            case OP_JMP_LOC:
			{
                vm->top->pc = VAL_AS_INT(operand);
                break;
			}
            case OP_JMP_AFTER:
			{
                vm->top->pc = current + VAL_AS_INT(operand);
                break;
			}
            case OP_JMP_FALSE:
			{
				objprim *condition = (objprim*)pop_objstack(stack);

                if (PRIM_AS_BOOL(condition))
                    advance(vm->top);
                else {
                    vm->top->pc = VAL_AS_INT(operand);
                }
                break;
			}
            case OP_LOAD_CONSTANT:
            {
                objprim *prim = NULL;
                switch (type) {
                    case VAL_EMPTY:
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
                    case VAL_NULL:
                        prim = create_new_primitive(PRIM_NULL);
                        PRIM_AS_NULL(prim) = VAL_AS_NULL(operand);
                        break;
					default:
					{
						runtime_error(stack, current, "Cannot load non-constant value.");
						return INTERPRET_RUNTIME_ERROR;
					}
                }
                object *obj = (object*)prim;
                vm_add_object(vm, obj);
				push_objstack(stack, obj);
                advance(vm->top);
                break;
            }
            case OP_LOAD_NAME:
            {
                object *obj = get_name(vm->top, operand);
                if (obj)
                    push_objstack(stack, obj);
                else {
                    printf("Name %s not found...\n",
                            VAL_AS_STRING(operand));
                    return INTERPRET_RUNTIME_ERROR;
                }
                advance(vm->top);
                break;
            }
            case OP_CALL_FUNCTION:
			{
				int argcount = VAL_AS_INT(operand);
				object **arguments = ALLOCATE(object*, argcount);
#ifdef DEBUG_ARI
                printf("\n");
#endif
				for (int i = 0; i < argcount; ++i) {
					arguments[i] = pop_objstack(stack);
#ifdef DEBUG_ARI
                    printf("   \targument %d: ", i + 1);
                    printf("\tvalue: ");
                    print_object(arguments[i]);
#endif
				}
#ifdef DEBUG_ARI
                printf("\n");
#endif
				objcode *funcobj = (objcode*)pop_objstack(stack);
                frame *localframe = NULL;

                if (funcobj->depth == 0) {
                    vm_add_object(vm, (object*)funcobj);    
                    localframe = &funcobj->localframe;
                }
                else {
                    localframe = ALLOCATE(frame, 1);
                    init_frame(localframe);
                }
                push_frame(&vm->top, localframe); 

				for (int k = 0, i = argcount - 1; k < argcount; k++) {
#ifdef DEBUG_ARI
                    printf("   \tcode object argument %d: %s\n", k + 1,
                            PRIM_AS_STRING(funcobj->arguments[k]));
#endif
					set_name(vm->top, 
							PRIM_AS_STRING(funcobj->arguments[k]),
							arguments[i--]);
				}
#ifdef DEBUG_ARI
                printf("\n");
#endif
				funcobj->depth++;
                funcobj->instructs.current = 0;
                execute(vm, &funcobj->instructs);
                funcobj->depth--;
                FREE(object*, arguments);
                break;
			}
            case OP_MAKE_FUNCTION:
			{
				push_objstack(stack, VAL_AS_OBJECT(operand));
                advance(vm->top);
                break;
			}
            case OP_STORE_NAME:
            {
                object *obj = pop_objstack(stack);
                char *string = VAL_AS_STRING(operand);
                set_name(vm->top, string, obj);
                advance(vm->top);
                break;
            }
            case OP_COMPARE:
            {
                binary_comp(vm, stack, VAL_AS_INT(operand));
                advance(vm->top);
                break;
            }
            case OP_BINARY_ADD:
				binary_op(vm, stack, '+');
                advance(vm->top);
                break;
            case OP_BINARY_SUB:
                binary_op(vm, stack, '-');
                advance(vm->top);
                break;
            case OP_BINARY_MULT:
                binary_op(vm, stack, '*');
                advance(vm->top);
                break;
            case OP_BINARY_DIVIDE:
                binary_op(vm, stack, '/');
                advance(vm->top);
                break;
            case OP_NEGATE:
            {
                objprim *obj = create_new_primitive(PRIM_DOUBLE);
                vm_add_object(vm, (object*)obj);
                push_objstack(stack, (object*)obj);
                binary_op(vm, stack, '*');
                advance(vm->top);
                break;
            }
            case OP_POP:
                advance(vm->top);
                break;
            case OP_PRINT:
            {
                if (peek_objstack(stack)) {
                    print_object(pop_objstack(stack));
                }
                advance(vm->top);
                break;
            }
            case OP_RETURN:
            {
                if (vm->top->next != NULL)
                    pop_frame(&vm->top);
                else {
                    vm->top->pc = 0;
                    return INTERPRET_OK;
                }
                vm->callstackpos--;
                advance(vm->top);
#ifdef DEBUG_ARI
                printf("\n\n");
                printf("Call Stack Position is %ld\n", vm->callstackpos);
#endif
                break;
            }
        }
    }
    return INTERPRET_OK;
}

void reset_vm(VM *vm)
{
    reset_parser(&vm->analyzer);
}

void free_vm(VM *vm)
{
    reset_objstack(&vm->evalstack);
    object *current = vm->objs;
    object *next = NULL;
    while (current) {
        next = current->next;
        FREE_OBJECT(current);
        current = next;
    }
	init_objstack(&vm->evalstack);
    reset_parser(&vm->analyzer);
    reset_frame(&vm->global.local);
    FREE(VM, vm);
}

VM *init_vm(void)
{
    VM *vm = ALLOCATE(VM, 1);
    init_parser(&vm->analyzer);
    init_objstack(&vm->evalstack);
    init_module(&vm->global);
    vm->top = &vm->global.local;
    vm->objs = NULL;
    vm->num_objects = 0;
    vm->callstackpos = 0;
    return vm;
}

void print_value(value val, valtype type)
{
	switch (type) {
		case VAL_EMPTY:
			printf("%-4s", "empty");
			break;
        case VAL_INT:
            printf("%-4d", VAL_AS_INT(val));
			break;
		case VAL_DOUBLE:
			printf("%-4f", VAL_AS_DOUBLE(val));
			break;
		case VAL_STRING:
			printf("%-4s", VAL_AS_STRING(val));
			break;
        case VAL_NULL:
            printf("%-4s", "null");
            break;
        case VAL_OBJECT:
            print_object(VAL_AS_OBJECT(val));
		default:
			break;
	}
}
