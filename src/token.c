#include "token.h"
#include <assert.h>

const char *tok_type_name(token_type_t t) {
    _Static_assert(TOK_COUNT == 8,
                   "Exhaustive handling of token types inside token_type_name");

    switch (t) {
    case TOK_INT:
        return "TOK_INT";
    case TOK_PLUS:
        return "TOK_PLUS";
    case TOK_MINUS:
        return "TOK_MINUS";
    case TOK_STAR:
        return "TOK_STAR";
    case TOK_SLASH:
        return "TOK_SLASH";
    case TOK_DUMP:
        return "TOK_DUMP";
    case TOK_EOF:
        return "TOK_EOF";
    case TOK_UNKNOWN:
        return "TOK_UNKNOWN";
    case TOK_COUNT:
        return "TOK_COUNT";
    default:
        return "?";
    }
}
