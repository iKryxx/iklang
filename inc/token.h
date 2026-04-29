#ifndef TOKEN_H
#define TOKEN_H

#include "da.h"
#include <string.h>

static inline int _idx_cmp_str(const da_t *arr, size_t i, void *data) {
    char *cur = *(char **)da_get(arr, i);
    if (cur != NULL && data != NULL) return strcmp(cur, data);
    return -1;
}

da_new_static(token_builtins, const char *, &_idx_cmp_str, "+", "-", "*", "/", "=", ">", ">=", "<", "<=", "!", "!=", "dump", "dup", "if", "end", "else");
#define TOKEN_IDENT_COUNT token_builtins.length

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENT,
    TOKEN_EOF,
    TOKEN_COUNT
} token_kind_t;

typedef enum {
    /* literals */
    TOK_INT = 0, // integer literal (42, 31)

    /* arithmetic operators */
    TOK_PLUS,  // '+'
    TOK_MINUS, // '-'
    TOK_STAR,  // '*'
    TOK_SLASH, // '/'

    /* boolean operators */
    TOK_EQUALS,         // '='
    TOK_GREATER,        // '>'
    TOK_GREATER_EQUALS, // ">="
    TOK_LESS,           // '<'
    TOK_LESS_EQUALS,    // "<="
    TOK_EXCLAM,         // '!'
    TOK_NOT_EQUALS,     // "!="

    /* builtins */
    TOK_DUMP, // "dump"
    TOK_DUP,  // "dup"
    TOK_IF,   // "if"
    TOK_END,  // "end"
    TOK_ELSE, // "else"

    /* control */
    TOK_EOF,     // '-1'
    TOK_UNKNOWN, // THROW
    TOK_COUNT
} token_type_t;

const char *tok_type_name(token_type_t t);

#endif
