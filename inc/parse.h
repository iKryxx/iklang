#ifndef PARSE_H
#define PARSE_H

#include "da.h"

typedef enum {
    OP_PUSH_INT,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_EQUALS,
    OP_GREATER,
    OP_GREATER_EQUALS,
    OP_LESS,
    OP_LESS_EQUALS,
    OP_NOT,
    OP_NOT_EQUALS,
    OP_DUMP,
    OP_DUP,
    OP_IF,
    OP_COUNT
} op_type_t;

typedef struct {
    op_type_t type;
    long long ival;
} op_t;

void parse(da_t *prog, const char *src);

#endif
