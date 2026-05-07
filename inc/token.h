#ifndef TOKEN_H
#define TOKEN_H

#include "da.h"
#include <string.h>

static inline int _idx_cmp_str(const da_t *arr, size_t i, void *data) {
    char *cur = *(char **)da_get(arr, i);
    if (cur != NULL && data != NULL) return strcmp(cur, data);
    return -1;
}

da_new_static(token_builtins, const char *, &_idx_cmp_str, "+", "-", "*", "/", "=", ">", ">=", "<", "<=", "!", "!=", "dump", "dup", "if", "end", "else", "while", "do", "let", "set", "mem", "load", "syscall3", "macro", "include");
#define TOKEN_IDENT_COUNT token_builtins.length

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENT,
    TOKEN_STRING,
    TOKEN_EOF,
    TOKEN_COUNT
} token_kind_t;

#endif
