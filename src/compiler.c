#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "object.h"
#include "objclass.h"
#include "objcode.h"
#include "opcode.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

static void compile_statement(instruct *instructs, stmt *statement);
static void compile_block(instruct *instructs, stmt *statement, 
        bool makeframe);
static void start_compile(instruct *instructs, stmt **statements, 
        int num_statements);


static char *take_string(token *tok)
{
    int length = tok->length;
    char *buffer = ALLOCATE(char, length + 1);
    memcpy(buffer, tok->start, tok->length);
    buffer[tok->length] = '\0';
    return buffer;
}

static void create_primobj_from_token(objprim *primobj, token *tok)
{
    int length = tok->length;
    char *takenstring = ALLOCATE(char, length + 1);
    memcpy(takenstring, tok->start, tok->length);
    takenstring[tok->length] = '\0';

    uint32_t hash = hashkey(takenstring, length);
    PRIM_AS_STRING(primobj) = init_primstring(length,  hash, takenstring);
}

static void patch_jump(instruct *instructs, int location, int jump)
{
    code8 *code = instructs->code[location];
    value *operand = &code->operand;
    if (code)
        VAL_AS_INT(operand) = jump;
}

static void check_instruct_capacity(instruct *instructs)
{
    int oldcapacity = instructs->capacity;
    instructs->capacity = GROW_CAPACITY(oldcapacity);
    instructs->code = GROW_ARRAY(instructs->code, code8*, oldcapacity, 
            instructs->capacity);
    for (int i = oldcapacity; i < instructs->capacity; ++i)
        instructs->code[i] = NULL;
}

static code8 *create_code(uint8_t bytecode, value operand, int line)
{
    code8 *code = ALLOCATE(code8, 1);
    code->bytecode = bytecode;
    code->operand = operand;
    code->line = line;
    return code;
}   

static int emit_instruction(instruct *instructs, uint8_t bytecode, 
        value operand, int line)
{
    int current = instructs->count;
    if (instructs->capacity < instructs->count + 1)
        check_instruct_capacity(instructs);

    instructs->code[instructs->count++] = create_code(bytecode, operand, 
            line);
    return current;
}

static void compile_expression(instruct *instructs, expr *expression, 
        int line)
{
    uint8_t byte = 0;
    value empty = {.type = VAL_EMPTY, .val_int = 0};
    value *operand = &empty;
    switch (expression->type) {
        case EXPR_ASSIGN: 
        {
            byte = OP_STORE_NAME;
            expr_assign *assign_expr = (expr_assign*)expression;
            token *name = assign_expr->name;

            compile_expression(instructs, assign_expr->value, line);
            VAL_AS_STRING(operand) = take_string(name);
            operand->type = VAL_STRING;
            break;
        }
        case EXPR_BINARY:
        {
            expr_binary *binary_expr = (expr_binary*)expression;
            compile_expression(instructs, binary_expr->left, line);
            compile_expression(instructs, binary_expr->right, line);
            switch (binary_expr->operator->type) {
                case TOKEN_PLUS:
                    byte = OP_BINARY_ADD;
                    break;
                case TOKEN_MINUS:
                    byte = OP_BINARY_SUB;
                    break;
                case TOKEN_SLASH:
                    byte = OP_BINARY_DIVIDE;
                    break;
                case TOKEN_STAR:
                    byte = OP_BINARY_MULT;
                    break;
                case TOKEN_EQUAL_EQUAL:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQUAL:
                case TOKEN_LESS:
                case TOKEN_LESS_EQUAL:
                    byte = OP_COMPARE;
                    VAL_AS_INT(operand) = binary_expr->operator->type;
                    operand->type = VAL_BOOL;
                    break;
                default:
                    break;
            }
            break;
        }
        case EXPR_GROUPING:
        {
            expr_grouping *grouping_expr = (expr_grouping*)expression;
            compile_expression(instructs, grouping_expr->expression, line);
            return;
        }
        case EXPR_LITERAL_STRING:
        {
            byte = OP_LOAD_CONSTANT;
            operand->type = VAL_STRING;
            expr_literal *literal_expr = (expr_literal*)expression;
            int length = strlen(literal_expr->literal);
            char *buffer = ALLOCATE(char, length + 1);
            
            strncpy(buffer, literal_expr->literal, length);
            buffer[length] = '\0';
            VAL_AS_STRING(operand) = buffer;
            break;
        }
        case EXPR_LITERAL_NUMBER:
        {
            byte = OP_LOAD_CONSTANT;
            expr_literal *literal_expr = (expr_literal*)expression;

            VAL_AS_DOUBLE(operand) = atof(literal_expr->literal);
            operand->type = VAL_DOUBLE;
            break;
        }
        case EXPR_LITERAL_BOOL:
        {
            byte = OP_LOAD_CONSTANT;
            expr_literal *literal_expr = (expr_literal*)expression;

            VAL_AS_INT(operand) = atoi(literal_expr->literal);
            operand->type = VAL_BOOL;
            break;
        }
        case EXPR_LITERAL_NULL:
        {
            byte = OP_LOAD_CONSTANT;
            operand->type = VAL_NULL;
            break;
        }
        case EXPR_UNARY:
        {
            expr_unary *unary_expr = (expr_unary*)expression;
            switch (unary_expr->operator->type) {
                case TOKEN_BANG:
                    break;
                case TOKEN_MINUS:
                    compile_expression(instructs, unary_expr->right, line);
                    byte = OP_NEGATE;
                    break;
                default:
                    // future error code here
                    break;
            }
            break;
        }
        case EXPR_VARIABLE:
        {
            byte = OP_LOAD_NAME;
            operand->type = VAL_STRING;
            expr_var *var_expr = (expr_var*)expression;
            token *name = var_expr->name;

            VAL_AS_STRING(operand) = take_string(name);
            break;
        }
        case EXPR_CALL:
        {
            operand->type = VAL_INT;
            
            expr_call *call_expr = (expr_call*)expression;
            
            if (call_expr->is_method)
                byte = OP_CALL_METHOD;
            else {
                byte = OP_CALL_FUNCTION;
                compile_expression(instructs, call_expr->expression, line);
            }
            
            int i = 0;
            for (i = 0; i < call_expr->count; i++)
                compile_expression(instructs, call_expr->arguments[i], line);
            
            /* argument count is passed as the operand */
            VAL_AS_INT(operand) = i;
            break;
        }
        case EXPR_METHOD:
        {
            byte = OP_LOAD_METHOD;

            expr_method *method_expr = (expr_method*)expression;

            compile_expression(instructs, method_expr->refobj, line);
            compile_expression(instructs, method_expr->call, line);

            break;
        }
        case EXPR_GET_PROP:
        {
            byte = OP_GET_PROPERTY;
            operand->type = VAL_STRING;
            
            expr_get *get_expr = (expr_get*)expression;
            token *name = get_expr->name;

            VAL_AS_STRING(operand) = take_string(name);
            compile_expression(instructs, get_expr->refobj, line);
            break;
        }
        case EXPR_SET_PROP:
        {
            byte = OP_SET_PROPERTY;
            operand->type = VAL_STRING;
            expr_set *set_expr = (expr_set*)expression;
            token *name = set_expr->name;
            
            VAL_AS_STRING(operand) = take_string(name);

            /* Put new value on stack */
            compile_expression(instructs, set_expr->value, line);
            /* Put reference object on stack */
            compile_expression(instructs, set_expr->refobj, line);
            
            break;
        }
        case EXPR_SOURCE:
        {
            byte = OP_GET_SOURCE;
            operand->type = VAL_STRING;
            expr_source *source_expr = (expr_source*)expression;
            token *name = source_expr->name;

            VAL_AS_STRING(operand) = take_string(name);
            break;
        }
    }
    emit_instruction(instructs, byte, *operand, line);
}

static void compile_expression_stmt(instruct *instructs, stmt *statement)
{
    stmt_expr *expr_stmt = (stmt_expr*)statement;
    compile_expression(instructs, expr_stmt->expression, 
            statement->line);
}

static void compile_block(instruct *instructs, stmt *statement, bool makeframe)
{
    stmt_block *block_stmt = (stmt_block*)statement;
    
    if (makeframe)
        emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL, 
                statement->line);

    int i = 0;
    stmt *current = NULL;
    while ((current = block_stmt->stmts[i++]))
        compile_statement(instructs, current);

    if (makeframe)
        emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL, statement->line);
}

static void compile_if(instruct *instructs, stmt *statement)
{
    int line = statement->line;
    stmt_if *if_stmt = (stmt_if*)statement;
    int jmpfalse = 0;

    compile_expression(instructs, if_stmt->condition, line);
    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL, line);

    compile_statement(instructs, if_stmt->thenbranch);

    if (if_stmt->elsebranch) {
        patch_jump(instructs, jmpfalse, instructs->count);
        compile_statement(instructs, if_stmt->elsebranch);
    }
    else 
        patch_jump(instructs, jmpfalse, instructs->count);
}

static void compile_while(instruct *instructs, stmt *statement)
{
    int line = statement->line;
    stmt_while *while_stmt = (stmt_while*)statement;
    int compare = 0;
    int jmpfalse = 0;
    int jmpbegin = 0;

    compare = instructs->count;

    compile_expression(instructs, while_stmt->condition, line);

    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL, line);
    compile_block(instructs, while_stmt->loopbody, false);
    
    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL, line);
    patch_jump(instructs, jmpbegin, compare);
    patch_jump(instructs, jmpfalse, instructs->count);
}

static void compile_for(instruct *instructs, stmt *statement)
{
    stmt_for *for_stmt = (stmt_for*)statement;
    int forbegin = 0;
    int jmpbegin = 0;
    int jmpfalse = 0;
    
    emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL, statement->line);

    // Initializer_statement
    compile_statement(instructs, for_stmt->stmts[0]);
    // thenbranch used for compare statement
    forbegin = instructs->count;
    compile_statement(instructs, for_stmt->stmts[1]);
    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL, 
            statement->line);
    // loop body
    compile_block(instructs, for_stmt->loopbody, false);
    // elsebranch used for iterator statement
    compile_statement(instructs, for_stmt->stmts[2]);

    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL, 
            statement->line);

    // patch the jump instructs
    patch_jump(instructs, jmpbegin, forbegin);
    patch_jump(instructs, jmpfalse, instructs->count);
    
    emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL, statement->line);
}

static void compile_function(instruct *instructs, stmt *statement)
{
    stmt_function *function_stmt = (stmt_function*)statement;
    
    token *name = function_stmt->name;
    int argcount = function_stmt->num_parameters;
    token **parameters = function_stmt->parameters;

    objprim **arguments = ALLOCATE(objprim*, argcount);

    for (int i = 0; i < argcount; ++i) {
        objprim *prim = create_new_primitive(PRIM_STRING);
        create_primobj_from_token(prim, parameters[i]);
        arguments[i] = prim;
    }
    
    objcode *codeobj = init_objcode(argcount, arguments);
    codeobj->name = take_string(name);

    compile_block(&(codeobj->instructs), function_stmt->block, false);

    emit_instruction(&(codeobj->instructs), OP_RETURN, NULL_VAL, 
            statement->line);
    /* Push new code object onto the stack */
    value valobj = {.type = VAL_OBJECT, .val_obj = (object*)codeobj};
    emit_instruction(instructs, OP_MAKE_FUNCTION, valobj, statement->line);

    /* Store object*/
    value operand = {.type = VAL_STRING, 
        .val_string = take_string(name)};
    emit_instruction(instructs, OP_STORE_NAME, operand, statement->line);
}

static void compile_method(instruct *instructs, stmt *statement)
{
    stmt_method *method_stmt = (stmt_method*)statement;
    int argcount = method_stmt->num_parameters + 1;
    token **parameters = method_stmt->parameters;

    objprim **arguments = ALLOCATE(objprim*, argcount);

    // 'this' is the first implicit argument for any method
    objprim *_this_ = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(_this_) = create_primstring("this");
    arguments[0] = _this_;

    for (int i = 1; i < argcount; ++i) {
        objprim *prim = create_new_primitive(PRIM_STRING);
        create_primobj_from_token(prim, parameters[i - 1]);
        arguments[i] = prim;
    }
    
    objcode *codeobj = init_objcode(argcount, arguments);

    token *name = method_stmt->name;
    codeobj->name = take_string(name);
    
    /* Compile method body */
    compile_block(&(codeobj->instructs), method_stmt->block, false);

    emit_instruction(&(codeobj->instructs), OP_RETURN, NULL_VAL,
            statement->line);
    /* Push new code object onto the stack */
    value valobj = {.type = VAL_OBJECT, .val_obj = (object*)codeobj};
    emit_instruction(instructs, OP_MAKE_METHOD, valobj, 
            statement->line);

    /* Store object */
    value operand = {.type = VAL_STRING, 
        .val_string = take_string(name)};
    emit_instruction(instructs, OP_STORE_NAME, operand, 
            statement->line);
}

static void compile_class(instruct *instructs, stmt *statement)
{
    stmt_class *class_stmt = (stmt_class*)statement;
    objclass *classobj = init_objclass();
    
    token *name = class_stmt->name;
    char *classname = take_string(name);
    classobj->name = create_primstring(classname);
    FREE(char, classname);
    /* Process attributes and methods */
    size_t num_attributes = class_stmt->num_attributes;
    size_t num_methods = class_stmt->num_methods;

    for (size_t i = 0; i < num_attributes; i++)
        compile_statement(&classobj->instructs, class_stmt->attributes[i]);
    
    for (size_t i = 0; i < num_methods; i++)
        compile_statement(&classobj->instructs, class_stmt->methods[i]);

    emit_instruction(&classobj->instructs, OP_RETURN, EMPTY_VAL, 
            statement->line);
    
    /* Push new code object onto the stack */
    value valobj = {.type = VAL_OBJECT, .val_obj = (object*)classobj};
    emit_instruction(instructs, OP_MAKE_CLASS, valobj, 
            statement->line);

    /* Store object*/
    value operand = {.type = VAL_STRING, 
        .val_string = take_string(name)};
    emit_instruction(instructs, OP_STORE_NAME, operand, statement->line);
}

static void compile_return(instruct *instructs, stmt *statement)
{
    stmt_return *return_stmt = (stmt_return*)statement;
    compile_expression(instructs, return_stmt->value, 
            statement->line);
    emit_instruction(instructs, OP_RETURN, EMPTY_VAL, statement->line);
}

static void compile_statement(instruct *instructs, stmt *statement)
{
    switch (statement->type) {
        case STMT_EXPR:
        {
            compile_expression_stmt(instructs, statement);
            break;
        }
        case STMT_BLOCK:
        {
            compile_block(instructs, statement, true);
            break;
        }
        case STMT_IF:
        {
            compile_if(instructs, statement);
            break;
        }
        case STMT_WHILE:
        {
            compile_while(instructs, statement);
            break;
        }
        case STMT_FOR:
        {
            compile_for(instructs, statement);
            break;
        }
        case STMT_FUNCTION:
        {
            compile_function(instructs, statement);
            break;
        }
        case STMT_METHOD:
        {
            compile_method(instructs, statement);
            break;
        }
        case STMT_CLASS:
        {
            compile_class(instructs, statement);
            break;
        }
        case STMT_RETURN:
        {
            compile_return(instructs, statement);
            break;
        }
    }
}

static void start_compile(instruct *instructs, stmt **statements, int num_statements)
{
    // Compile parse trees into bytecode
    for (int i = 0; i < num_statements; i++) {
        if (instructs->capacity < instructs->count + 1)
            check_instruct_capacity(instructs);
        compile_statement(instructs, statements[i]);
    }
}

instruct compile(parser *analyzer, const char *source)
{
    instruct instructs;
    init_instruct(&instructs);

    if (parse(analyzer, source)) {
        reset_parser(analyzer);
        return instructs;
    }
    start_compile(&instructs, analyzer->statements, analyzer->num_statements);

    emit_instruction(&instructs, OP_RETURN, EMPTY_VAL, 
            analyzer->num_statements);

    return instructs;
}
