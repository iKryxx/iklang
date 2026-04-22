#ifndef LEXER_H
#define LEXER_H

#include "da.h"
#include "token.h"

typedef struct {
    da_t src;   // Pointer to source string
    size_t pos; // current position in src
} lexer_t;

typedef struct {
    token_type_t type;
    int ival;
} token_t;

void lexer_init(lexer_t *l, const char *src);
token_t lexer_next(lexer_t *l);

#endif
