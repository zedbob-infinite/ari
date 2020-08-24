#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "compiler.h"
#include "debug.h"
#include "instruct.h"
#include "frame.h"
#include "memory.h"
#include "objclass.h"
#include "objcode.h"
#include "objstack.h"
#include "opcode.h"
#include "token.h"
#include "tokenizer.h"
#include "vm.h"

static void call_builtin(VM *vm, object *obj, int argcount, 
        object **arguments)
{
    objbuiltin *builtinobj = (objbuiltin*)obj;
    object *result = builtinobj->func(argcount, arguments);
}

static void load_builtin(VM *vm, char *name, builtin function)
{
    objbuiltin *builtin_obj = ALLOCATE(objbuiltin, 1);
    init_object(&builtin_obj->header, OBJ_BUILTIN);
    builtin_obj->func = function;

    frame *global = &vm->global.local;
    objhash_set(&global->locals, name, (object*)builtin_obj);
}

static object *builtin_println(int argcount, object **args)
{
    for (int i = argcount - 1; i >= 0; i--)
        print_object(args[i]);
    printf("\n");
    return NULL;
}

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

static void vm_pop_frame(VM *vm)
{
    frame* popped = pop_frame(&vm->top);
    if (popped->is_adhoc) {
        reset_frame(popped);
        FREE(frame, popped);
    }
    vm->callstackpos--;
}

static void runtime_error(VM *vm, objstack *stack, size_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "[line %ld] in script\n", line);

    reset_objstack(stack);
    while (vm->callstackpos > 0)
        vm_pop_frame(vm); 
}

static void runtime_error_loadname(VM *vm, char *name, uint64_t current)
{
    char msg[100];
    sprintf(msg, "Name %s not found...", name);
    runtime_error(vm, &vm->evalstack, current, msg);
}

static char *take_string(value val, int length)
{
    char *buffer = ALLOCATE(char, length);
    buffer = memcpy(buffer, VAL_AS_STRING(val) + 1, length - 1);
    buffer[length - 2] = '\0';
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
        case OP_CALL_METHOD:
            msg = "CALL_METHOD";
            break;
        case OP_MAKE_METHOD:
            msg = "MAKE_METHOD";
            break;
        case OP_MAKE_CLASS:
            msg = "MAKE_CLASS";
            break;
        case OP_SET_PROPERTY:
            msg = "SET_PROPERTY";
            break;
        case OP_GET_PROPERTY:
            msg = "GET_PROPERTY";
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

static void create_new_instance(VM *vm, object *obj, int argcount, object **arguments)
{
    objclass *classobj = (objclass*)obj;
    objinstance *new_instance = init_objinstance(classobj);

    /* Future code for init method will go here */
	push_objstack(&vm->evalstack, (object*)new_instance);
}

static void call_function(VM *vm, object *obj, int argcount, object **arguments)
{
    objcode *funcobj = (objcode*)obj;
    frame *localframe = NULL;

    if (funcobj->depth == 0) {
        vm_add_object(vm, (object*)funcobj);    
        localframe = &funcobj->localframe;
    }
    else {
        localframe = ALLOCATE(frame, 1);
        init_frame(localframe);
        localframe->is_adhoc = true;
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
}

int execute(VM *vm, instruct *instructs)
{
    vm->callstackpos++;
    uint64_t count = instructs->count;
    frame *global = &vm->global.local;
#ifdef DEBUG_ARI
    int compress = (int)log10(count);
    printf("Current Frame is %p\n\n", vm->top);
    printf("Call Stack Position is %ld\n", vm->callstackpos);
#endif
    objstack *stack = &vm->evalstack;
    while (vm->top->pc < instructs->count) {
		uint64_t current = vm->top->pc;
		code8 *code = instructs->code[current];
        value operand = code->operand;
        valtype type = code->operand.type; 
#ifdef DEBUG_ARI
        printf("|%*ld|\t", compress, current + 1);
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
                newframe->pc = vm->top->pc;
                push_frame(&vm->top, newframe); 
                advance(vm->top);
                break;
            }
            case OP_POP_FRAME:
            {
                frame *oldframe = pop_frame(&vm->top);
                vm->top->pc = oldframe->pc;
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
                        runtime_error(vm, stack, current, "No object found.");
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
						runtime_error(vm, stack, current, 
                                "Cannot load non-constant value.");
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
                    char msg[100];
                    sprintf(msg, "Name %s not found...",
                                VAL_AS_STRING(operand));
					runtime_error(vm, stack, current, msg);
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
                    printf("\n");
#endif
				}
#ifdef DEBUG_ARI
                printf("\n");
#endif
                object *popped = pop_objstack(stack);

                if (OBJ_IS_CLASS(popped))
                    create_new_instance(vm, popped, argcount, arguments);
                else if (OBJ_IS_CODE(popped))
                    call_function(vm, popped, argcount, arguments);
                else if (OBJ_IS_BUILTIN(popped))
                    call_builtin(vm, popped, argcount, arguments);
                advance(vm->top);
                break;
			}
            case OP_MAKE_FUNCTION:
			{
				push_objstack(stack, VAL_AS_OBJECT(operand));
                advance(vm->top);
                break;
			}
            case OP_MAKE_CLASS:
            {
				objclass *classobj = (objclass*)VAL_AS_OBJECT(operand);
                push_frame(&vm->top, &classobj->localframe);
                execute(vm, &classobj->instructs);
                push_objstack(stack, VAL_AS_OBJECT(operand));
                break;
            }
            case OP_CALL_METHOD:
            {
                advance(vm->top);
                break;
            }
            case OP_MAKE_METHOD:
            {
				push_objstack(stack, VAL_AS_OBJECT(operand));
                advance(vm->top);
                break;
            }
            case OP_GET_PROPERTY:
            {
                object *obj = pop_objstack(stack);
                char *name = VAL_AS_STRING(operand);
                switch (obj->type) {
                    case OBJ_CLASS:
                    {
                        objclass *classobj = (objclass*)obj;
                        object *prop = objhash_get(classobj->header.__attrs__, 
                                name);
                        if (prop)
                            push_objstack(stack, prop);
                        else {
                            runtime_error_loadname(vm, name, current);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        break;
                    }
                    case OBJ_INSTANCE:
                    {
                        objinstance *instobj = (objinstance*)obj;
                        object *prop = objhash_get(instobj->header.__attrs__, 
                                name);
                        if (!prop) {
                            objclass *classobj = instobj->class;
                            prop = objhash_get(classobj->header.__attrs__,
                                    name);
                        }
                        if (!prop) {
                            runtime_error_loadname(vm, name, current);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        push_objstack(stack, prop);
                        break;
                    }
                    default:
                    {
                        runtime_error(vm, stack, current, 
                            "Invalid Operation: object has no attributes.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                advance(vm->top);
                break;
            }
            case OP_SET_PROPERTY:
            {
                object *obj = pop_objstack(stack);
                char *name = VAL_AS_STRING(operand);
                object *val = pop_objstack(stack);
                switch (obj->type) {
                    case OBJ_CLASS:
                    {
                        objclass *classobj = (objclass*)obj;
                        objhash_set(classobj->header.__attrs__, name, val);
                        break;
                    }
                    case OBJ_INSTANCE:
                    {
                        objinstance *instobj = (objinstance*)obj;
                        objhash_set(instobj->header.__attrs__, name, val);
                        break;
                    }
                    default:
                    {
                        char msg[100];
                        sprintf(msg, "Error: object has no attribute %s.",
                                name);
                        runtime_error(vm, stack, current, msg);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
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
                    printf("\n");
                }
                advance(vm->top);
                break;
            }
            case OP_RETURN:
            {
                if (vm->top->next) {
                    vm_pop_frame(vm);
                    advance(vm->top);
                }
                else {
                    vm->callstackpos--;
                    vm->top->pc = 0;
                }
#ifdef DEBUG_ARI
                printf("Returning to Call Stack: %ld\n", vm->callstackpos);
                printf("\n");
#endif
                return INTERPRET_OK;
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
    
    load_builtin(vm, "println", builtin_println);
    
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
