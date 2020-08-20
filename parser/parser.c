#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "memory.h"
#include "parser.h"
#include "stmt.h"
#include "token.h"
#include "tokenizer.h"

static stmt *statement(parser *analyzer);
static expr *expression(parser *analyzer);
static token *advance(parser *analyzer);
static stmt *block(parser *analyzer);
static char *token_type(int type);

static stmt_expr *init_stmt_expr(void)
{
    stmt_expr *new_stmt = ALLOCATE(stmt_expr, 1);
    new_stmt->header.type = STMT_EXPR;
    new_stmt->expression = NULL;
    return new_stmt;
}

static stmt_block *init_stmt_block(void)
{
    stmt_block *new_stmt = ALLOCATE(stmt_block, 1);
    new_stmt->header.type = STMT_BLOCK;
    new_stmt->count = 0;
    new_stmt->capacity = 0;
    new_stmt->stmts = NULL;
    return new_stmt;
}

static stmt_if *init_stmt_if(void)
{
    stmt_if *new_stmt = ALLOCATE(stmt_if, 1);
    new_stmt->header.type = STMT_IF;
    new_stmt->condition = NULL;
    new_stmt->thenbranch = NULL;
    new_stmt->elsebranch = NULL;
    return new_stmt;
}

static stmt_while *init_stmt_while(void)
{
    stmt_while *new_stmt = ALLOCATE(stmt_while, 1);
    new_stmt->header.type = STMT_WHILE;
    new_stmt->condition = NULL;
    new_stmt->loopbody = NULL;
    return new_stmt;
}

static stmt_for *init_stmt_for(void)
{
    stmt_for *new_stmt = ALLOCATE(stmt_for, 1);
    new_stmt->header.type = STMT_FOR;
    new_stmt->count = 0;
    new_stmt->capacity = 0;
    new_stmt->stmts = NULL;
    new_stmt->loopbody = NULL;
    return new_stmt;
}

static stmt_print *init_stmt_print(void)
{
    stmt_print *new_stmt = ALLOCATE(stmt_print, 1);
    new_stmt->header.type = STMT_PRINT;
    new_stmt->value = NULL;
    return new_stmt;
}

static stmt_function *init_stmt_function(void)
{
    stmt_function *new_stmt = ALLOCATE(stmt_function, 1);
    new_stmt->header.type = STMT_FUNCTION;
    new_stmt->name = NULL;
    new_stmt->num_parameters = 0;
    new_stmt->parameters = NULL;
    new_stmt->block = NULL;
    return new_stmt;
}

static stmt_return *init_stmt_return(void)
{
    stmt_return *new_stmt = ALLOCATE(stmt_return, 1);
    new_stmt->header.type = STMT_RETURN;
    new_stmt->value = NULL;
    return new_stmt;
}

static void *init_stmt(stmttype type)
{
    switch (type) {
        case STMT_EXPR:
            return init_stmt_expr();
        case STMT_BLOCK:
            return init_stmt_block();
        case STMT_IF:
            return init_stmt_if();
        case STMT_WHILE:
            return init_stmt_while();
        case STMT_FOR:
            return init_stmt_for();
        case STMT_PRINT:
            return init_stmt_print();
        case STMT_FUNCTION:
            return init_stmt_function();
        case STMT_RETURN:
            return init_stmt_return();
    }
    // Should be unreachable
    return NULL;
}

static expr_assign *init_expr_assign(void)
{
    expr_assign *new_expr = ALLOCATE(expr_assign, 1);
    new_expr->header.type = EXPR_ASSIGN;
    new_expr->name = NULL;
    new_expr->value = NULL;
    new_expr->expression = NULL;
    return new_expr;
}

static expr_binary *init_expr_binary(void)
{
    expr_binary *new_expr = ALLOCATE(expr_binary, 1);
    new_expr->header.type = EXPR_BINARY;
    new_expr->operator = NULL;
    new_expr->left = NULL;
    new_expr->right = NULL;
    return new_expr;
}

static expr_grouping *init_expr_grouping(void)
{
    expr_grouping *new_expr = ALLOCATE(expr_grouping, 1);
    new_expr->header.type = EXPR_GROUPING;
    new_expr->expression = NULL;
    return new_expr;
}

static expr_literal *init_expr_literal(exprtype type)
{
    expr_literal *new_expr = ALLOCATE(expr_literal, 1);
    new_expr->header.type = type;
    new_expr->literal = NULL;
    return new_expr;
}

static expr_unary *init_expr_unary(void)
{
    expr_unary *new_expr = ALLOCATE(expr_unary, 1);
    new_expr->header.type = EXPR_UNARY;
    new_expr->operator = NULL;
    new_expr->right = NULL;
    return new_expr;
}

static expr_var *init_expr_variable(void)
{
    expr_var *new_expr = ALLOCATE(expr_var, 1);
    new_expr->header.type = EXPR_VARIABLE;
    new_expr->name = NULL;
    return new_expr;
}

static expr_call *init_expr_call(void)
{
    expr_call *new_expr = ALLOCATE(expr_call, 1);
    new_expr->header.type = EXPR_CALL;
    new_expr->count = 0;
    new_expr->capacity = 0;
    new_expr->arguments = 0;
    new_expr->expression = 0;
    return new_expr;
}

static void *init_expr(exprtype type)
{
    switch (type) {
        case EXPR_ASSIGN:
            return init_expr_assign();
        case EXPR_BINARY:
            return init_expr_binary();
        case EXPR_GROUPING:
            return init_expr_grouping();
        case EXPR_LITERAL_STRING:
        case EXPR_LITERAL_NUMBER:
        case EXPR_LITERAL_BOOL:
        case EXPR_LITERAL_NULL:
            return init_expr_literal(type);
        case EXPR_UNARY:
            return init_expr_unary();
        case EXPR_VARIABLE:
            return init_expr_variable();
        case EXPR_CALL:
            return init_expr_call();
    }
    // Not reachable.
    return NULL;
}

static void delete_expression(expr *pexpr)
{
    if (pexpr) {
        switch (pexpr->type) {
            case EXPR_ASSIGN:
            {
                expr_assign *del = (expr_assign*)pexpr;
                delete_expression(del->value);
                delete_expression(del->expression);
                FREE(expr_assign, del);
                break;
            }
            case EXPR_BINARY:
            {
                expr_binary *del = (expr_binary*)pexpr;
                delete_expression(del->left);
                delete_expression(del->right);
                FREE(expr_binary, del);
                break;
            }
            case EXPR_GROUPING:
            {
                expr_grouping *del = (expr_grouping*)pexpr;
                delete_expression(del->expression);
                FREE(expr_grouping, del);
                break;
            }
            case EXPR_LITERAL_STRING:
            case EXPR_LITERAL_NUMBER:
            case EXPR_LITERAL_BOOL:
            case EXPR_LITERAL_NULL:
            {
                expr_literal *del = (expr_literal*)pexpr;
                FREE(char, del->literal);
                FREE(expr_literal, del);
                break;
            }
            case EXPR_UNARY:
            {
                expr_unary *del = (expr_unary*)pexpr;
                delete_expression(del->right);
                FREE(expr_unary, del);
                break;
            }
            case EXPR_VARIABLE:
            {
                expr_var *del = (expr_var*)pexpr;
                FREE(expr_var, del);
                break;
            }
            case EXPR_CALL:
            {
                expr_call *del = (expr_call*)pexpr;
                delete_expression(del->expression);
                for (size_t i = 0; i < del->count; i++)
                    delete_expression(del->arguments[i]);
                FREE(expr*, del->arguments);
                FREE(expr_call, del);
                break;
            }
        }
    }
}

static void delete_statements(stmt *pstmt)
{
    if (pstmt) {
        switch (pstmt->type) {
            case STMT_EXPR:
            {
                stmt_expr *del = (stmt_expr*)pstmt;
                delete_expression(del->expression);
                FREE(stmt_expr, del);
                break;
            }
            case STMT_BLOCK:
            {
                stmt_block *del = (stmt_block*)pstmt;
                for (int i = 0; i < del->count; i++)
                    delete_statements(del->stmts[i]);
                FREE(stmt*, del->stmts);
                FREE(stmt_block, del);
                break;
            }
            case STMT_IF:
            {
                stmt_if *del = (stmt_if*)pstmt;
                delete_expression(del->condition);
                delete_statements(del->thenbranch);
                delete_statements(del->elsebranch);
                FREE(stmt_if, del);
                break;
            }
            case STMT_WHILE:
            {
                stmt_while *del = (stmt_while*)pstmt;
                delete_expression(del->condition);
                delete_statements(del->loopbody);
                FREE(stmt_while, del);
                break;
            }
            case STMT_FOR:
            {
                stmt_for *del = (stmt_for*)pstmt;
                delete_statements(del->loopbody);
                for (int i = 0; i < 3; i++)
                    delete_statements(del->stmts[i]);
                FREE(stmt*, del->stmts);
                FREE(stmt_for, del);
                break;
            }
            case STMT_PRINT:
            {
                stmt_print *del = (stmt_print*)pstmt;
                delete_expression(del->value);
                FREE(stmt_print, del);
                break;
            }
            case STMT_FUNCTION:
            {
                stmt_function *del = (stmt_function*)pstmt;
                delete_statements(del->block);
                FREE(token*, del->parameters);
                FREE(stmt_function, del);
                break;
            }
            case STMT_RETURN:
            {
                stmt_return *del = (stmt_return*)pstmt;
                delete_expression(del->value);
                FREE(stmt_return, del);
                break;
            }
        }
    }
}

static char *take_string(token *tok)
{
    int length = tok->length;
    char *buffer = ALLOCATE(char, length + 1);
    buffer = strncpy(buffer, tok->start, tok->length);
    buffer[tok->length] = '\0';
    return buffer;
}

static void check_parser_capacity(parser* analyzer)
{
    int oldcapacity = analyzer->capacity;
    analyzer->capacity = GROW_CAPACITY(analyzer->capacity);
    analyzer->statements = GROW_ARRAY(analyzer->statements, 
                    stmt*, oldcapacity, analyzer->capacity);
    for (int i = oldcapacity; i < analyzer->capacity; ++i)
        analyzer->statements[i] = NULL;
}

static void synchronize(parser *analyzer)
{
    analyzer->panicmode = false;

    token **tokens =  analyzer->scan.tokens;
    while (tokens[analyzer->current]->type != TOKEN_EOF) {
        if (tokens[analyzer->current]->type == TOKEN_SEMICOLON) return;

        switch (tokens[analyzer->current]->type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                ;
        }
        advance(analyzer);
    }
}

static void error_at(parser *analyzer, token *tok, const char *msg)
{
    if (analyzer->panicmode) return;
    analyzer->panicmode = true;

    fprintf(stderr, "[line %d] Error", tok->line);
    fprintf(stderr, " for token %s\n", token_type(tok->type));
    if (tok->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (tok->type == TOKEN_ERROR)
        fprintf(stderr, ": %.*s", tok->length, tok->start);
    else
        fprintf(stderr, " at '%.*s'", tok->length, tok->start);
    fprintf(stderr, "\n");
    analyzer->haderror = true;
}

static void error(parser *analyzer, const char *msg)
{
    error_at(analyzer, analyzer->scan.tokens[analyzer->current - 1], msg);
}

static void error_at_current(parser *analyzer, const char *msg)
{
    error_at(analyzer, analyzer->scan.tokens[analyzer->current], msg);
}

static token *peek(parser *analyzer)
{
    return analyzer->scan.tokens[analyzer->current];
}

static bool is_at_end(parser *analyzer)
{
    return peek(analyzer)->type == TOKEN_EOF;
}

static bool check(parser *analyzer, tokentype type)
{
    if (is_at_end(analyzer))
        return false;
    return peek(analyzer)->type == type;
}

static token *previous(parser *analyzer)
{
    return analyzer->scan.tokens[analyzer->current - 1];
}

static token *advance(parser *analyzer)
{
    if (!is_at_end(analyzer))
        analyzer->current++;
    return previous(analyzer);
}

static token *consume(parser *analyzer, tokentype type, char *msg)
{
    if (check(analyzer, type))
        return advance(analyzer);

    error_at_current(analyzer, msg);
    return NULL;
}

static bool match(parser *analyzer, tokentype type)
{
    if (check(analyzer, type)) {
        advance(analyzer);
        return true;
    }
    return false;
}

static stmt *get_expression_statement(expr *new_expr)
{
    stmt_expr *new_stmt = init_stmt(STMT_EXPR);
    new_stmt->expression = new_expr;
    return (stmt*)new_stmt;
}

static stmt *get_if_statement(expr *condition, stmt *thenbranch, 
        stmt *elsebranch)
{
    stmt_if *new_stmt = init_stmt(STMT_IF);
    new_stmt->condition = condition;
    new_stmt->thenbranch = thenbranch;
    new_stmt->elsebranch = elsebranch;
    return (stmt*)new_stmt;
}

static stmt *get_while_statement(expr *condition, stmt *loopbody)
{
    stmt_while *new_stmt = init_stmt(STMT_WHILE);
    new_stmt->condition = condition;
    new_stmt->loopbody = loopbody;
    return (stmt*)new_stmt;
}

static stmt *get_for_statement(stmt *initializer, stmt *condition,
        stmt *iterator, stmt *loopbody)
{
    stmt_for *new_stmt = init_stmt(STMT_FOR);
    new_stmt->stmts = ALLOCATE(stmt*, 3);
    new_stmt->capacity = 3;
    new_stmt->count = 3;
    new_stmt->stmts[0] = initializer;
    new_stmt->stmts[1] = condition;
    new_stmt->stmts[2] = iterator;
    new_stmt->loopbody = loopbody;
    return (stmt*)new_stmt;
}
static stmt *get_print_statement(expr *value)
{
    stmt_print *new_stmt = init_stmt(STMT_PRINT);
    new_stmt->value = value;
    return (stmt*)new_stmt;
}

static stmt *get_function_statement(token *name, int num_parameters, token **parameters, 
        stmt *body)
{
    stmt_function *new_stmt = init_stmt(STMT_FUNCTION);
    new_stmt->name = name;
	new_stmt->num_parameters = num_parameters;
    new_stmt->parameters = parameters;
    new_stmt->block = body;
    return (stmt*)new_stmt;
}

static stmt *get_return_statement(expr *value)
{
    stmt_return *new_stmt = init_stmt(STMT_RETURN);
    new_stmt->value = value;
    return (stmt*)new_stmt;
}

static expr *get_assign_expr(expr *assign_expr, expr *value)
{
    expr_var *var_expr = (expr_var*)assign_expr;
    expr_assign *new_expr = init_expr(EXPR_ASSIGN);
    new_expr->name = var_expr->name;
    new_expr->value = value;
    new_expr->expression = assign_expr;
    return (expr*)new_expr;
}

static expr *get_binary_expr(token *operator, expr *left, expr *right)
{
    expr_binary *new_expr = init_expr(EXPR_BINARY);
    new_expr->left = left;
    new_expr->right = right;
    new_expr->operator = operator;
    return (expr*)new_expr;
}

static expr *get_grouping_expr(expr *group_expr)
{
    expr_grouping *new_expr = init_expr(EXPR_GROUPING);
    new_expr->expression = group_expr;
    return (expr*)new_expr;
}

static expr *get_literal_expr(char *value, exprtype type)
{
    expr_literal *new_expr = init_expr(type);
    new_expr->literal = value;
    return (expr*)new_expr;
}

static expr *get_unary_expr(token *opcode, expr *right)
{
    expr_unary *new_expr = init_expr(EXPR_UNARY);
    new_expr->operator = opcode;
    new_expr->right = right;
    return (expr*)new_expr;
}

static expr *get_variable_expr(token *name)
{
    expr_var *new_expr = init_expr(EXPR_VARIABLE);
    new_expr->name = name;
    return (expr*)new_expr;
}

static expr *get_call_expression(expr *callee, expr **arguments, int num_arguments, int capacity)
{
	expr_call *new_expr = init_expr(EXPR_CALL);
	new_expr->expression = callee;
	new_expr->arguments = arguments;
    new_expr->count = num_arguments;
    new_expr->capacity = capacity;
	return (expr*)new_expr;
}

static expr *primary(parser *analyzer)
{
    if (match(analyzer, TOKEN_FALSE)) {
        char *buffer = ALLOCATE(char, 2);
        char *number = "0";
        strncpy(buffer, number, 2);
        return get_literal_expr(buffer, EXPR_LITERAL_BOOL);
    }
    if (match(analyzer, TOKEN_TRUE)) {
        char *buffer = ALLOCATE(char, 2);
        char *number = "1";
        strncpy(buffer, number, 2);
        return get_literal_expr(buffer, EXPR_LITERAL_BOOL);
    }
    if (match(analyzer, TOKEN_NULL)) {
        char *buffer = ALLOCATE(char, 5);
        char *null = "NULL";
        strncpy(buffer, null, 4);
        return get_literal_expr(buffer, EXPR_LITERAL_NULL);
    }
    if (match(analyzer, TOKEN_NUMBER))
        return get_literal_expr(take_string(previous(analyzer)),
                EXPR_LITERAL_NUMBER);
    if (match(analyzer, TOKEN_STRING)) 
        return get_literal_expr(take_string(previous(analyzer)),
                    EXPR_LITERAL_STRING);
    if (match(analyzer, TOKEN_IDENTIFIER))
        return get_variable_expr(previous(analyzer));
    if (match(analyzer, TOKEN_LEFT_PAREN)) {
        expr *new_expr = expression(analyzer);
        consume(analyzer, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return get_grouping_expr(new_expr);
    }

    error(analyzer, "Expect expression.");
    return NULL;
}

static expr *finish_call(parser *analyzer, expr *callee)
{
    /* Initialize arguments array */
	int i = 0;
	int oldcapacity = 0;
    int capacity = 0;
    capacity = GROW_CAPACITY(capacity);
    expr **arguments = ALLOCATE(expr*, capacity);
    for (int j = 0; j < capacity; j++)
        arguments[j] = NULL;

    /* Build the arguments array */
    if (!check(analyzer, TOKEN_RIGHT_PAREN)) {
        do {
            if (i > capacity - 1) {
                oldcapacity = capacity;
                capacity = GROW_CAPACITY(capacity);
                arguments = GROW_ARRAY(arguments, expr*, oldcapacity, capacity);
                for (int j = oldcapacity; j < capacity; j++)
                    arguments[j] = NULL;
            }
            arguments[i++] = expression(analyzer);
        } while (match(analyzer, TOKEN_COMMA));
    }
    consume(analyzer, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return get_call_expression(callee, arguments, i, capacity);
}

static expr *call(parser *analyzer)
{
	expr *new_expr = primary(analyzer);
	if (match(analyzer, TOKEN_LEFT_PAREN))
		return finish_call(analyzer, new_expr);
	return new_expr;
}

static expr *unary(parser *analyzer)
{
    if (match(analyzer, TOKEN_BANG) || match(analyzer, TOKEN_MINUS)) {
        token *opcode = previous(analyzer);
        expr *right = unary(analyzer);
        return get_unary_expr(opcode, right);
    }
    return call(analyzer);
}

static expr *multiplication(parser *analyzer)
{
    expr *new_expr = unary(analyzer);

    while (match(analyzer, TOKEN_SLASH) || match(analyzer, TOKEN_STAR)) {
        token *opcode = previous(analyzer);
        expr *right = unary(analyzer);
        new_expr = get_binary_expr(opcode, new_expr,
                right);
    }
    return new_expr;
}

static expr *addition(parser *analyzer)
{
    expr *new_expr = multiplication(analyzer);

    while (match(analyzer, TOKEN_MINUS) || match(analyzer, TOKEN_PLUS)) {
        token *opcode = previous(analyzer);
        expr *right = multiplication(analyzer);
        new_expr = get_binary_expr(opcode, new_expr,
                right);
    }
    return new_expr;
}

static expr *comparison(parser *analyzer)
{
    expr *new_expr = addition(analyzer);
    
    while (match(analyzer, TOKEN_GREATER) ||
            match(analyzer, TOKEN_GREATER_EQUAL) ||
            match(analyzer, TOKEN_LESS) ||
            match(analyzer, TOKEN_LESS_EQUAL)) {
        token *opcode = previous(analyzer);
        expr *right = addition(analyzer);
        new_expr = get_binary_expr(opcode, new_expr,
                right);
    }
    return new_expr;
}

static expr *equality(parser *analyzer)
{
    expr *new_expr = comparison(analyzer);
    while (match(analyzer, TOKEN_BANG_EQUAL)
            || match(analyzer, TOKEN_EQUAL_EQUAL)) {
        token *opcode = previous(analyzer);
        expr *right = comparison(analyzer);
        new_expr = get_binary_expr(opcode, new_expr,
                right);
    }
    return new_expr;
}

static expr* assignment(parser *analyzer)
{
    expr *new_expr = equality(analyzer);
    if (match(analyzer, TOKEN_EQUAL)) {
        if (new_expr->type == EXPR_VARIABLE)
            return get_assign_expr(new_expr, assignment(analyzer));
        error(analyzer, "Invalid assignment target.");
    }
    return new_expr;
}

static expr *expression(parser *analyzer)
{
    return assignment(analyzer);
}

static stmt *function(parser *analyzer)
{
    token *name = consume(analyzer, TOKEN_IDENTIFIER, "Expect function name.");
    consume(analyzer, TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    int i = 0;
    int oldsize = 0;
    int size = 8;
    token **parameters = ALLOCATE(token*, size);
    if (!check(analyzer, TOKEN_RIGHT_PAREN)) {
        do {
            if (i > size) {
                oldsize = size;
                size *= 2;
                parameters = GROW_ARRAY(parameters, token*, oldsize, size);
                for (int j = oldsize; j < size; j++)
                    parameters[j] = NULL;
            }
            parameters[i++] = consume(analyzer, TOKEN_IDENTIFIER, 
                    "Expect parameter name.");
        } while (match(analyzer, TOKEN_COMMA));
    }

    consume(analyzer, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(analyzer, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    stmt *body = block(analyzer);
    return get_function_statement(name, i, parameters, body);
}

static stmt *declaration(parser *analyzer)
{
    if (match(analyzer, TOKEN_FUN))
        return function(analyzer);
    if (analyzer->panicmode) synchronize(analyzer);
    return statement(analyzer);
}

static inline void check_stmt_capacity(stmt_block *block_stmt)
{
    int oldcapacity = block_stmt->capacity;
    if (block_stmt->capacity < block_stmt->count + 1) {
        block_stmt->capacity = GROW_CAPACITY(block_stmt->capacity);
        block_stmt->stmts = realloc(block_stmt->stmts, sizeof(stmt*) * block_stmt->capacity);
    }
    for (int i = oldcapacity; i < block_stmt->capacity; ++i)
        block_stmt->stmts[i] = NULL;
}

static stmt *block(parser *analyzer)
{
    stmt_block *new_stmt = init_stmt(STMT_BLOCK);

    while (!check(analyzer, TOKEN_RIGHT_BRACE) && !is_at_end(analyzer)) {
        check_stmt_capacity(new_stmt);
        new_stmt->stmts[new_stmt->count++] = declaration(analyzer);
    }

    consume(analyzer, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return (stmt*)new_stmt;
}

static stmt *if_statement(parser *analyzer)
{
    consume(analyzer, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expr *condition = expression(analyzer);
    consume(analyzer, TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

    stmt *thenbranch = statement(analyzer);
    stmt *elsebranch = NULL;
    if (match(analyzer, TOKEN_ELSE))
        elsebranch = statement(analyzer);

    return get_if_statement(condition, thenbranch, elsebranch);
}

static inline stmt *iterator_statement(parser *analyzer)
{
    expr *new_expr = expression(analyzer);
    return get_expression_statement(new_expr);
}

static stmt *for_statement(parser *analyzer)
{
    consume(analyzer, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    stmt *initializer = statement(analyzer);
    stmt *condition = statement(analyzer);
    stmt *iterator = iterator_statement(analyzer);
    consume(analyzer, TOKEN_RIGHT_PAREN, "Expect ')' after for condition.");
    stmt *loop_body = statement(analyzer);
    return get_for_statement(initializer, condition, iterator, loop_body);
}

static stmt *while_statement(parser *analyzer)
{
    consume(analyzer, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expr *condition = expression(analyzer);
    consume(analyzer, TOKEN_RIGHT_PAREN,
            "Expect ')' after while condition.");
    stmt *loop_body = statement(analyzer);
    return get_while_statement(condition, loop_body);
}

static stmt *expression_statement(parser *analyzer)
{
    expr *new_expr = expression(analyzer);
    consume(analyzer, TOKEN_SEMICOLON, "Expect ';' after expression.");
    return get_expression_statement(new_expr);
}

static stmt *print_statement(parser *analyzer)
{
    expr *value = expression(analyzer);
    consume(analyzer, TOKEN_SEMICOLON, "Expect ';' after expressin.");
    return get_print_statement(value);
}

static stmt *return_statement(parser *analyzer)
{
    expr *value = NULL;
    if (!check(analyzer, TOKEN_SEMICOLON))
        value = expression(analyzer);

    consume(analyzer, TOKEN_SEMICOLON, "Expect ';' after return value.");
    return get_return_statement(value);
}

static stmt *statement(parser *analyzer)
{
    if (match(analyzer, TOKEN_FOR))
        return for_statement(analyzer);
    if (match(analyzer, TOKEN_RETURN))
        return return_statement(analyzer);
    if (match(analyzer, TOKEN_WHILE))
        return while_statement(analyzer);
    if (match(analyzer, TOKEN_PRINT))
        return print_statement(analyzer);
    if (match(analyzer, TOKEN_IF))
        return if_statement(analyzer);
    if (match(analyzer, TOKEN_LEFT_BRACE))
        return block(analyzer);
    return expression_statement(analyzer);
}

void reset_parser(parser *analyzer)
{
    if (analyzer->statements) {
        stmt **statements = analyzer->statements;
        for (int i = 0; i < analyzer->num_statements; ++i) {
            delete_statements(statements[i]);
        }
        FREE(stmt*, analyzer->statements);
    }
    init_parser(analyzer);
    reset_scanner(&analyzer->scan);
}

void init_parser(parser *analyzer)
{
    analyzer->num_statements = 0;
    analyzer->capacity = 0;
    analyzer->statements = NULL;
    analyzer->current = 0;
    analyzer->num_tokens = 0;
    analyzer->tokens = NULL;
    analyzer->panicmode = false;
    analyzer->haderror = false;
}

bool parse(parser *analyzer, const char *source)
{
    // Get the tokens
    init_scanner(&analyzer->scan);
    scan_tokens(&analyzer->scan, source);

    check_parser_capacity(analyzer);
    // Parse tokens into AST
    int i = 0;
    while (!is_at_end(analyzer)) {
        if (analyzer->capacity < analyzer->num_statements + 1)
            check_parser_capacity(analyzer);

        analyzer->statements[i++] = declaration(analyzer);
        analyzer->num_statements++;
        if (analyzer->panicmode) synchronize(analyzer);
    }
    is_at_end(analyzer);
    return analyzer->haderror;
}

static char *token_type(int type)
{
    char *msg = NULL;
    switch (type) {
        case TOKEN_LEFT_PAREN: msg = "LEFT_PAREN"; break;
        case TOKEN_RIGHT_PAREN: msg = "RIGHT_PAREN"; break;
        case TOKEN_LEFT_BRACE: msg = "LEFT_BRACE"; break;
        case TOKEN_RIGHT_BRACE: msg = "RIGHT_BRACE"; break;
        case TOKEN_LEFT_BRACKET: msg = "LEFT_BRACE"; break;
        case TOKEN_RIGHT_BRACKET: msg = "RIGHT_BRACE"; break;
        case TOKEN_COMMA: msg = "COMMA"; break;
        case TOKEN_DOT: msg = "DOT"; break;
        case TOKEN_MINUS: msg = "MINUS"; break;
        case TOKEN_PLUS: msg = "PLUS"; break;
        case TOKEN_SEMICOLON: msg = "SEMICOLON"; break;
        case TOKEN_SLASH: msg = "SLASH"; break;
        case TOKEN_STAR: msg = "STAR"; break;
        case TOKEN_BANG: msg = "BANG"; break;
        case TOKEN_BANG_EQUAL: msg = "BANG_EQUAL"; break;
        case TOKEN_EQUAL: msg = "EQUAL"; break;
        case TOKEN_EQUAL_EQUAL: msg = "EQUAL_EQUAL"; break;
        case TOKEN_GREATER: msg = "GREATER"; break;
        case TOKEN_GREATER_EQUAL: msg = "GREATER_EQUAL"; break;
        case TOKEN_LESS: msg = "LESS"; break;
        case TOKEN_LESS_EQUAL: msg = "LESS_EQUAL"; break;
        case TOKEN_IDENTIFIER: msg = "IDENTIFIER"; break;
        case TOKEN_STRING: msg = "STRING"; break;
        case TOKEN_NUMBER: msg = "NUMBER"; break;
        case TOKEN_AND: msg = "AND"; break;
        case TOKEN_CLASS: msg = "CLASS"; break;
        case TOKEN_ELSE: msg = "ELSE"; break;
        case TOKEN_FALSE: msg = "FALSE"; break;
        case TOKEN_FUN: msg = "FUN"; break;
        case TOKEN_FOR: msg = "FOR"; break;
        case TOKEN_IF: msg = "IF"; break;
        case TOKEN_NULL: msg = "NULL"; break;
        case TOKEN_OR: msg = "OR"; break;
        case TOKEN_RETURN: msg = "RETURN"; break;
        case TOKEN_SUPER: msg = "SUPER"; break;
        case TOKEN_THIS: msg = "THIS"; break;
        case TOKEN_TRUE: msg = "TRUE"; break;
        case TOKEN_VAR: msg = "VAR"; break;
        case TOKEN_WHILE: msg = "WHILE"; break;
        case TOKEN_PRINT: msg = "PRINT"; break;
        case TOKEN_EXIT: msg = "EXIT"; break;
        case TOKEN_ERROR: msg = "ERROR"; break;
        case TOKEN_EOF: msg = "EOF"; break;
        default:
                msg="ERROR";
                break;
    }
    return msg;
}