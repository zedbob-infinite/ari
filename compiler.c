#include <stdio.h>

#include "compiler.h"
#include "instruct.h"
#include "parser.h"
#include "tokenizer.h"

#define COPY_TOKEN_VALUE(val, token)       (strncpy(val, token->start, token->length))

/*static void compile_expression(instruct *instructs, expr *expression)
{
    int current;
    uint8_t byte;
    char *val = NULL;
    switch (expression->type) {
        case EXPR_UNARY:
            switch (expression->operator->type) {
                case TOKEN_BANG:
                    break;
                case TOKEN_MINUS:
                    compile_expression(instructs, expression->right);
                    byte = OP_NEGATE;
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


            val = create_new_value(VAL_STRING);
            ALLOCATE_STRING_VAL(val, expression->name->length);
            COPY_TOKEN_STRING(val, expression->name);
            break;
        }
        case EXPR_VARIABLE:
        {
            byte = OP_LOAD_NAME;
            val = create_new_value(VAL_STRING);
            ALLOCATE_STRING_VAL(val, expression->name->length);
            COPY_TOKEN_STRING(val, expression->name);
            break;
        }
        case EXPR_BINARY:
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
                    val = create_new_value(VAL_NUMBER);
                    NUMBER_VAL(val, expression->operator->type);
                    break;
                default:
                    break;
            }
            break;
        case EXPR_LITERAL_NUMBER:
        {
            byte = OP_LOAD_CONSTANT;
            val = create_new_value(VAL_NUMBER);
            NUMBER_VAL(val, atof(expression->
*/

/*static void compile_statement(instruct *instructs, stmt *statement)
{
    switch (statement->type) {
        case STMT_FOR:
            compile_for(instructs, statement);
            break;
        case STMT_WHILE:
            compile_while(instructs, statement);
            break;
        case STMT_BLOCK:
            compile_block(instructs, statement);
            break;
        case STMT_EXPRESSION:
            compile_expression(instructs, statement->expression);
            break;
        case STMT_IF:
            compile_if(instructs, statement);
            break;
        case STMT_VAR:
            compile_variable(instructs, statement);
            break;
    }
}*/

void compile(VM *vm, const char *source)
{
    // initialize parser
    init_parser(&vm->analyzer);
    parse(&vm->analyzer, source);

    // Compile parse trees into bytecode
    /*for (int i = 0; i < vm->analyzer->num_statements; i++)
        compile_statement(instructs, &vm->analyzer->statements[i]);*/

    reset_parser(&vm->analyzer);
    reset_scanner(&vm->analyzer.scan);
    /*return instructs;*/
}
