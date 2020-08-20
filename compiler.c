#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "object.h"
#include "objcode.h"
#include "opcode.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

static void compile_statement(instruct *instructs, stmt *statement);
static void compile_block(instruct *instructs, stmt *statement, 
        bool makeframe);

static void start_compile(instruct *instructs, stmt **statements, int num_statements);

static void patch_jump(instruct *instructs, int location, int jump)
{
    code8 *code = instructs->code[location];
    if (code)
        VAL_AS_INT(code->operand) = jump;
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

static char *take_string(token *tok)
{
    int length = tok->length;
    char *buffer = ALLOCATE(char, length + 1);
    buffer = strncpy(buffer, tok->start, tok->length);
    buffer[tok->length] = '\0';
    return buffer;
}

static code8 *create_code(uint8_t bytecode, value operand)
{
	code8 *code = ALLOCATE(code8, 1);
	code->bytecode = bytecode;
	code->operand = operand;
	return code;
}	

static int emit_instruction(instruct *instructs, uint8_t bytecode, value operand)
{
	int current = instructs->count;
	if (instructs->capacity < instructs->count + 1)
		check_instruct_capacity(instructs);

	instructs->code[instructs->count++] = create_code(bytecode, operand);
	return current;
}

static void compile_expression(instruct *instructs, expr *expression)
{
    uint8_t byte = 0;
    value operand = {.type = VAL_EMPTY, .val_int = 0};
    switch (expression->type) {
        case EXPR_ASSIGN: 
        {
            expr_assign *assign_expr = (expr_assign*)expression;
            byte = OP_STORE_NAME;
            compile_expression(instructs, assign_expr->value);
			VAL_AS_STRING(operand) = take_string(assign_expr->name);
			operand.type = VAL_STRING;
            break;
        }
        case EXPR_BINARY:
		{
            expr_binary *binary_expr = (expr_binary*)expression;
            compile_expression(instructs, binary_expr->left);
            compile_expression(instructs, binary_expr->right);
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
					operand.type = VAL_INT;
                    break;
                default:
                    break;
			}
            break;
		}
        case EXPR_GROUPING:
        {
            expr_grouping *grouping_expr = (expr_grouping*)expression;
            compile_expression(instructs, grouping_expr->expression);
            return;
        }
		case EXPR_LITERAL_STRING:
		{
            expr_literal *literal_expr = (expr_literal*)expression;
            byte = OP_LOAD_CONSTANT;
            int length = strlen(literal_expr->literal);
            char *buffer = ALLOCATE(char, length + 1);
            buffer = strncpy(buffer, literal_expr->literal, length);
            buffer[length] = '\0';
			VAL_AS_STRING(operand) = buffer;
			operand.type = VAL_STRING;
			break;
		}
		case EXPR_LITERAL_NUMBER:
		{
            expr_literal *literal_expr = (expr_literal*)expression;
			byte = OP_LOAD_CONSTANT;
            VAL_AS_DOUBLE(operand) = atof(literal_expr->literal);
			operand.type = VAL_DOUBLE;
			break;
		}
		case EXPR_LITERAL_BOOL:
		{
            expr_literal *literal_expr = (expr_literal*)expression;
			byte = OP_LOAD_CONSTANT;
            VAL_AS_INT(operand) = atoi(literal_expr->literal);
			operand.type = VAL_INT;
			break;
		}
		case EXPR_LITERAL_NULL:
		{
			byte = OP_LOAD_CONSTANT;
			operand.type = VAL_NULL;
			break;
		}
        case EXPR_UNARY:
        {
            expr_unary *unary_expr = (expr_unary*)expression;
            switch (unary_expr->operator->type) {
                case TOKEN_BANG:
                    break;
                case TOKEN_MINUS:
                    compile_expression(instructs, unary_expr->right);
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
            expr_var *var_expr = (expr_var*)expression;
            byte = OP_LOAD_NAME;
			VAL_AS_STRING(operand) = take_string(var_expr->name);
			operand.type = VAL_STRING;
            break;
        }
		case EXPR_CALL:
		{
            expr_call *call_expr = (expr_call*)expression;
			byte = OP_CALL_FUNCTION;
			compile_expression(instructs, call_expr->expression);
			
            int i = 0;
            for (i = 0; i < call_expr->count; i++)
                compile_expression(instructs, call_expr->arguments[i]);
			operand.type = VAL_INT;
			VAL_AS_INT(operand) = i;
			break;
		}
	}
	emit_instruction(instructs, byte, operand);
}

static void compile_expression_stmt(instruct *instructs, stmt *statement)
{
    stmt_expr *expr_stmt = (stmt_expr*)statement;
    compile_expression(instructs, expr_stmt->expression);
}

static void compile_block(instruct *instructs, stmt *statement, bool makeframe)
{
    stmt_block *block_stmt = (stmt_block*)statement;
    
    if (makeframe)
        emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL);

    int i = 0;
    stmt *current = NULL;
    while ((current = block_stmt->stmts[i++]))
        compile_statement(instructs, current);

    if (makeframe)
        emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL);
}

static void compile_if(instruct *instructs, stmt *statement)
{
    stmt_if *if_stmt = (stmt_if*)statement;
    int jmpfalse = 0;

    compile_expression(instructs, if_stmt->condition);
    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL);

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
    stmt_while *while_stmt = (stmt_while*)statement;
    int compare = 0;
    int jmpfalse = 0;
    int jmpbegin = 0;

    compare = instructs->count;

    compile_expression(instructs, while_stmt->condition);

    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL);
    compile_block(instructs, while_stmt->loopbody, false);
    
    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL);
    patch_jump(instructs, jmpbegin, compare);
    patch_jump(instructs, jmpfalse, instructs->count);
}

static void compile_for(instruct *instructs, stmt *statement)
{
    stmt_for *for_stmt = (stmt_for*)statement;
    int forbegin = 0;
    int jmpbegin = 0;
    int jmpfalse = 0;
    
    emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL);

    // Initializer_statement
    compile_statement(instructs, for_stmt->stmts[0]);
    // thenbranch used for compare statement
    forbegin = instructs->count;
    compile_statement(instructs, for_stmt->stmts[1]);
    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL);
    // loop body
    compile_block(instructs, for_stmt->loopbody, false);
    // elsebranch used for iterator statement
    compile_statement(instructs, for_stmt->stmts[2]);

    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL);

    // patch the jump instructs
    patch_jump(instructs, jmpbegin, forbegin);
    patch_jump(instructs, jmpfalse, instructs->count);
    
    emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL);
}

static void compile_print(instruct *instructs, stmt *statement)
{
    stmt_print *print_stmt = (stmt_print*)statement;
    compile_expression(instructs, print_stmt->value);
    emit_instruction(instructs, OP_PRINT, EMPTY_VAL);
}

static void compile_function(instruct *instructs, stmt *statement)
{
    stmt_function *function_stmt = (stmt_function*)statement;
	int argcount = function_stmt->num_parameters;
	token **parameters = function_stmt->parameters;

	objprim **arguments = ALLOCATE(objprim*, argcount);

	for (int i = 0; i < argcount; ++i) {
		objprim *prim = create_new_primitive(PRIM_STRING);
		PRIM_AS_STRING(prim) = take_string(parameters[i]);
		arguments[i] = prim;
	}
	
	objcode *codeobj = init_objcode(argcount, arguments);

    compile_block(&(codeobj->instructs), function_stmt->block, false);

    emit_instruction(&(codeobj->instructs), OP_RETURN, NULL_VAL);
	/* Push new code object onto the stack */
	value valobj = {.type = VAL_OBJECT, .val_obj = (object*)codeobj};
	emit_instruction(instructs, OP_MAKE_FUNCTION, valobj);

	/* Store object*/
    token *name = function_stmt->name;
    value operand = {.type = VAL_STRING, .val_string = take_string(name)};
    emit_instruction(instructs, OP_STORE_NAME, operand);
}

static void compile_return(instruct *instructs, stmt *statement)
{
    stmt_return *return_stmt = (stmt_return*)statement;
    compile_expression(instructs, return_stmt->value);
    emit_instruction(instructs, OP_RETURN, EMPTY_VAL);
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
        case STMT_PRINT:
        {
            compile_print(instructs, statement);
            break;
        }
        case STMT_FUNCTION:
        {
            compile_function(instructs, statement);
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

	emit_instruction(&instructs, OP_RETURN, EMPTY_VAL);

    return instructs;
}
