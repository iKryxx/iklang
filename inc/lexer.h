#ifndef LEXER_H
#define LEXER_H

#include "da.h"
#include "token.h"

typedef struct {
    char *buf;  // Heap-allocated source file content; owned by lexer
    da_t src;   // Text tokens referencing into buf
    size_t pos; // current position in src
} lexer_t;

typedef struct {
    token_type_t type;
    long long ival;
} token_t;

void lexer_init(lexer_t *l, const char *src);
void lexer_free(lexer_t *l);
token_t lexer_next(lexer_t *l);

#endif
