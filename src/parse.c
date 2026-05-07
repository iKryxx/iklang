#include "parse.h"
#include "error.h"
#include "lexer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name [MAX_IDENTIFIER_LENGTH + 1];
    da_t ops;
} macro_t;

static inline int _idx_cmp_char_arr(const da_t *arr, size_t i, void *data) {
    char *cur = (char *)da_get(arr, i);
    if (cur != NULL && data != NULL)
        return strcmp(cur, data);
    return -1;
}

static inline int _idx_cmp_op_arr_name(const da_t *arr, size_t i, void *data) {
    macro_t *cur = (macro_t *)da_get(arr, i);
    if(cur != NULL && data != NULL)
        return strcmp(cur->name, data);
    return -1;
}


void parse(da_t *prog, const char *src) {
    assert(TOKEN_IDENT_COUNT == 24 &&
           "Exhaustive handling of token types inside parse");

    lexer_t lex;
    lexer_init(&lex, src);

    *prog = da_new(op_t);
    da_t cross_reference_stack = da_new(size_t);
    da_t bind_reference_stack = da_new(size_t);
    da_t bindings_list = da_new(char[MAX_IDENTIFIER_LENGTH + 1]);
    bindings_list.compare_cb = &_idx_cmp_char_arr;
    da_t macros_list = da_new(macro_t);
    macros_list.compare_cb = &_idx_cmp_op_arr_name;

    token_t tok;
    bool is_in_macro = false;
    macro_t cur_macro = { 0 };

    do {
        tok = lexer_next(&lex);
        op_t new_op = {0};
        da_t *cur_op_list = is_in_macro ? &cur_macro.ops : prog;
        op_t *op = da_push(cur_op_list, &new_op);

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
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    break;
                case OP_ELSE: {
                    if (cross_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op_t *block_begin = (op_t *)da_get(cur_op_list, *(size_t *)da_pop(&cross_reference_stack) - 1);
                    if (block_begin->type != OP_IF && block_begin->type != OP_ELSE)
                        err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    block_begin->jmp_addr = cur_op_list->length - 1;
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    break;
                }
                case OP_ENDIF: {
                    if (cross_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op_t *block_begin = (op_t *)da_get(cur_op_list, *(size_t *)da_pop(&cross_reference_stack) - 1);
                    if (block_begin->type == OP_IF || block_begin->type == OP_ELSE) {
                        block_begin->jmp_addr = cur_op_list->length - 1;
                    } else if (block_begin->type == OP_DO) {
                        op->jmp_addr = block_begin->jmp_addr;
                        block_begin->jmp_addr = cur_op_list->length - 1;
                        op->type = OP_ENDWHILE;
                    } else if (block_begin->type == OP_MACRO) {
                        da_push(&macros_list, &cur_macro);
                        op->type = OP_ENDMACRO;
                        is_in_macro = false;
                    } else {
                        err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    }
                    break;
                }
                case OP_WHILE:
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    break;
                case OP_DO: {
                    if (cross_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    size_t block_idx = *(size_t *)da_pop(&cross_reference_stack);
                    op_t *block_begin = (op_t *)da_get(cur_op_list, block_idx - 1);
                    if (block_begin->type != OP_WHILE) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op->jmp_addr = block_idx - 1;
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    break;
                }
                case OP_LET: {
                    if (bind_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op_t *ident_op = (op_t *)da_get(cur_op_list, *(size_t *)da_pop(&bind_reference_stack) - 1);
                    if (ident_op->type != OP_IDENT) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    if (da_has(&bindings_list, ident_op->name)) err_throw(ERR_REDEFINITION, ERR_CTX(ident_op->location, ident_op->name));
                    strcpy(op->name, ident_op->name);
                    da_push(&bindings_list, &ident_op->name);
                    break;
                }
                case OP_SET_VALUE: {
                    op_t *prev_op = (op_t *)da_get(cur_op_list, cur_op_list->length - 2);
                    if (prev_op == NULL) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    if (prev_op->type == OP_PUSH_IDENT) {
                        prev_op->type = OP_SET_VALUE;
                        da_pop(cur_op_list);
                        continue;
                    }
                    if (prev_op->type == OP_LOAD) {
                        prev_op->type = OP_SET_ARR_IDX;
                        da_pop(cur_op_list);
                        continue;
                    }
                    err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                }
                case OP_LOAD: {
                    op_t *prev_op = (op_t *)da_get(cur_op_list, cur_op_list->length - 2);
                    if (prev_op == NULL || prev_op->type != OP_PUSH_IDENT)
                        err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    prev_op->type = OP_LOAD;
                    da_pop(cur_op_list);
                    continue;
                }
                case OP_MACRO: {
                    if (is_in_macro) err_throw(ERR_NESTED_MACRO_DEFINITION, ERR_CTX(tok.location, NULL));
                    cur_macro.ops = da_new(op_t);
                    cur_op_list = &cur_macro.ops;
                    da_pop(prog); op = da_push(cur_op_list, op);
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    if (bind_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    op_t *ident_op = (op_t *)da_get(prog, *(size_t *)da_pop(&bind_reference_stack) - 1);
                    if (ident_op->type != OP_IDENT) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    if (da_has(&bindings_list, ident_op->name)) err_throw(ERR_REDEFINITION, ERR_CTX(ident_op->location, ident_op->name));
                    strcpy(cur_macro.name, ident_op->name);
                    is_in_macro = true;
                    break;
                }
                default:
                    break;
                }
            } else {
                if (da_has(&bindings_list, tok.name)) {
                    op->type = OP_PUSH_IDENT;
                } else if (da_has(&macros_list, tok.name)) {
                    da_pop(cur_op_list);
                    macro_t *macro = da_get(&macros_list, da_idx_of(&macros_list, tok.name));
                    size_t offset = cur_op_list->length;
                    for (size_t i = 1; i < macro->ops.length - 1; i++) {
                        op_t *macro_op = da_get(&macro->ops, i);
                        switch (macro_op->type) {
                        case OP_IF:
                        case OP_ELSE:
                        case OP_DO:
                        case OP_ENDWHILE:
                            macro_op->jmp_addr += offset - 1;
                            break;
                        default:
                            break;
                        }
                        da_push(cur_op_list, macro_op);
                    }
                    continue;
                } else {
                    if (strlen(tok.name) > MAX_IDENTIFIER_LENGTH)
                        err_throw(ERR_IDENTIFIER_TOO_LONG, ERR_CTX(tok.location, tok.name));
                    op->type = OP_IDENT;
                    da_push(&bind_reference_stack, &cur_op_list->length);
                }
                strcpy(op->name, tok.name);
            }
            break;
        case TOKEN_STRING:
            op->type = OP_STRING_LITERAL;
            op->str_literal = tok.str_value;
            break;
        case TOKEN_EOF:
            da_pop(cur_op_list);
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
    _Static_assert(OP_COUNT == 31,
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
    case OP_ENDIF:          return "OP_END";
    case OP_ELSE:           return "OP_ELSE";
    case OP_WHILE:          return "OP_WHILE";
    case OP_DO:             return "OP_DO";
    case OP_ENDWHILE:       return "OP_ENDWHILE";
    case OP_LET:            return "OP_LET";
    case OP_SET_VALUE:      return "OP_SET_VALUE";
    case OP_SET_ARR_IDX:    return "OP_SET_ARR_IDX";
    case OP_IDENT:          return "OP_IDENT";
    case OP_PUSH_IDENT:     return "OP_PUSH_IDENT";
    case OP_MEM:            return "OP_MEM";
    case OP_LOAD:           return "OP_LOAD";
    case OP_STRING_LITERAL: return "OP_STRING_LITERAL";
    case OP_SYSCALL3:       return "OP_SYSCALL3";
    case OP_MACRO:          return "OP_MACRO";
    case OP_ENDMACRO:       return "OP_ENDMACRO";
    default:                return "OP_UNKNOWN";
    }
}

op_type_t op_name_type(const char *name) {
    _Static_assert(OP_COUNT == 31,
                   "Exhaustive handling of operator types inside op_name_type");

    if (strcmp(name, "+")        == 0) return OP_PLUS;
    if (strcmp(name, "-")        == 0) return OP_MINUS;
    if (strcmp(name, "*")        == 0) return OP_STAR;
    if (strcmp(name, "/")        == 0) return OP_SLASH;
    if (strcmp(name, "=")        == 0) return OP_EQUALS;
    if (strcmp(name, ">")        == 0) return OP_GREATER;
    if (strcmp(name, ">=")       == 0) return OP_GREATER_EQUALS;
    if (strcmp(name, "<")        == 0) return OP_LESS;
    if (strcmp(name, "<=")       == 0) return OP_LESS_EQUALS;
    if (strcmp(name, "!")        == 0) return OP_NOT;
    if (strcmp(name, "!=")       == 0) return OP_NOT_EQUALS;
    if (strcmp(name, "dump")     == 0) return OP_DUMP;
    if (strcmp(name, "dup")      == 0) return OP_DUP;
    if (strcmp(name, "if")       == 0) return OP_IF;
    if (strcmp(name, "end")      == 0) return OP_ENDIF; // COULD ALSO BE OP_ENDWHILE OR OP_ENDMACRO LATER
    if (strcmp(name, "else")     == 0) return OP_ELSE;
    if (strcmp(name, "while")    == 0) return OP_WHILE;
    if (strcmp(name, "do")       == 0) return OP_DO;
    if (strcmp(name, "let")      == 0) return OP_LET;
    if (strcmp(name, "set")      == 0) return OP_SET_VALUE; // COULD ALSO BE OP_SET_AT_PTR LATER
    if (strcmp(name, "mem")      == 0) return OP_MEM;
    if (strcmp(name, "load")     == 0) return OP_LOAD;
    if (strcmp(name, "syscall3") == 0) return OP_SYSCALL3;
    if (strcmp(name, "macro")    == 0) return OP_MACRO;
    // ignore OP_STRING as its not TOK_IDENT
    return OP_IDENT;
}
