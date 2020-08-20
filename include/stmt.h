#ifndef ari_stmt_h
#define ari_stmt_h

#include "token.h"
#include "expr.h"

typedef enum stmttype_t
{
    STMT_EXPR,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_PRINT,
    STMT_FUNCTION,
    STMT_RETURN,
} stmttype;

typedef struct stmt_t
{
    stmttype type;
} stmt;

typedef struct stmt_expr_t
{
    stmt header;
    expr *expression;
} stmt_expr;

typedef struct stmt_block_t
{
    stmt header;
    int count;
    int capacity;
    stmt **stmts;
} stmt_block;

typedef struct stmt_if_t
{
    stmt header;
    expr *condition;
    stmt *thenbranch;
    stmt *elsebranch;
} stmt_if;

typedef struct stmt_while_t
{
    stmt header;
    expr *condition;
    stmt *loopbody;
} stmt_while;

typedef struct stmt_for_t
{
    stmt header;
    int count;
    int capacity;
    stmt **stmts;
    stmt *loopbody;
} stmt_for;

typedef struct stmt_print_t
{
    stmt header;
    expr *value;
} stmt_print;

typedef struct stmt_function_t
{
    stmt header;
    token *name;
	int num_parameters;
    token **parameters;
    struct stmt_t *block;
} stmt_function;

typedef struct stmt_return_t
{
    stmt header;
    expr *value;
} stmt_return;

/*typedef struct stmt_t
{
    stmttype type;
    token *name;
	int num_parameters;
    token **parameters;
    struct stmt_t *initializer;
    expr *expression;
    expr *condition;
    expr *value;
    struct stmt_t *loopbody;
    struct stmt_t *thenbranch;
    struct stmt_t *elsebranch;
    struct stmt_t *block;
    int count;
    int capacity;
    struct stmt_t **stmts;
} stmt;*/

#endif
