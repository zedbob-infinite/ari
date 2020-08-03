#ifndef ari_parser_h
#define ari_parser_h

#include <stdbool.h>

#include "stmt.h"
#include "token.h"

typedef struct parser_t
{
    int num_statements;
    int capacity;
    stmt *statements;
    int current;
    token **tokens;
    int num_of_tokens;
    bool panicmode;
    bool haderror;
} parser;

void parse(parser *analyzer);
void init_parser(parser *analyzer);
void reset_parser(parser *analyzer);

#endif
