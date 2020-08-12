#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "opcode.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

static void compile_statement(instruct *instructs, stmt *statement);
static void compile_block(instruct *instructs, stmt *statement, 
        bool makeframe);

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

static code8 *create_code(uint8_t bytecode, value operand, valtype type)
{
	code8 *code = ALLOCATE(code8, 1);
	code->bytecode = bytecode;
	code->operand = operand;
    code->type = type;
	return code;
}	

static int emit_instruction(instruct *instructs, uint8_t bytecode, value operand, valtype type)
{
	int current = instructs->count;
	if (instructs->capacity < instructs->count + 1)
		check_instruct_capacity(instructs);

	instructs->code[instructs->count++] = create_code(bytecode, operand, type);
	return current;
}

static void compile_expression(instruct *instructs, expr *expression)
{
    uint8_t byte = 0;
    value operand;
    valtype type = VAL_EMPTY;
    VAL_AS_INT(operand) = 0;
    switch (expression->type) {
        case EXPR_UNARY:
            switch (expression->operator->type) {
                case TOKEN_BANG:
                    break;
                case TOKEN_MINUS:
                    compile_expression(instructs, expression->right);
                    byte = OP_NEGATE;
                    break;
				default:
					// future error code here
					break;
			}
            break;
        case EXPR_GROUPING:
            compile_expression(instructs, expression->expression);
            return;
        case EXPR_ASSIGN: 
        {
            byte = OP_STORE_NAME;
            compile_expression(instructs, expression->value);
			VAL_AS_STRING(operand) = take_string(expression->name);
            type = VAL_STRING;
            break;
        }
        case EXPR_VARIABLE:
        {
            byte = OP_LOAD_NAME;
			VAL_AS_STRING(operand) = take_string(expression->name);
            type = VAL_STRING;
            break;
        }
        case EXPR_BINARY:
		{
            compile_expression(instructs, expression->left);
            compile_expression(instructs, expression->right);
            switch (expression->operator->type) {
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
                    VAL_AS_INT(operand) = expression->operator->type;
                    type = VAL_INT;
                    break;
                default:
                    break;
			}
            break;
		}
		case EXPR_LITERAL_NUMBER:
		{
			byte = OP_LOAD_CONSTANT;
            VAL_AS_DOUBLE(operand) = atof(expression->literal);
            type = VAL_DOUBLE;
			break;
		}
		case EXPR_LITERAL_STRING:
		{
			int length = strlen(expression->literal);
            byte = OP_LOAD_CONSTANT;

            ALLOCATE_VAL_STRING(operand, expression->literal);
            COPY_VALUE_STRING(operand, expression->literal, length);
            type = VAL_STRING;
			break;
		}
		case EXPR_LITERAL_BOOL:
		{
			byte = OP_LOAD_CONSTANT;
            VAL_AS_INT(operand) = atoi(expression->literal);
            type = VAL_INT;
			break;
		}
		default:
			// future error code here
			break;
	}
	emit_instruction(instructs, byte, operand, type);
}

static void compile_for(instruct *instructs, stmt *statement)
{
    int forbegin = 0;
    int jmpbegin = 0;
    int jmpfalse = 0;
    
    emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL, VAL_INT);

    // Initializer_statement
    compile_statement(instructs, statement->stmts[0]);
    // thenbranch used for compare statement
    forbegin = instructs->count;
    compile_statement(instructs, statement->stmts[1]);
    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL, VAL_INT);
    // loop body
    compile_block(instructs, statement->loopbody, false);
    // elsebranch used for iterator statement
    compile_statement(instructs, statement->stmts[2]);

    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL, VAL_INT);

    // patch the jump instructs
    patch_jump(instructs, jmpbegin, forbegin);
    patch_jump(instructs, jmpfalse, instructs->count);
    
    emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL, VAL_INT);
}

static void compile_while(instruct *instructs, stmt *statement)
{
    int compare = 0;
    int jmpfalse = 0;
    int jmpbegin = 0;

    compare = instructs->count;

    compile_expression(instructs, statement->condition);

    jmpfalse = emit_instruction(instructs, OP_JMP_FALSE, EMPTY_VAL, VAL_INT);
    compile_block(instructs, statement->loopbody, false);
    
    jmpbegin = emit_instruction(instructs, OP_JMP_LOC, EMPTY_VAL, VAL_INT);
    patch_jump(instructs, jmpbegin, compare);
    
    patch_jump(instructs, jmpfalse, instructs->count);
}

static void compile_variable(instruct *instructs, stmt *statement)
{
    token *name = statement->name;
    if (statement->initializer)
        compile_statement(instructs, statement->initializer);

    value operand;
    VAL_AS_STRING(operand) = take_string(name);

    emit_instruction(instructs, OP_STORE_NAME, operand, VAL_STRING);
}

static void compile_block(instruct *instructs, stmt *statement, bool makeframe)
{
    int i = 0;
    stmt *current = NULL;
    if (makeframe)
        emit_instruction(instructs, OP_PUSH_FRAME, EMPTY_VAL, VAL_INT);

    while ((current = statement->stmts[i++]))
        compile_statement(instructs, current);

    if (makeframe)
        emit_instruction(instructs, OP_POP_FRAME, EMPTY_VAL, VAL_INT);
}

static void compile_function(instruct *instructs, stmt *statement)
{
    /*stmt *new_stmt = init_stmt();
    new_stmt->type = STMT_FUNCTION;
    new_stmt->name = name;
    new_stmt->parameters = parameters;
    new_stmt->block = body;*/
    token *name = statement->name;

    value operand;
    VAL_AS_STRING(operand) = take_string(name);

    emit_instruction(instructs, OP_STORE_NAME, operand, VAL_STRING);
}

static void compile_statement(instruct *instructs, stmt *statement)
{
    switch (statement->type) {
        case STMT_FUNCTION:
            compile_function(instructs, statement);
            break;
        case STMT_FOR:
            compile_for(instructs, statement);
            break;
        case STMT_WHILE:
            compile_while(instructs, statement);
            break;
        case STMT_BLOCK:
            compile_block(instructs, statement, true);
            break;
        case STMT_EXPR:
            compile_expression(instructs, statement->expression);
            //emit_instruction(instructs, OP_POP, NULL);
            //emit_instruction(instructs, OP_RETURN, NULL);
            break;
        case STMT_PRINT:
        {
            compile_expression(instructs, statement->value);
            emit_instruction(instructs, OP_PRINT, EMPTY_VAL, -1);
            break;
        }
        case STMT_IF:
            //compile_if(instructs, statement);
            break;
        case STMT_VAR:
            compile_variable(instructs, statement);
            break;
        default:
            break;
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

    // Compile parse trees into bytecode
	for (int i = 0; i < analyzer->num_statements; i++) {
        if (instructs.capacity < instructs.count + 1)
            check_instruct_capacity(&instructs);
        compile_statement(&instructs, analyzer->statements[i]);
    }

    value empty;
    VAL_AS_INT(empty) = 0;
	emit_instruction(&instructs, OP_RETURN, empty, -1);
    reset_parser(analyzer);

    return instructs;
}
