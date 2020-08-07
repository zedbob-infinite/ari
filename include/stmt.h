#ifndef ari_stmt_h
#define ari_stmt_h

#include "token.h"
#include "expr.h"

typedef enum stmttype_t
{
    STMT_BLOCK,
    STMT_EXPR,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_VAR,
    STMT_PRINT,
} stmttype;

typedef struct stmt_t
{
    stmttype type;
    token *name;
    struct stmt_t *initializer;
    expr *expression;
    expr *condition;
    expr *value;
    struct stmt_t *loopbody;
    struct stmt_t *thenbranch;
    struct stmt_t *elsebranch;
    int count;
    int capacity;
    struct stmt_t **stmts;
} stmt;

#endif
