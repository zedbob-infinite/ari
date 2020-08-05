#ifndef ari_parser_h
#define ari_parser_h

#include <stdbool.h>

#include "stmt.h"
#include "token.h"
#include "tokenizer.h"

typedef struct parser_t
{
    // statements
    int num_statements;
    int capacity;
    stmt **statements;
    // tokens 
    int current;
    int num_tokens;
    token **tokens;
    // flags
    bool panicmode;
    bool haderror;
    // parser struct has the scanner
    scanner scan;
} parser;

bool parse(parser *analyzer, const char *source);
void init_parser(parser *analyzer);
void reset_parser(parser *analyzer);

#endif
