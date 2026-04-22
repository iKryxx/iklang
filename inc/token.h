#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    /* literals */
    TOK_INT = 0, // integer literal (42, 31)

    /* arithmetic operators */
    TOK_PLUS,  // '+'
    TOK_MINUS, // '-'
    TOK_STAR,  // '*'
    TOK_SLASH, // '/'

    /* builtins */
    TOK_DUMP, // "dump"

    /* control */
    TOK_EOF,     // '-1'
    TOK_UNKNOWN, // THROW
    TOK_COUNT
} token_type_t;

const char *tok_type_name(token_type_t t);

#endif
