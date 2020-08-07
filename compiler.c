#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "object.h"
#include "opcode.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

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

static code8 *create_code(uint8_t bytecode, object *operand)
{
	code8 *code = ALLOCATE(code8, 1);
	code->bytecode = bytecode;
	code->operand = operand;
	return code;
}	

static int emit_instruction(instruct *instructs, uint8_t bytecode, object *operand)
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
    object *operand = NULL;
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

			objprim *obj = create_new_primitive(VAL_STRING);
            
			PRIM_AS_STRING(obj) = take_string(expression->name);
			operand = (object*)obj;
            break;
        }
        case EXPR_VARIABLE:
        {
            byte = OP_LOAD_NAME;
            objprim *obj = create_new_primitive(VAL_STRING);

			PRIM_AS_STRING(obj) = take_string(expression->name);
			operand = (object*)obj;
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
                    objprim *obj = create_new_primitive(VAL_INT);
                    INT_VAL(obj, expression->operator->type);
					operand = (object*)obj;
                    break;
                default:
                    break;
			}
            break;
		}
		case EXPR_LITERAL_NUMBER:
		{
			byte = OP_LOAD_CONSTANT;
			objprim *obj = create_new_primitive(VAL_DOUBLE);
			DOUBLE_VAL(obj, atof(expression->literal));
			operand = (object*)obj;
			break;
		}
		case EXPR_LITERAL_STRING:
		{
			int length = sizeof(expression->literal);
			byte = OP_LOAD_CONSTANT;
			objprim *obj = create_new_primitive(VAL_STRING);
			ALLOCATE_PRIM_STRING(obj, length);
            COPY_PRIM_STRING(obj, expression->literal, length);
			operand = (object*)obj;
			break;
		}
		case EXPR_LITERAL_BOOL:
		{
			byte = OP_LOAD_CONSTANT;
			objprim *obj = create_new_primitive(VAL_BOOL);
			BOOL_VAL(obj, atoi(expression->literal));
			operand = (object*)obj;
			break;
		}
		default:
			// future error code here
			break;
	}
	emit_instruction(instructs, byte, operand);
}

static void compile_statement(instruct *instructs, stmt *statement)
{
    switch (statement->type) {
        case STMT_FOR:
            //compile_for(instructs, statement);
            break;
        case STMT_WHILE:
            //compile_while(instructs, statement);
            break;
        case STMT_BLOCK:
            //compile_block(instructs, statement);
            break;
        case STMT_EXPR:
            compile_expression(instructs, statement->expression);
            //emit_instruction(instructs, OP_POP, NULL);
            emit_instruction(instructs, OP_RETURN, NULL);
            break;
        case STMT_IF:
            //compile_if(instructs, statement);
            break;
        case STMT_VAR:
            //compile_variable(instructs, statement);
            break;
    }
}

instruct *compile(parser *analyzer, const char *source)
{
    if (parse(analyzer, source))
        return NULL;

    // Compile parse trees into bytecode
    instruct *instructs = ALLOCATE(instruct, 1);
    init_instruct(instructs);
	for (int i = 0; i < analyzer->num_statements; i++) {
        if (instructs->capacity < instructs->count + 1)
            check_instruct_capacity(instructs);
        compile_statement(instructs, analyzer->statements[i]);
    }

	//emit_instruction(instructs, OP_RETURN, NULL);
    reset_parser(analyzer);
    reset_scanner(&analyzer->scan);

    return instructs;
}
