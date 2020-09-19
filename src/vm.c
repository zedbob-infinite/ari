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

static inline void set_name(frame *localframe, primstring *name, object *val)
{
    objhash_set(&localframe->locals, name, val);
}

static inline object *get_name(frame *localframe, char *name)
{
    frame *current = localframe;
    object *obj = NULL;
    primstring *pname = create_primstring(name);
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
    primstring *name = create_primstring("__init__");
    object *prop = objhash_get(classobj->header.__attrs__, name);
    arguments[argcount] = vm->objregister;
    if (prop)
        call_function(vm, prop, argcount + 1, arguments);
    else
        advance(vm->top);

    push_objstack(&vm->evalstack, (object*)new_instance);
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

static inline void op_jmp_loc(VM *vm, int jump)
{
    vm->top->pc = jump;
}

static inline void op_jmp_after(VM *vm, int jump)
{
    uint64_t current = vm->top->pc;
    vm->top->pc = current + jump;
}

static inline void op_jmp_false(VM *vm, int jump, objprim *condition)
{ 
    if (PRIM_AS_BOOL(condition))
        advance(vm->top);
    else
        vm->top->pc = jump;
}

static inline intrpstate op_load_constant(VM *vm, int line, int type, value *constant)
{
    objstack *stack = &vm->evalstack;

    objprim *prim = NULL;
    switch (type) {
        case VAL_EMPTY:
            return runtime_error(vm, stack, line, "No object found.");
        case VAL_BOOL:
            prim = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(prim) = VAL_AS_BOOL(constant);
            break;
        case VAL_DOUBLE:
            prim = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(prim) = VAL_AS_DOUBLE(constant);
            break;
        case VAL_STRING:
            prim = create_new_primitive(PRIM_STRING);
            construct_primstring(prim, VAL_AS_STRING(constant));
            break;
        case VAL_NULL:
            prim = create_new_primitive(PRIM_NULL);
            PRIM_AS_NULL(prim) = VAL_AS_NULL(constant);
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

static inline intrpstate op_load_name(VM *vm, int line, char *name)
{
    object *obj = get_name(vm->top, name);
    if (obj)
        push_objstack(&vm->evalstack, obj);
    else
        return runtime_error_loadname(vm, name, line);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline void op_load_method(VM *vm)
{
    advance(vm->top);
}

static inline intrpstate op_call_function(VM *vm, int line, int argcount)
{
    objstack *stack = &vm->evalstack; 
    object **arguments = ALLOCATE(object*, argcount + 1);
#ifdef DEBUG_ARI
    printf("\n");
#endif
    for (int i = 0; i < argcount; ++i) 
        arguments[i] = pop_objstack(stack);
    
    object *popped = pop_objstack(stack);

    if (!popped)
        return runtime_error(vm, &vm->evalstack, line,
                "CallError: object is not callable");

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
        return runtime_error(vm, &vm->evalstack, line,
                "CallError: object is not callable");
    FREE(object*, arguments);
    return INTERPRET_OK;
}

static inline void op_make_function(VM *vm, value *operand)
{
    object *func = VAL_AS_OBJECT(operand);
    push_objstack(&vm->evalstack, func);
    advance(vm->top);
}

static inline void op_make_class(VM *vm, value *operand)
{
    objclass *classobj = (objclass*)VAL_AS_OBJECT(operand);
    vm_push_frame(vm, &classobj->localframe);
    execute(vm, &classobj->instructs);
    push_objstack(&vm->evalstack, VAL_AS_OBJECT(operand));
}

static inline void op_call_method(VM *vm, int argcount)
{
    object **arguments = ALLOCATE(object*, argcount);
    
    int i = 0;
    for (i = 0; i < argcount - 1; i++)
        arguments[i] = pop_objstack(&vm->evalstack);
    
    arguments[i] = vm->objregister;
    object *popped = pop_objstack(&vm->evalstack);
    call_function(vm, popped, argcount, arguments);
    advance(vm->top);
}

static inline void op_make_method(VM *vm, value *operand)
{
    push_objstack(&vm->evalstack, VAL_AS_OBJECT(operand));
    advance(vm->top);
}

static inline intrpstate op_get_property(VM *vm, int line, char *getname)
{
    objstack *stack = &vm->evalstack;
    
    // Store instance in object register
    object *obj = pop_objstack(stack);
    vm->objregister = obj;

    primstring *name = create_primstring(getname);
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

static inline intrpstate op_set_property(VM *vm, int line, char *setname)
{
    objstack *stack = &vm->evalstack;

    object *obj = pop_objstack(&vm->evalstack);
    primstring *name = create_primstring(setname);
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

static inline intrpstate op_get_source(VM *vm, int line, char *name)
{
    primstring *modname = create_primstring(name);
    /* Code to create module here */
    advance(vm->top);
    return INTERPRET_OK;
}
static inline void op_store_name(VM *vm, char *name)
{
    object *obj = pop_objstack(&vm->evalstack);
    primstring *pname = create_primstring(name);
    set_name(vm->top, pname, obj);
    advance(vm->top);
}

static inline intrpstate op_compare(VM *vm, int line, int cmptype)
{
    objprim *b = (objprim*)pop_objstack(&vm->evalstack);
    objprim *a = (objprim*)pop_objstack(&vm->evalstack);
    
    if ((!a) || (!b))
        return runtime_error(vm, &vm->evalstack, line,
                "ComparisonError: object not found");

    object *obj = binary_comp(a, b, cmptype);
    
    vm_add_object(vm, obj);
    push_objstack(&vm->evalstack, obj);
    advance(vm->top);
    return INTERPRET_OK;
}

static inline intrpstate op_binary_add(VM *vm, int line)
{
    objstack *stack = &vm->evalstack;

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

static inline intrpstate op_binary_sub(VM *vm, int line)
{
    objstack *stack = &vm->evalstack;

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

static inline intrpstate op_binary_mult(VM *vm, int line)
{
    objstack *stack = &vm->evalstack;
    
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

static inline intrpstate op_binary_div(VM *vm, int line)
{
    objstack *stack = &vm->evalstack;
    
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

static inline intrpstate op_negate(VM *vm, int line)
{
    objstack *stack = &vm->evalstack;

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
    else
        vm->top->pc = 0;
#ifdef DEBUG_ARI
    printf("\n");
#endif
    return INTERPRET_OK;
}

intrpstate execute(VM *vm, instruct *instructs)
{
    if (vm->framestackpos == 0)
        if (vm->haderror)
            vm->haderror = false;
    objstack *stack = &vm->evalstack;
#ifdef DEBUG_ARI
    uint64_t count = instructs->count;
    int compress = (int)log10(count);
    printf("\nCurrent Frame: %p\n", vm->top);
    printf("Frame\tInstruct   OP\t\t\toperand\n");
    printf("-----\t--------   ----------\t\t--------\n");
#endif
    while (vm->top->pc < instructs->count) {
        uint64_t current = vm->top->pc;
        code8 *code = instructs->code[current];
        int line = code->line;
        if (vm->haderror) {
            fprintf(stderr, "[line %d] in script\n", line);
            return INTERPRET_RUNTIME_ERROR;
        }
        value *operand = &code->operand;
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
            /* PUSH_FRAME: Pushes an adhoc frame onto the frame stack. 
             * Adhoc frames are used to create new scopes outside of a 
             * new function or class.
             */
            case OP_PUSH_FRAME:
            {
                op_push_frame(vm);
                break;
            }
            /* POP_FRAME: Pops an adhoc frame from the frame stack.
             */
            case OP_POP_FRAME:
            {
                op_pop_frame(vm);
                break;
            }
            /* JMP_LOC: Directly jumps to a new location in 
             * the instructions.
             */
            case OP_JMP_LOC:
            {
                int jump = VAL_AS_INT(operand);
                op_jmp_loc(vm, jump);
                break;
            }
            /* JMP_AFTER: Uses an offset to make a jump in
             * the instructions.
             */
            case OP_JMP_AFTER:
            {
                int jump = VAL_AS_INT(operand);
                op_jmp_after(vm, jump);
                break;
            }
            /* JMP_FALSE: Only jump if condition is true. Note
             * the other jump operations do not directly evalute
             * whether to jump based on conditions.
             */
            case OP_JMP_FALSE:
            {
                objprim *condition = (objprim*)pop_objstack(stack);
                if (!condition)
                    return runtime_error(vm, stack, line,
                            "ConditionError: No condition found.");
                int jump = VAL_AS_INT(operand);
                op_jmp_false(vm, jump, condition);
                break;
            }
            /* LOAD_CONSTANT: Takes a value from the compiler
             * and creates a new objprim object and pushes it
             * onto the object stack.
             */
            case OP_LOAD_CONSTANT:
            {
                intrpstate errcode = op_load_constant(vm, line, type, 
                        operand);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* LOAD_NAME: searches through the linked object
             * hashtables to find an entry. If found, it places
             * that object onto the stack.
             */
            case OP_LOAD_NAME:
            {
                char *name =VAL_AS_STRING(operand);
                intrpstate errcode = op_load_name(vm, line, name);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* LOAD_METHOD: This operation only advances the 
             * program counter at this point. May be removed later on.
             */
            case OP_LOAD_METHOD:
            {
                op_load_method(vm);
                break;
            }
            /* CALL_FUNCTION: Call operation used to call functions
             * and create objects.
             */ 
            case OP_CALL_FUNCTION:
            {
                int argcount = VAL_AS_INT(operand);
                intrpstate errcode = op_call_function(vm, line, argcount);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* MAKE_FUNCTION: Takes a function passed from the
             * compiler and places it on object stack.
             */
            case OP_MAKE_FUNCTION:
            {
                op_make_function(vm, operand);
                break;
            }
            /* MAKE_CLASS: Takes a class passed from the
             * compiler and passes the class body to execute()
             * to transform class definition into a new class.
             * Afterwards, class is placed on the object stack.
             */
            case OP_MAKE_CLASS:
            {
                op_make_class(vm, operand);
                break;
            }
            /* CALL_METHOD: Similar to CALL_FUNCTION, this
             * operation calls a method instead of a function. The 
             * biggest difference between the two are that 
             * OP_CALL_METHOD places the object the method is called 
             * against in the object register.
             *
             * This allows the method to use 'this' in the method body,
             * and get attributes directly from the instance.
             */
            case OP_CALL_METHOD:
            {
                int argcount = VAL_AS_INT(operand) + 1;
                op_call_method(vm, argcount);
                break;
            }
            /* MAKE_METHOD: Takes a method constructed in the compiler
             * and places it on the object stack.
             */
            case OP_MAKE_METHOD:
            {
                op_make_method(vm, operand);
                break;
            }
            /* GET_PROPERTY: Similar to LOAD_NAME, this operation
             * attempts to retrieve an attribute from an object.
             *
             * This is done differently than LOAD_NAME, however, as
             * it looks only at the hashtable of the object, and doesn't
             * search the frame stack at all.  
             */
            case OP_GET_PROPERTY:
            {
                char *name = VAL_AS_STRING(operand);
                intrpstate errcode = op_get_property(vm, line, name);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* SET_PROPERTY: Pops an object from the object stack
             * and stores it in the hashtable of the object that this
             * operation was called against.
             *
             * e.g.: foo.bar = "Hello!"; 
             */
            case OP_SET_PROPERTY:
            {
                char *name = VAL_AS_STRING(operand);
                intrpstate errcode = op_set_property(vm, line, name);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            case OP_GET_SOURCE:
            {
                char *name = VAL_AS_STRING(operand);
                intrpstate errcode = op_get_source(vm, line, name);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* STORE_NAME: Pops an object from the object stack
             * and stores it in the hashtable of top frame on the
             * frame stack.
             *
             * e.g.:
             *          // Stored at global hashtable
             *          foo = "happy!";
             *          {
             *              // Stored at hashtable created by new scope
             *              bar = "sad!";
             *          }
             */
            case OP_STORE_NAME:
            {
                char *name = VAL_AS_STRING(operand);
                op_store_name(vm, name);
                break;
            }
            /* COMPARE: takes two objprims, compares them and returns
             * objprim bool object of either true or false.
             *
             * Right now this is limited to objprims, but will
             * be working to expand this to all objects soon.
             */
            case OP_COMPARE:
            {
                int cmptype = VAL_AS_INT(operand);
                intrpstate errcode = op_compare(vm, line, cmptype);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* BINARY_ADD: takes two obprims, adds them together
             * and places the result on the object stack.
             *
             * Will be adding functionality to allow user-defined 
             * binary_add operations for any non built-in object.
             */
            case OP_BINARY_ADD:
            {
                intrpstate errcode = op_binary_add(vm, line);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* BINARY_SUB: takes two objprims, subtracts them from
             * each other, and places the result on the object stack.
             *
             * Like BINARY_ADD, more functionality coming soon.
             */
            case OP_BINARY_SUB:
            {
                intrpstate errcode = op_binary_sub(vm, line);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* BINARY_MULT: takes two objprims, multiplies them together, 
             * and places the result on the object stack.
             *
             * Like BINARY_ADD, more functionality coming soon.
             */
            case OP_BINARY_MULT:
            {
                intrpstate errcode = op_binary_mult(vm, line);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* BINARY_DIVIDE: takes two objprims, divides them, 
             * and places the result on the object stack.
             *
             * Like BINARY_ADD, more functionality coming soon.
             */
            case OP_BINARY_DIVIDE:
            {
                intrpstate errcode = op_binary_div(vm, line);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* NEGATE: takes an objprim, negates it
             * and places the result on the object stack.
             *
             */
            case OP_NEGATE:
            {
                intrpstate errcode = op_negate(vm, line);
                if (errcode != INTERPRET_OK)
                    return errcode;
                break;
            }
            /* POP: pops the object stack.
             *
             * Note: this discards the object on the stack.
             */
            case OP_POP:
            {
                op_pop(vm);
                break;
            }
            /* RETURN: Ends an ari function or method and returns
             * to original caller.
             */
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
    vm->framestackpos = 0;
    vm->haderror = false;

    builtin funcs[] = {builtin_println, builtin_input, builtin_type, 
                       builtin_clock};
    char *names[] = {"print", "input", "type", "clock"};

    object *obj = NULL;
    for (int i = 0; i < 4; i++) {
        obj = load_builtin(vm, names[i], funcs[i]);
        vm_add_object(vm, obj);
    }

    return vm;
}

void print_value(value *val, valtype type)
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
