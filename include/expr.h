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
    EXPR_SET_PROP,
    EXPR_GET_PROP,
    EXPR_METHOD,
} exprtype;

typedef struct expr_t
{
    exprtype type;
} expr;

typedef struct expr_assign_t
{
    expr header;
    token *name;
    expr *value;
    expr *expression;
} expr_assign;

typedef struct expr_binary_t
{
    expr header;
    token *operator;
    expr *left;
    expr *right;
} expr_binary;

typedef struct expr_grouping_t
{
    expr header;
    expr *expression;
} expr_grouping;

typedef struct expr_literal_t
{
    expr header;
    char *literal;
} expr_literal;

typedef struct expr_unary_t
{
    expr header;
    token *operator;
    expr *right;
} expr_unary;

typedef struct expr_var_t
{
    expr header;
    token *name;
} expr_var;

typedef struct expr_call_t
{
    expr header;
    int count;
	int capacity;
    expr **arguments;
    expr *expression;
} expr_call;

typedef struct expr_set_t
{
    expr header;
    token *name;
    token *refobj;
    expr *value;
} expr_set;

typedef struct expr_get_t
{
    expr header;
    token *name;
    token *calling;
} expr_get;

#endif
