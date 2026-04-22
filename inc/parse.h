#ifndef PARSE_H
#define PARSE_H

#include "da.h"

typedef enum {
    OP_PUSH_INT,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_DUMP,
    OP_COUNT
} op_type_t;

typedef struct {
    op_type_t type;
    long long ival;
} op_t;

void parse(da_t *prog, const char *src);

#endif
