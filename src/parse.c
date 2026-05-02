#include "parse.h"
#include "error.h"
#include "lexer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline int _idx_cmp_char_arr(const da_t *arr, size_t i, void *data) {
    char *cur = (char *)da_get(arr, i);
    if (cur != NULL && data != NULL)
        return strcmp(cur, data);
    return -1;
}

void parse(da_t *prog, const char *src) {
    assert(TOKEN_IDENT_COUNT == 18 &&
           "Exhaustive handling of token types inside parse");

    lexer_t lex;
    lexer_init(&lex, src);

    *prog = da_new(op_t);
    da_t cross_reference_stack = da_new(size_t);
    da_t bind_reference_stack = da_new(size_t);
    da_t bindings_list = da_new(char[MAX_IDENTIFIER_LENGTH + 1]);
    bindings_list.compare_cb = &_idx_cmp_char_arr;

    token_t tok;
    do {
        tok = lexer_next(&lex);
        op_t new_op = {0};
        op_t *op = da_push(prog, &new_op);

        switch (tok.type) {
        case TOKEN_NUMBER:
            op->type = OP_PUSH_INT;
            op->ival = tok.ival;
            break;
        case TOKEN_IDENT:
            if (da_has(&token_builtins, tok.name)) {
                op->type = op_name_type(tok.name);

                switch (op->type) {
                case OP_IF:
                    da_push(&cross_reference_stack, &prog->length);
                    break;
                case OP_ELSE:
                case OP_END: {
                    if (cross_reference_stack.length == 0)
                        err_throw(ERR_UNEXPECTED_KEYWORD,
                                  ERR_CTX(tok.location, tok.name));
                    op_t *block_begin = (op_t *)da_get(
                        prog, *(size_t *)da_pop(&cross_reference_stack) - 1);
                    block_begin->jmp_addr = prog->length - 1;
                    if (op->type == OP_ELSE)
                        da_push(&cross_reference_stack, &prog->length);
                    break;
                }
                case OP_LET: {
                    if (bind_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op_t *ident_op = (op_t *)da_get(prog, *(size_t *)da_pop(&bind_reference_stack) - 1);
                    
                    if (ident_op->type != OP_IDENT) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    if (da_has(&bindings_list, ident_op->name)) err_throw(ERR_REDEFINITION, ERR_CTX(ident_op->location, ident_op->name));
                    
                    strcpy(op->name, ident_op->name);
                    da_push(&bindings_list, &ident_op->name);
                    break;
                }
                case OP_SET: {
                    op_t *ident_op = (op_t *)da_get(prog, prog->length - 2);
                    if (ident_op == NULL || ident_op->type != OP_PUSH_IDENT)
                        err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    ident_op->type = OP_SET;
                    da_pop(prog);
                    continue;
                }
                default:
                    break;
                }
            } else {
                if (da_has(&bindings_list, tok.name)) {
                    op->type = OP_PUSH_IDENT;
                } else {
                    if (strlen(tok.name) > MAX_IDENTIFIER_LENGTH)
                        err_throw(ERR_IDENTIFIER_TOO_LONG, ERR_CTX(tok.location, tok.name));
                    op->type = OP_IDENT;
                    da_push(&bind_reference_stack, &prog->length);
                }
                strcpy(op->name, tok.name);
            }
            break;
        case TOKEN_EOF:
            da_pop(prog);
            continue;
        default:
            break;
        }

        memcpy(&op->location, &tok.location, sizeof(tok.location));
    } while (tok.type != TOKEN_EOF);

    if (cross_reference_stack.length != 0) {
        while (cross_reference_stack.length > 0) {
            op_t *op = (op_t *)da_get(prog, *(size_t *)da_pop(&cross_reference_stack) - 1);
            err_print(ERR_UNCLOSED_BLOCK, ERR_CTX(op->location, NULL));
        }
        exit(1);
    }
    if (bind_reference_stack.length != 0) {
        while (bind_reference_stack.length > 0) {
            op_t *op = (op_t *)da_get(prog, *(size_t *)da_pop(&bind_reference_stack) - 1);
            err_print(ERR_UNDEFINED_SYMBOL, ERR_CTX(op->location, op->name));
        }
        exit(1);
    }

    lexer_free(&lex);
}

const char *op_type_name(op_type_t o) {
    _Static_assert(OP_COUNT == 21,
                   "Exhaustive handling of operator types inside op_type_name");

    switch (o) {
    case OP_PUSH_INT:       return "OP_PUSH_INT";
    case OP_PLUS:           return "OP_PLUS";
    case OP_MINUS:          return "OP_MINUS";
    case OP_STAR:           return "OP_STAR";
    case OP_SLASH:          return "OP_SLASH";
    case OP_EQUALS:         return "OP_EQUALS";
    case OP_GREATER:        return "OP_GREATER";
    case OP_GREATER_EQUALS: return "OP_GREATER_EQUALS";
    case OP_LESS:           return "OP_LESS";
    case OP_LESS_EQUALS:    return "OP_LESS_EQUALS";
    case OP_NOT:            return "OP_NOT";
    case OP_NOT_EQUALS:     return "OP_NOT_EQUALS";
    case OP_DUMP:           return "OP_DUMP";
    case OP_DUP:            return "OP_DUP";
    case OP_IF:             return "OP_IF";
    case OP_END:            return "OP_END";
    case OP_ELSE:           return "OP_ELSE";
    case OP_LET:            return "OP_LET";
    case OP_SET:            return "OP_SET";
    case OP_IDENT:          return "OP_IDENT";
    case OP_PUSH_IDENT:     return "OP_PUSH_IDENT";
    default:                return "OP_UNKNOWN";
    }
}

op_type_t op_name_type(const char *name) {
    _Static_assert(OP_COUNT == 21,
                   "Exhaustive handling of operator types inside op_name_type");

    if (strcmp(name, "+")    == 0) return OP_PLUS;
    if (strcmp(name, "-")    == 0) return OP_MINUS;
    if (strcmp(name, "*")    == 0) return OP_STAR;
    if (strcmp(name, "/")    == 0) return OP_SLASH;
    if (strcmp(name, "=")    == 0) return OP_EQUALS;
    if (strcmp(name, ">")    == 0) return OP_GREATER;
    if (strcmp(name, ">=")   == 0) return OP_GREATER_EQUALS;
    if (strcmp(name, "<")    == 0) return OP_LESS;
    if (strcmp(name, "<=")   == 0) return OP_LESS_EQUALS;
    if (strcmp(name, "!")    == 0) return OP_NOT;
    if (strcmp(name, "!=")   == 0) return OP_NOT_EQUALS;
    if (strcmp(name, "dump") == 0) return OP_DUMP;
    if (strcmp(name, "dup")  == 0) return OP_DUP;
    if (strcmp(name, "if")   == 0) return OP_IF;
    if (strcmp(name, "end")  == 0) return OP_END;
    if (strcmp(name, "else") == 0) return OP_ELSE;
    if (strcmp(name, "let")  == 0) return OP_LET;
    if (strcmp(name, "set")  == 0) return OP_SET;
    return OP_IDENT;
}
