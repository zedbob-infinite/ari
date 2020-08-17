#ifndef ari_expr_h
#define ari_expr_h

#include "token.h"

typedef enum exprtype_t
{
    EXPR_ASSIGN,
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_NUMBER,
    EXPR_LITERAL_BOOL,
    EXPR_LITERAL_NULL,
    EXPR_UNARY,
    EXPR_VARIABLE,
	EXPR_CALL,
} exprtype;

typedef struct expr_t
{
    exprtype type;
    token *name;
    token *operator;
    char *literal;
    int count;
	int capacity;
    struct expr_t **arguments;
    struct expr_t *expression;
    struct expr_t *value;
    struct expr_t *left;
    struct expr_t *right;
} expr;

#endif

