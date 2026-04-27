#include "token.h"
#include <assert.h>

const char *tok_type_name(token_type_t t) {
    _Static_assert(TOK_COUNT == 17,
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
    case TOK_EQUALS:
        return "TOK_EQUALS";
    case TOK_GREATER:
        return "TOK_GREATER";
    case TOK_GREATER_EQUALS:
        return "TOK_GREATER_EQUALS";
    case TOK_LESS:
        return "TOK_LESS";
    case TOK_LESS_EQUALS:
        return "TOK_LESS_EQUALS";
    case TOK_EXCLAM:
        return "TOK_EXCLAM";
    case TOK_NOT_EQUALS:
        return "TOK_NOT_EQUALS";
    case TOK_DUMP:
        return "TOK_DUMP";
    case TOK_DUP:
        return "TOK_DUP";
    case TOK_IF:
        return "TOK_IF";
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
