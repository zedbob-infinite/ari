#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "compiler.h"
#include "debug.h"
#include "error.h"
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


void vm_push_frame(VM *vm, frame *newframe)
{
    push_frame(&vm->top, newframe);
    vm->framestackpos++;
}

int vm_pop_frame(VM *vm)
{
    frame* popped = pop_frame(&vm->top);
    int current = popped->pc;
    popped->pc = 0;
    if (popped->is_adhoc) {
        reset_frame(popped);
        FREE(frame, popped);
    }
    vm->framestackpos--;
    return current;
}

static inline void advance(frame *currentframe)
{
    currentframe->pc++;
}

static inline bool has_been_added(void *obj)
{
    object *testobj = (object*)obj;
    return testobj->accounted == true;
}

static void construct_primstring(objprim *primobj, char *_string_)
{
    int length = strlen(_string_) - 2;
    uint32_t hash = hashkey(_string_, length);
    char *takenstring = ALLOCATE(char, length + 1);
    memcpy(takenstring, _string_ + 1, length);
    takenstring[length] = '\0';

    PRIM_AS_STRING(primobj) = init_primstring(length, hash, takenstring);
}


static void vm_add_object(VM *vm, object *obj)
{
    if (has_been_added(obj))
        return;
    object *previous = vm->objs;
    obj->next = previous;
    vm->objs = obj;
    vm->num_objects++;
    obj->accounted = true;
}

static inline void delete_value(value *val, valtype type)
{
    if (type == VAL_STRING)
        FREE(char, val->val_string);
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

static inline void set_name(frame *localframe, primstring *name, object *val)
{
    objhash_set(&localframe->locals, name, val);
}

static inline object *get_name(frame *localframe, value name)
{
    frame *current = localframe;
    object *obj = NULL;
    primstring *pname = create_primstring(VAL_AS_STRING(name));
    do {
        obj = objhash_get(&current->locals, pname);
        current = current->next;
    } while ((!obj) && (current));
    free_primstring(pname);
    return obj;
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
    vm_push_frame(vm, localframe);

    for (int k = 0, i = argcount - 1; k < argcount; k++) {
#ifdef DEBUG_ARI
        printf("   \tcode object argument %d: %s\n", k + 1,
                PRIM_AS_RAWSTRING(funcobj->arguments[k]));
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
}

static void create_new_instance(VM *vm, object *obj, int argcount, object **arguments)
{
    objclass *classobj = (objclass*)obj;
    objinstance *new_instance = init_objinstance(classobj);
    
    vm->objregister = (object*)new_instance;
    /* Future code for init method will go here */
    /*primstring *name = create_primstring("__init__");
    object *prop = objhash_get(classobj->header.__attrs__, name);
    if (prop)
        call_function(vm, prop, argcount, arguments);
    else
        printf("__init__ not found...\n");*/

    push_objstack(&vm->evalstack, (object*)new_instance);
    advance(vm->top);
}

static inline void op_push_frame(VM *vm)
{
    frame *newframe = ALLOCATE(frame, 1);
    init_frame(newframe);
    newframe->pc = vm->top->pc;
    newframe->is_adhoc = true;
    vm_push_frame(vm, newframe);
    advance(vm->top);
}

static inline void op_pop_frame(VM *vm)
{
    vm->top->pc = vm_pop_frame(vm);
    advance(vm->top);
}

static inline void op_jmp_loc(VM *vm, code8 *code)
{
    vm->top->pc = VAL_AS_INT(code->operand);
}

static inline void op_jmp_after(VM *vm, code8 *code)
{
    uint64_t current = vm->top->pc;
    vm->top->pc = current + VAL_AS_INT(code->operand);
}

static inline void op_jmp_false(VM *vm, code8 *code)
{
    objprim *condition = (objprim*)pop_objstack(&vm->evalstack);
    
    if (PRIM_AS_BOOL(condition))
        advance(vm->top);
    else
        vm->top->pc = VAL_AS_INT(code->operand);
}

static inline intrpstate op_load_constant(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;

    objprim *prim = NULL;
    switch (code->operand.type) {
        case VAL_EMPTY:
            return runtime_error(vm, stack, line, "No object found.");
        case VAL_BOOL:
            prim = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(prim) = VAL_AS_BOOL(code->operand);
            break;
        case VAL_DOUBLE:
            prim = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(prim) = VAL_AS_DOUBLE(code->operand);
            break;
        case VAL_STRING:
            prim = create_new_primitive(PRIM_STRING);
            construct_primstring(prim, VAL_AS_STRING(code->operand));
            break;
        case VAL_NULL:
            prim = create_new_primitive(PRIM_NULL);
            PRIM_AS_NULL(prim) = VAL_AS_NULL(code->operand);
            break;
        default:
            return runtime_error(vm, stack, line,
                    "Cannot load non-constant value.");
    }
    object *obj = (object*)prim;
    vm_add_object(vm, obj);
    push_objstack(stack, obj);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_load_name(VM *vm, code8 *code)
{
    object *obj = get_name(vm->top, code->operand);
    if (obj)
        push_objstack(&vm->evalstack, obj);
    else
        return runtime_error_loadname(vm, VAL_AS_STRING(code->operand),
                code->line);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline void op_load_method(VM *vm)
{
    advance(vm->top);
}

static inline intrpstate op_call_function(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack; 
    int argcount = VAL_AS_INT(code->operand);
    object **arguments = ALLOCATE(object*, argcount);
#ifdef DEBUG_ARI
    printf("\n");
#endif
    for (int i = 0; i < argcount; ++i) 
        arguments[i] = pop_objstack(stack);
    
    object *popped = pop_objstack(stack);
        
    if (OBJ_IS_CLASS(popped))
        create_new_instance(vm, popped, argcount, arguments);
    else if (OBJ_IS_CODE(popped))
        call_function(vm, popped, argcount, arguments);
    else if (OBJ_IS_BUILTIN(popped)) {
        object *obj = call_builtin(vm, popped, argcount, arguments);
        advance(vm->top);
        if (obj)
            push_objstack(stack, obj);
    }
    else
        return runtime_error(vm, &vm->evalstack, code->line,
                "CallError: object is not callable");
    FREE(object*, arguments);
    return INTERPRET_OK;
}

static inline void op_make_function(VM *vm, code8 *code)
{
    object *func = VAL_AS_OBJECT(code->operand);
    push_objstack(&vm->evalstack, func);
    advance(vm->top);
}

static inline void op_make_class(VM *vm, code8 *code)
{
    objclass *classobj = (objclass*)VAL_AS_OBJECT(code->operand);
    vm_push_frame(vm, &classobj->localframe);
    execute(vm, &classobj->instructs);
    push_objstack(&vm->evalstack, VAL_AS_OBJECT(code->operand));
}

static inline void op_call_method(VM *vm, code8 *code)
{
    int argcount = VAL_AS_INT(code->operand) + 1;
    object **arguments = ALLOCATE(object*, argcount);
    
    int i = 0;
    for (i = 0; i < argcount - 1; i++)
        arguments[i] = pop_objstack(&vm->evalstack);
    
    arguments[i] = vm->objregister;
    object *popped = pop_objstack(&vm->evalstack);
    call_function(vm, popped, argcount, arguments);
    advance(vm->top);
}

static inline void op_make_method(VM *vm, code8 *code)
{
    push_objstack(&vm->evalstack, VAL_AS_OBJECT(code->operand));
    advance(vm->top);
}

static inline intrpstate op_get_property(VM *vm, code8 *code)
{
    int line = code->line;
    objstack *stack = &vm->evalstack;
    
    // Store instance in object register
    object *obj = pop_objstack(stack);
    vm->objregister = obj;

    primstring *name = create_primstring(VAL_AS_STRING(code->operand));
    switch (obj->type) {
        case OBJ_CLASS:
        {
            objclass *classobj = (objclass*)obj;
            object *prop = objhash_get(classobj->header.__attrs__,
                    name);
            if (prop)
                push_objstack(stack, prop);
            else
                return runtime_error_loadname(vm, 
                        PRIMSTRING_AS_RAWSTRING(name), line);
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
            if (!prop)
                return runtime_error_loadname(vm, 
                        PRIMSTRING_AS_RAWSTRING(name), line);
            push_objstack(stack, prop);
            break;
        }
        default:
        {
            return runtime_error(vm, stack, line,
                "Invalid Operation: object has no attributes.");
        }
    }
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_set_property(VM *vm, code8 *code)
{
    int line = code->line;
    objstack *stack = &vm->evalstack;

    object *obj = pop_objstack(&vm->evalstack);
    primstring *name = create_primstring(VAL_AS_STRING(code->operand));
    object *val = pop_objstack(&vm->evalstack);
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
                    PRIMSTRING_AS_RAWSTRING(name));
            return runtime_error(vm, stack, line, msg);
        }
    }
    advance(vm->top);
    return INTERPRET_OK;
}

static inline void op_store_name(VM *vm, code8 *code)
{
    object *obj = pop_objstack(&vm->evalstack);
    char *string = VAL_AS_STRING(code->operand);
    primstring *pstring = create_primstring(string);
    set_name(vm->top, pstring, obj);
    advance(vm->top);
}

static inline void op_compare(VM *vm, code8 *code)
{
    binary_comp(vm, &vm->evalstack, VAL_AS_INT(code->operand));
    advance(vm->top);
}

static inline intrpstate op_binary_add(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;

    object *b = pop_objstack(stack);
    object *a = pop_objstack(stack);
    object *c = NULL;

    if (!a->__add__)
        if (!b->__add__)
            return runtime_error_unsupported_operation(vm, line,
                    '+');
        else
            c = b->__add__(a, b);
    else
        c = a->__add__(a, b);

    if (!c)
        return runtime_error_unsupported_operation(vm,
                line, '+');

    vm_add_object(vm, c);
    push_objstack(stack, c);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_binary_sub(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;

    object *b = pop_objstack(stack);
    object *a = pop_objstack(stack);
    object *c = NULL;

    if (!a->__sub__)
        if (!b->__sub__)
            return runtime_error_unsupported_operation(vm, line,
                    '-');
        else
            c = b->__sub__(a, b);
    else
        c = a->__sub__(a, b);

    if (!c)
        return runtime_error_unsupported_operation(vm, line,
                '-');

    vm_add_object(vm, c);
    push_objstack(stack, c);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_binary_mult(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;
    
    object *b = pop_objstack(stack);
    object *a = pop_objstack(stack);
    object *c = NULL;

    if (!a->__mul__)
        if (!b->__mul__)
            return runtime_error_unsupported_operation(vm, line, '*');
        else
            c = b->__mul__(a, b);
    else
        c = a->__mul__(a, b);

    if (!c)
        return runtime_error_unsupported_operation(vm, line, '*');

    vm_add_object(vm, c);
    push_objstack(stack, c);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_binary_div(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;
    
    object *b = pop_objstack(stack);
    object *a = pop_objstack(stack);
    object *c = NULL;
    if (check_zero_div(a, b))
        return runtime_error_zero_div(vm, line);

    if (!a->__div__)
        if (!b->__div__)
            return runtime_error_unsupported_operation(vm, line, '/');
        else
            c = b->__div__(a, b);
    else
        c = a->__div__(a, b);

    if (!c)
        return runtime_error_unsupported_operation(vm, line, '/');

    vm_add_object(vm, c);
    push_objstack(stack, c);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_negate(VM *vm, code8 *code)
{
    objstack *stack = &vm->evalstack;
    int line = code->line;

    objprim *prim = create_new_primitive(PRIM_DOUBLE);
    PRIM_AS_DOUBLE(prim) = 1;

    vm_add_object(vm, (object*)prim);
    push_objstack(stack, (object*)prim);

    object *b = pop_objstack(stack);
    object *a = pop_objstack(stack);
    object *c = NULL;

    if (!a->__mul__)
        if (!b->__mul__)
            return runtime_error_unsupported_operation(vm,
                    line, '*');
        else
            c = b->__mul__(a, b);
    else
        c = a->__mul__(a, b);

    if (!c)
        return runtime_error_unsupported_operation(vm, line,
                '*');
    advance(vm->top);
    return INTERPRET_OK;
}

static inline void op_pop(VM *vm)
{
    pop_objstack(&vm->evalstack);
    advance(vm->top);
}

static inline intrpstate op_return(VM *vm)
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
    printf("\n");
#endif
    return INTERPRET_OK;
}

intrpstate execute(VM *vm, instruct *instructs)
{
    vm->callstackpos++;
    uint64_t count = instructs->count;
#ifdef DEBUG_ARI
    int compress = (int)log10(count);
    printf("\nCurrent Frame: %p\n", vm->top);
    printf("Frame\tInstruct   OP\t\t\toperand\n");
    printf("-----\t--------   ----------\t\t--------\n");
#endif
    while (vm->top->pc < instructs->count) {
        uint64_t current = vm->top->pc;
        code8 *code = instructs->code[current];
        value operand = code->operand;
        valtype type = code->operand.type;
#ifdef DEBUG_ARI
        printf("|%03d|\t", vm->framestackpos);
        printf("|%*ld|\t   ", compress, current + 1);
        print_bytecode(code->bytecode);
        printf("\t(");
        print_value(operand, type);
        printf(")");
        printf("\n");
#endif
        switch (code->bytecode) {
            case OP_PUSH_FRAME:
            {
                op_push_frame(vm);
                break;
            }
            case OP_POP_FRAME:
            {
                op_pop_frame(vm);
                break;
            }
            case OP_JMP_LOC:
            {
                op_jmp_loc(vm, code);
                break;
            }
            case OP_JMP_AFTER:
            {
                op_jmp_after(vm, code);
                break;
            }
            case OP_JMP_FALSE:
            {
                op_jmp_false(vm, code);
                break;
            }
            case OP_LOAD_CONSTANT:
            {
                intrpstate errcode = op_load_constant(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_LOAD_NAME:
            {
                intrpstate errcode = op_load_name(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_LOAD_METHOD:
            {
                op_load_method(vm);
                break;
            }
            case OP_CALL_FUNCTION:
            {
                intrpstate errcode = op_call_function(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_MAKE_FUNCTION:
            {
                op_make_function(vm, code);
                break;
            }
            case OP_MAKE_CLASS:
            {
                op_make_class(vm, code);
                break;
            }
            case OP_CALL_METHOD:
            {
                op_call_method(vm, code);
                break;
            }
            case OP_MAKE_METHOD:
            {
                op_make_method(vm, code);
                break;
            }
            case OP_GET_PROPERTY:
            {
                intrpstate errcode = op_get_property(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_SET_PROPERTY:
            {
                intrpstate errcode = op_set_property(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_STORE_NAME:
            {
                op_store_name(vm, code);
                break;
            }
            case OP_COMPARE:
            {
                op_compare(vm, code);
                break;
            }
            case OP_BINARY_ADD:
            {
                intrpstate errcode = op_binary_add(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_BINARY_SUB:
            {
                intrpstate errcode = op_binary_sub(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_BINARY_MULT:
            {
                intrpstate errcode = op_binary_mult(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_BINARY_DIVIDE:
            {
                intrpstate errcode = op_binary_div(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_NEGATE:
            {
                intrpstate errcode = op_negate(vm, code);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_POP:
            {
                op_pop(vm);
                break;
            }
            case OP_RETURN:
            {
                return op_return(vm);
            }
        }
    }
    return INTERPRET_OK;
}

void reset_vm(VM *vm)
{
    vm->top->pc = 0;
    reset_parser(&vm->analyzer);
}

void free_vm(VM *vm)
{
    object *current = NULL;
    object *next = NULL;
    while ((current = vm->objs)) {
        next = current->next;
        FREE_OBJECT(current);
        vm->objs = next;
    }
    reset_objstack(&vm->evalstack);
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
    vm->objregister = NULL;
    vm->num_objects = 0;
    vm->callstackpos = 0;
    vm->framestackpos = 0;

    builtin funcs[] = {builtin_println, builtin_input, builtin_type};
    char *names[] = {"print", "input", "type"};

    object *obj = NULL;
    for (int i = 0; i < 3; i++) {
        obj = load_builtin(vm, names[i], funcs[i]);
        vm_add_object(vm, obj);
    }

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
