#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "memory.h"
#include "tokenizer.h"
#include "token.h"

void reset_scanner(scanner *scan)
{
    if (scan->tokens) {
        for (int i = 0; i < scan->capacity; ++i) {
            free(scan->tokens[i]);
        }
        free(scan->tokens);
    }
    init_scanner(scan);
}

void init_scanner(scanner *scan)
{
    scan->start = NULL;
    scan->current = NULL;
    scan->line = 1;
    scan->length = 0;
    scan->source = NULL;
    scan->num_tokens = 0;
    scan->capacity = 0;
    scan->tokens = NULL;
}

static void check_scanner_capacity(scanner *scan)
{
    int oldcapacity = scan->capacity;
    scan->capacity = GROW_CAPACITY(scan->capacity);
    scan->tokens = GROW_ARRAY(scan->tokens, token*, oldcapacity, scan->capacity);

    for (int i = oldcapacity; i < scan->capacity; ++i)
        scan->tokens[i] = NULL;
}

static inline bool is_at_end(scanner *scan)
{
    return *scan->current == '\0';
}

static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static inline char peek(scanner *scan)
{
    return *scan->current;
}

static inline char peek_next(scanner *scan)
{
    if (is_at_end(scan)) return '\0';
    return scan->current[1];
}

static inline char advance(scanner *scan)
{
    scan->current++;
    return scan->current[-1];
}

static void add_token(scanner *scan, tokentype type)
{
    if (scan->capacity < scan->num_tokens + 1)
        check_scanner_capacity(scan);
    token *tok = ALLOCATE(token, 1);

    tok->type = type;
    tok->start = scan->start;
    tok->length = (int)(scan->current - scan->start);
    tok->line = scan->line;
    scan->tokens[scan->num_tokens++] = tok;
}

static inline tokentype check_keyword(scanner* scan, int start, int length,
        const char *rest, tokentype type)
{
    if (scan->current - scan->start == start + length &&
            memcmp(scan->start + start, rest, length) == 0)
        return type;
    return TOKEN_IDENTIFIER;
}

static inline void identifier(scanner *scan)
{ 
    tokentype type = 0;

    while (is_alpha(peek(scan)) || is_digit(peek(scan))) advance(scan);

    switch (scan->start[0]) {
        case 'a': type = check_keyword(scan, 1, 2, "nd", TOKEN_AND); break;
        case 'c': type = check_keyword(scan, 1, 4, "lass", TOKEN_CLASS); break;
        case 'e': 
                  if (scan->current - scan->start > 1)
                      switch(scan->start[1]) {
                          case 'l':
                              type = check_keyword(scan, 2, 2, "se", TOKEN_ELSE); 
                              break;
                          case 'x':
                              type = check_keyword(scan, 2, 2, "it", TOKEN_EXIT);
                              break;
                      }
                  break;
        case 'f':
                  if (scan->current - scan->start > 1)
                      switch(scan->start[1]) {
                          case 'a': 
                              type = check_keyword(scan, 2, 3, "lse", TOKEN_FALSE);
                              break;
                          case 'o':
                              type = check_keyword(scan, 2, 1, "r", TOKEN_FOR);
                              break;
                          case 'u':
                              type = check_keyword(scan, 2, 1, "n", TOKEN_FUN);
                              break;
                      }
                  break;
        case 'i': type = check_keyword(scan, 1, 1, "f", TOKEN_IF); break;
        case 'n': type = check_keyword(scan, 1, 3, "ull", TOKEN_NULL); break;
        case 'o': type = check_keyword(scan, 1, 2, "r", TOKEN_OR); break;
        case 'p': type = check_keyword(scan, 1, 4, "rint", TOKEN_PRINT); break;
        case 'r': type = check_keyword(scan, 1, 5, "eturn", TOKEN_RETURN); break;
        case 's': type = check_keyword(scan, 1, 4, "uper", TOKEN_SUPER); break;
        case 't':
                  if (scan->current - scan->start > 1)
                      switch (scan->start[1]) {
                          case 'h': 
                              type = check_keyword(scan, 2, 2, "is", TOKEN_THIS);
                              break;
                          case 'r':
                              type = check_keyword(scan, 2, 2, "ue", TOKEN_TRUE);
                              break;
                      }
                  break;
        case 'v': type = check_keyword(scan, 1, 2, "ar", TOKEN_VAR); break;
        case 'w': type = check_keyword(scan, 1, 4, "hile", TOKEN_WHILE); break;
    }
    add_token(scan, type);
}

static inline void number(scanner *scan)
{
    while (is_digit(peek(scan))) advance(scan);

    if (peek(scan) == '.' && is_digit(peek_next(scan))) {
        advance(scan);

        while (is_digit(peek(scan))) advance(scan);
    }

    add_token(scan, TOKEN_NUMBER);
}


static void error_token(scanner *scan, const char *msg)
{
    check_scanner_capacity(scan);
    token *tok = ALLOCATE(token, 1);

    tok->type = TOKEN_ERROR;
    tok->start = msg;
    tok->length = strlen(msg);
    tok->line = scan->line;
    scan->tokens[scan->num_tokens++] = tok;
}

static bool match(scanner *scan, char expected)
{
    char c = *scan->current;
    if (is_at_end(scan))
        return false;
    if (c != expected)
        return false;

    scan->current++;
    return true;
}

static inline void string(scanner *scan)
{
    while (peek(scan) != '"' && !is_at_end(scan)) {
        if (peek(scan) == '\n') scan->line++;
        advance(scan);
    }

    if (is_at_end(scan)) return error_token(scan, "Unterminated string.");

    advance(scan);
    add_token(scan, TOKEN_STRING);
}

static void scan_token(scanner *scan)
{
    if (is_at_end(scan)) return add_token(scan, TOKEN_EOF);

    char c = advance(scan);
    switch(c) {
        case '(':   add_token(scan, TOKEN_LEFT_PAREN); break;
        case ')':   add_token(scan, TOKEN_RIGHT_PAREN); break;
        case '{':   add_token(scan, TOKEN_LEFT_BRACE); break;
        case '}':   add_token(scan, TOKEN_RIGHT_BRACE); break;
        case '[':   add_token(scan, TOKEN_LEFT_BRACKET); break;
        case ']':   add_token(scan, TOKEN_RIGHT_BRACKET); break;
        case ',':   add_token(scan, TOKEN_COMMA); break;
        case '.':   add_token(scan, TOKEN_DOT); break;
        case '-':   add_token(scan, TOKEN_MINUS); break;
        case '+':   add_token(scan, TOKEN_PLUS); break;
        case '*':   add_token(scan, TOKEN_STAR); break;
        case '/':
                    if (match(scan, '/'))
                        while (peek(scan) != '\n' && !is_at_end(scan))
                            advance(scan);
                    else
                        add_token(scan, TOKEN_SLASH);
                    break;
        case ';':   add_token(scan, TOKEN_SEMICOLON); break;
        case '!':
                    add_token(scan, match(scan, '=') 
                            ? TOKEN_BANG_EQUAL: TOKEN_BANG);
                    break;
        case '=':
                    add_token(scan, match(scan, '=')
                            ? TOKEN_EQUAL_EQUAL: TOKEN_EQUAL);
                    break;
        case '<':
                    add_token(scan, match(scan, '=')
                            ? TOKEN_LESS_EQUAL : TOKEN_LESS);
                    break;
        case '>':
                    add_token(scan, match(scan, '=')
                            ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case ' ':
        case '\r':
        case '\t':
            // Ignore whitespace
            break;
        case '\n':
            scan->line++;
            break;
        case '"': string(scan); break;
        default:
                  if (is_digit(c))
                      number(scan);
                  else if (is_alpha(c))
                      identifier(scan);
                  else
                      return error_token(scan, "Unexpected character.");
                  break;
    }
}

static inline void scan_next(scanner *scan)
{
    scan->start = scan->current;
    scan_token(scan);
}

void scan_tokens(scanner *scan, const char *source)
{
    check_scanner_capacity(scan);
    scan->source = source;

    scan->length = strlen(scan->source);
    scan->start = scan->current = scan->source;
    while (!is_at_end(scan)) {
        scan_next(scan);
    }
    scan_next(scan);
}
