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
    token_kind_t type;
    char name[10];
    union {
        long long ival;
        size_t jmp_addr;
    };
    struct {
        const char *file;
        size_t row;
        size_t col;
    } location;
} token_t;

void lexer_init(lexer_t *l, const char *src);
void lexer_free(lexer_t *l);
token_t lexer_next(lexer_t *l);
token_t lexer_peek_next(lexer_t *l, bool *success);

#endif
