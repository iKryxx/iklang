#define _GNU_SOURCE
#include "parse.h"
#include "error.h"
#include "lexer.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef IKLANG_LIB_DIR
#define IKLANG_LIB_DIR "/usr/local/lib/iklang"
#endif

typedef struct {
    char name [MAX_IDENTIFIER_LENGTH + 1];
    da_t ops;
} macro_t;

typedef struct {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    struct { const char *file; size_t row; size_t col; } location;
    type_id_t type;
} bind_ref_t;

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

static void parse_file(
    const char *src,
    da_t *prog,
    da_t *bindings_list,
    da_t *bindings_types,
    da_t *macros_list,
    da_t *active_includes,
    da_t *done_includes
) {
    lexer_t lex;
    lexer_init(&lex, src);

    da_t cross_reference_stack = da_new(size_t);
    da_t bind_reference_stack = da_new(bind_ref_t);

    token_t tok;
    bool is_in_macro = false;
    macro_t cur_macro = { 0 };

    size_t tok_i = 0;

    do {
        tok = *(token_t*)da_get(&lex.tokens, tok_i++);

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
                        da_push(macros_list, &cur_macro);
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
                    bind_ref_t ref = *(bind_ref_t *)da_pop(&bind_reference_stack);
                    if (da_has(bindings_list, ref.name)) err_throw(ERR_REDEFINITION, ERR_CTX(ref.location, ref.name));
                    strcpy(op->name, ref.name);
                    op->value_type = ref.type;
                    da_push(bindings_list, ref.name);
                    da_push(bindings_types, &ref.type);
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
                    else if (prev_op->type == OP_LOAD_IDENT) {
                        prev_op->type = OP_SET_ARR_IDX;
                        da_pop(cur_op_list);
                        continue;
                    } else {
                        op->type = OP_SET_ADDR;
                        continue;
                    }
                    err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                }
                case OP_LOAD_IDENT: {
                    op_t *prev_op = (op_t *)da_get(cur_op_list, cur_op_list->length - 2);
                    if (prev_op == NULL) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    if(prev_op->type == OP_PUSH_IDENT) {
                        prev_op->type = OP_LOAD_IDENT;
                        da_pop(cur_op_list);
                        continue;
                    } else { 
                        op->type = OP_LOAD_ADDR;
                        continue;
                    }   
                }
                case OP_MACRO: {
                    if (is_in_macro) err_throw(ERR_NESTED_MACRO_DEFINITION, ERR_CTX(tok.location, NULL));
                    cur_macro.ops = da_new(op_t);
                    cur_op_list = &cur_macro.ops;
                    da_pop(prog); op = da_push(cur_op_list, op);
                    da_push(&cross_reference_stack, &cur_op_list->length);
                    if (bind_reference_stack.length == 0) err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));
                    bind_ref_t ref = *(bind_ref_t *)da_pop(&bind_reference_stack);
                    if (da_has(bindings_list, ref.name)) err_throw(ERR_REDEFINITION, ERR_CTX(ref.location, ref.name));
                    strcpy(cur_macro.name, ref.name);
                    is_in_macro = true;
                    break;
                }
                case OP_INCLUDE: {
                    op_t *prev_op = da_get(cur_op_list, cur_op_list->length - 2);
                    if (prev_op == NULL || prev_op->type != OP_STRING_LITERAL)
                        err_throw(ERR_UNEXPECTED_KEYWORD, ERR_CTX(tok.location, tok.name));

                    char canonical[PATH_MAX];
                    if (realpath(prev_op->str_literal, canonical) == NULL) {
                        char lib_path[PATH_MAX];
                        snprintf(lib_path, sizeof(lib_path), "%s/%s", IKLANG_LIB_DIR, prev_op->str_literal);
                        if (realpath(lib_path, canonical) == NULL) {
                            fprintf(stderr, "error: include '%s' not found (searched './' and '%s')\n",
                                    prev_op->str_literal, IKLANG_LIB_DIR);
                            exit(1);
                        }
                    }

                    if (da_has(active_includes, canonical))
                        err_throw(ERR_CIRCULAR_INCLUDE, ERR_CTX(tok.location, prev_op->str_literal));
                    if (da_has(done_includes, canonical))
                        err_throw(ERR_DOUBLE_INCLUDE, ERR_CTX(tok.location, prev_op->str_literal));

                    da_pop(cur_op_list);
                    da_pop(cur_op_list);

                    da_push(active_includes, canonical);
                    parse_file(canonical, prog, bindings_list, bindings_types, macros_list, active_includes, done_includes);
                    da_pop(active_includes);
                    da_push(done_includes, canonical);
                    continue;
                }
                default:
                    break;
                }
            } else {
                if (da_has(bindings_list, tok.name)) {
                    op->type = OP_PUSH_IDENT;
                    size_t idx = da_idx_of(bindings_list, tok.name);
                    op->value_type = *(type_id_t *)da_get(bindings_types, idx);
                } else if (da_has(macros_list, tok.name)) {
                    da_pop(cur_op_list);
                    macro_t *macro = da_get(macros_list, da_idx_of(macros_list, tok.name));
                    size_t offset = cur_op_list->length;
                    for (size_t i = 1; i < macro->ops.length - 1; i++) {
                        op_t copy = *(op_t *)da_get(&macro->ops, i);
                        switch (copy.type) {
                        case OP_IF:
                        case OP_ELSE:
                        case OP_DO:
                        case OP_ENDWHILE:
                            copy.jmp_addr += offset - 1;
                            break;
                        default:
                            break;
                        }
                        da_push(cur_op_list, &copy);
                    }
                    continue;
                } else {
                    if (strlen(tok.name) > MAX_IDENTIFIER_LENGTH)
                        err_throw(ERR_IDENTIFIER_TOO_LONG, ERR_CTX(tok.location, tok.name));
                    op->type = OP_IDENT;
                    bind_ref_t _ref = {0};
                    strcpy(_ref.name, tok.name);
                    memcpy(&_ref.location, &tok.location, sizeof(tok.location));
                    {
                        op_t *prev_val_op = (op_t *)da_get(cur_op_list, cur_op_list->length - 2);
                        type_id_t inferred = T_UNKNOWN;
                        if (prev_val_op != NULL) {
                            switch (prev_val_op->type) {
                            case OP_PUSH_INT:       inferred = T_INT; break;
                            case OP_STRING_LITERAL: inferred = T_STRING_LIT; break;
                            case OP_PUSH_IDENT:     inferred = prev_val_op->value_type; break;
                            default: break;
                            }
                        }
                        op->value_type = inferred;
                        _ref.type = inferred;
                    }
                    da_push(&bind_reference_stack, &_ref);
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
            bind_ref_t ref = *(bind_ref_t *)da_pop(&bind_reference_stack);
            err_print(ERR_UNDEFINED_SYMBOL, ERR_CTX(ref.location, ref.name));
        }
        exit(1);
    }

    lexer_free(&lex);
}


void parse(da_t *prog, const char *src) {
    assert(TOKEN_IDENT_COUNT == 36 &&
           "Exhaustive handling of token types inside parse");

    *prog = da_new(op_t);

    da_t bindings_list = da_new(char[MAX_IDENTIFIER_LENGTH + 1]);
    bindings_list.compare_cb = &_idx_cmp_char_arr;
    da_t bindings_types = da_new(type_id_t);
    const char *argc_name = "argc"; type_id_t argc_type = T_INT;
    const char *argv_name = "argv"; type_id_t argv_type = T_INT; // because its a ptr

    da_push(&bindings_list, argc_name);
    da_push(&bindings_list, argv_name);
    da_push(&bindings_types, &argc_type);
    da_push(&bindings_types, &argv_type);

    da_t macros_list = da_new(macro_t);
    macros_list.compare_cb = &_idx_cmp_op_arr_name;

    da_t active_includes = da_new(char[PATH_MAX]);
    active_includes.compare_cb = &_idx_cmp_char_arr;
    da_t done_includes = da_new(char[PATH_MAX]);
    done_includes.compare_cb = &_idx_cmp_char_arr;

    char canonical[PATH_MAX];
    if (realpath(src, canonical) == NULL) {
        perror(src);
        exit(1);
    }

    da_push(&active_includes, canonical);
    parse_file(canonical, prog, &bindings_list, &bindings_types, &macros_list, &active_includes, &done_includes);
    da_pop(&active_includes);
    da_push(&done_includes, canonical);
}

const char *op_type_name(op_type_t o) {
    _Static_assert(OP_COUNT == 45,
                   "Exhaustive handling of operator types inside op_type_name");

    switch (o) {
    case OP_PUSH_INT:       return "OP_PUSH_INT";
    case OP_PLUS:           return "OP_PLUS";
    case OP_MINUS:          return "OP_MINUS";
    case OP_STAR:           return "OP_STAR";
    case OP_SLASH:          return "OP_SLASH";
    case OP_PERCENT:        return "OP_PERCENT";
    case OP_EQUALS:         return "OP_EQUALS";
    case OP_GREATER:        return "OP_GREATER";
    case OP_GREATER_EQUALS: return "OP_GREATER_EQUALS";
    case OP_LESS:           return "OP_LESS";
    case OP_LESS_EQUALS:    return "OP_LESS_EQUALS";
    case OP_NOT:            return "OP_NOT";
    case OP_NOT_EQUALS:     return "OP_NOT_EQUALS";
    case OP_DUMP:           return "OP_DUMP";
    case OP_DUP:            return "OP_DUP";
    case OP_DROP:           return "OP_DROP";
    case OP_SWAP:           return "OP_SWAP";
    case OP_OVER:           return "OP_OVER";
    case OP_ROT:            return "OP_ROT";
    case OP_NIP:            return "OP_NIP";
    case OP_TUCK:           return "OP_TUCK";
    case OP_IF:             return "OP_IF";
    case OP_ENDIF:          return "OP_END";
    case OP_ELSE:           return "OP_ELSE";
    case OP_WHILE:          return "OP_WHILE";
    case OP_DO:             return "OP_DO";
    case OP_ENDWHILE:       return "OP_ENDWHILE";
    case OP_LET:            return "OP_LET";
    case OP_SET_VALUE:      return "OP_SET_VALUE";
    case OP_SET_ARR_IDX:    return "OP_SET_ARR_IDX";
    case OP_SET_ADDR:       return "OP_SET_ADDR";
    case OP_IDENT:          return "OP_IDENT";
    case OP_PUSH_IDENT:     return "OP_PUSH_IDENT";
    case OP_MEM:            return "OP_MEM";
    case OP_LOAD_IDENT:     return "OP_LOAD_IDENT";
    case OP_LOAD_ADDR:      return "OP_LOAD_ADDR";
    case OP_LOAD_HWORD:     return "OP_LOAD_HWORD";
    case OP_LOAD_SWORD:     return "OP_LOAD_SWORD";
    case OP_LOAD_DWORD:     return "OP_LOAD_DWORD";
    case OP_LOAD_QWORD:     return "OP_LOAD_QWORD";
    case OP_STRING_LITERAL: return "OP_STRING_LITERAL";
    case OP_SYSCALL3:       return "OP_SYSCALL3";
    case OP_MACRO:          return "OP_MACRO";
    case OP_ENDMACRO:       return "OP_ENDMACRO";
    case OP_INCLUDE:        return "OP_INCLUDE";
    default:                return "OP_UNKNOWN";
    }
}

op_type_t op_name_type(const char *name) {
    _Static_assert(OP_COUNT == 45,
                   "Exhaustive handling of operator types inside op_name_type");

    if (strcmp(name, "+")        == 0) return OP_PLUS;
    if (strcmp(name, "-")        == 0) return OP_MINUS;
    if (strcmp(name, "*")        == 0) return OP_STAR;
    if (strcmp(name, "/")        == 0) return OP_SLASH;
    if (strcmp(name, "%")        == 0) return OP_PERCENT;
    if (strcmp(name, "=")        == 0) return OP_EQUALS;
    if (strcmp(name, ">")        == 0) return OP_GREATER;
    if (strcmp(name, ">=")       == 0) return OP_GREATER_EQUALS;
    if (strcmp(name, "<")        == 0) return OP_LESS;
    if (strcmp(name, "<=")       == 0) return OP_LESS_EQUALS;
    if (strcmp(name, "!")        == 0) return OP_NOT;
    if (strcmp(name, "!=")       == 0) return OP_NOT_EQUALS;
    if (strcmp(name, "dump")     == 0) return OP_DUMP;
    if (strcmp(name, "dup")      == 0) return OP_DUP;
    if (strcmp(name, "drop")     == 0) return OP_DROP;
    if (strcmp(name, "swap")     == 0) return OP_SWAP;
    if (strcmp(name, "over")     == 0) return OP_OVER;
    if (strcmp(name, "rot")      == 0) return OP_ROT;
    if (strcmp(name, "nip")      == 0) return OP_NIP;
    if (strcmp(name, "tuck")     == 0) return OP_TUCK;
    if (strcmp(name, "if")       == 0) return OP_IF;
    if (strcmp(name, "end")      == 0) return OP_ENDIF; // COULD ALSO BE OP_ENDWHILE OR OP_ENDMACRO LATER
    if (strcmp(name, "else")     == 0) return OP_ELSE;
    if (strcmp(name, "while")    == 0) return OP_WHILE;
    if (strcmp(name, "do")       == 0) return OP_DO;
    if (strcmp(name, "let")      == 0) return OP_LET;
    if (strcmp(name, "set")      == 0) return OP_SET_VALUE; // COULD ALSO BE OP_SET_AT_PTR OR OP_SET_ADDR LATER 
    if (strcmp(name, "mem")      == 0) return OP_MEM;
    if (strcmp(name, "load")     == 0) return OP_LOAD_IDENT; // COULD ALSO BE OP_LOAD_ADDR LATER
    if (strcmp(name, "l8")       == 0) return OP_LOAD_HWORD;
    if (strcmp(name, "l16")      == 0) return OP_LOAD_SWORD;
    if (strcmp(name, "l32")      == 0) return OP_LOAD_DWORD;
    if (strcmp(name, "l64")      == 0) return OP_LOAD_QWORD;
    if (strcmp(name, "syscall3") == 0) return OP_SYSCALL3;
    if (strcmp(name, "macro")    == 0) return OP_MACRO;
    if (strcmp(name, "include")  == 0) return OP_INCLUDE;
    // ignore OP_STRING as its not TOK_IDENT
    return OP_IDENT;
}

typedef struct {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    type_id_t type;
} tc_binding_t;

typedef enum { TC_BLOCK_IF, TC_BLOCK_IF_HAS_ELSE, TC_BLOCK_WHILE } tc_block_kind_t;

typedef struct {
    tc_block_kind_t kind;
    da_t snapshot;
} tc_block_t;

static da_t tc_snapshot(const da_t *stack) {
    da_t snap = da_new(type_id_t);
    for (size_t i = 0; i < stack->length; i++)
        da_push(&snap, da_get(stack, i));
    return snap;
}

static bool tc_stacks_equal(const da_t *a, const da_t *b) {
    if (a->length != b->length) return false;
    for (size_t i = 0; i < a->length; i++)
        if (*(type_id_t *)da_get(a, i) != *(type_id_t *)da_get(b, i))
            return false;
    return true;
}

static void tc_restore(da_t *stack, const da_t *snap) {
    stack->length = 0;
    for (size_t i = 0; i < snap->length; i++)
        da_push(stack, da_get(snap, i));
}

static inline bool tc_is_int_compat(type_id_t t) {
    return t == T_INT || t == T_STRING_LIT;
}

static void tc_print_stack(const char *label, const da_t *stack) {
    fprintf(stderr, "%s\n", label);
    if (stack->length == 0) {
        fprintf(stderr, "        - (empty)\n");
    } else {
        for (size_t i = 0; i < stack->length; i++)
            fprintf(stderr, "        - %s\n", type_id_name(*(type_id_t *)da_get(stack, i)));
    }
}

void type_check(da_t *prog) {
    _Static_assert(OP_COUNT == 45,
                   "Exhaustive handling of operator types inside typecheck");
    _Static_assert(T_COUNT == 3,
                   "Exhaustive handling of types inside type_check");
    da_t type_stack = da_new(type_id_t);
    da_t tc_bindings = da_new(tc_binding_t);
    da_t tc_branch_stack = da_new(tc_block_t);

    op_t *op;
    for (size_t i = 0; i < prog->length; i++) {
        op = (op_t *)da_get(prog, i);

        switch(op->type) {
        case OP_PUSH_INT: {
            type_id_t t = T_INT;
            da_push(&type_stack, &t);
            break;
        }
        case OP_PLUS: 
        case OP_MINUS:
        case OP_STAR:
        case OP_SLASH:
        case OP_PERCENT:
        case OP_EQUALS:
        case OP_GREATER:
        case OP_GREATER_EQUALS:
        case OP_LESS:
        case OP_LESS_EQUALS:
        case OP_NOT_EQUALS:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));

            type_id_t *b = da_pop(&type_stack);
            type_id_t *a = da_pop(&type_stack);
            
            type_id_t out;

            if (tc_is_int_compat(*a) && tc_is_int_compat(*b)) {
                out = T_INT;
                da_push(&type_stack, &out);
            }
            else {
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX2(op->location, type_id_name(*a), type_id_name(*b)));
            }

            break;
        }
        case OP_NOT:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));

            type_id_t *a = da_pop(&type_stack);
            
            type_id_t out;

            if (tc_is_int_compat(*a)) {
                out = T_INT;
                da_push(&type_stack, &out);
            }
            else {
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*a)));
            }

            break;
        }
        case OP_DUMP:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));

            type_id_t *a = da_pop(&type_stack);
            
            if (!tc_is_int_compat(*a)) {
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*a)));
            }

            break;
        }
        case OP_DUP:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            type_id_t *a = da_get(&type_stack, type_stack.length - 1);
            da_push(&type_stack, a);
            break;
        }
        case OP_DROP:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            da_pop(&type_stack);
            break;
        }
        case OP_SWAP:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            break;
        }
        case OP_OVER:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            type_id_t *b = da_get(&type_stack, type_stack.length - 2);
            da_push(&type_stack, b);
            break;
        }
        case OP_ROT:
        {
            if (type_stack.length < 3) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            break;
        }
        case OP_NIP:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            da_remove(&type_stack, type_stack.length - 2);
            break;
        }
        case OP_TUCK:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            type_id_t *b = da_get(&type_stack, type_stack.length - 1);
            da_insert(&type_stack, type_stack.length - 1, b);
            break;
        }
        case OP_IF:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, "if"));
            type_id_t *cond = da_pop(&type_stack);
            if (*cond != T_INT)
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*cond)));
            tc_block_t block = { .kind = TC_BLOCK_IF, .snapshot = tc_snapshot(&type_stack) };
            da_push(&tc_branch_stack, &block);
            break;
        }
        case OP_ELSE:
        {
            tc_block_t *top = (tc_block_t *)da_get(&tc_branch_stack, tc_branch_stack.length - 1);
            da_t then_snap = tc_snapshot(&type_stack);
            tc_restore(&type_stack, &top->snapshot);
            da_free(&top->snapshot);
            top->snapshot = then_snap;
            top->kind = TC_BLOCK_IF_HAS_ELSE;
            break;
        }
        case OP_ENDIF:
        {
            tc_block_t block = *(tc_block_t *)da_get(&tc_branch_stack, tc_branch_stack.length - 1);
            tc_branch_stack.length--;
            if (block.kind == TC_BLOCK_IF) {
                if (!tc_stacks_equal(&type_stack, &block.snapshot))
                    err_throw(ERR_BRANCH_STACK_MISMATCH, ERR_CTX(op->location, "if"));
            } else {
                if (!tc_stacks_equal(&type_stack, &block.snapshot)) {
                    fprintf(stderr, "%s:%zu:%zu: error: `if` and `else` branches leave the stack in different states.\n",
                            op->location.file, op->location.row, op->location.col);
                    tc_print_stack("    if branch state:", &block.snapshot);
                    tc_print_stack("    else branch state:", &type_stack);
                    da_free(&block.snapshot);
                    exit(1);
                }
            }
            da_free(&block.snapshot);
            break;
        }
        case OP_WHILE:
        {
            tc_block_t block = { .kind = TC_BLOCK_WHILE, .snapshot = tc_snapshot(&type_stack) };
            da_push(&tc_branch_stack, &block);
            break;
        }
        case OP_DO:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, "do"));
            type_id_t *cond = da_pop(&type_stack);
            if (*cond != T_INT)
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*cond)));
            tc_block_t *top = (tc_block_t *)da_get(&tc_branch_stack, tc_branch_stack.length - 1);
            if (!tc_stacks_equal(&type_stack, &top->snapshot))
                err_throw(ERR_BRANCH_STACK_MISMATCH, ERR_CTX(op->location, "do"));
            break;
        }
        case OP_ENDWHILE:
        {
            tc_block_t block = *(tc_block_t *)da_get(&tc_branch_stack, tc_branch_stack.length - 1);
            tc_branch_stack.length--;
            if (!tc_stacks_equal(&type_stack, &block.snapshot))
                err_throw(ERR_BRANCH_STACK_MISMATCH, ERR_CTX(op->location, "while"));
            da_free(&block.snapshot);
            break;
        }
        case OP_IDENT:
            break;
        case OP_LET:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op->name));
            type_id_t *top = (type_id_t *)da_get(&type_stack, type_stack.length - 1);
            tc_binding_t entry = {0};
            strcpy(entry.name, op->name);
            entry.type = *top;
            da_push(&tc_bindings, &entry);
            da_pop(&type_stack);
            break;
        }
        case OP_PUSH_IDENT:
        {
            type_id_t resolved = op->value_type;
            if (resolved == T_UNKNOWN) {
                for (size_t j = 0; j < tc_bindings.length; j++) {
                    tc_binding_t *b = (tc_binding_t *)da_get(&tc_bindings, j);
                    if (strcmp(b->name, op->name) == 0) { resolved = b->type; break; }
                }
            }
            da_push(&type_stack, &resolved);
            break;
        }
        case OP_SET_VALUE:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op->name));
            type_id_t *new_val = da_pop(&type_stack);
            for (size_t j = 0; j < tc_bindings.length; j++) {
                tc_binding_t *b = (tc_binding_t *)da_get(&tc_bindings, j);
                if (strcmp(b->name, op->name) == 0) {
                    if (b->type != T_UNKNOWN && *new_val != b->type)
                        err_throw(ERR_TYPE_MISSMATCH, ERR_CTX2(op->location, type_id_name(*new_val), type_id_name(b->type)));
                    b->type = *new_val;
                    break;
                }
            }
            break;
        }
        case OP_SET_ARR_IDX:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op->name));
            type_id_t *index = da_pop(&type_stack);
            if (*index != T_INT)
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*index)));
            da_pop(&type_stack); // value — any type allowed
            break;
        }
        case OP_SET_ADDR:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            type_id_t *addr = da_pop(&type_stack);
            if (*addr != T_INT)
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*addr)));
            da_pop(&type_stack); // value — any type allowed
            break;
        }
        case OP_MEM:
        {
            if (type_stack.length < 2) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op->name));
            type_id_t *stride = da_pop(&type_stack);
            type_id_t *size = da_pop(&type_stack);
            if(*stride != T_INT) err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*stride)));
            if(*size   != T_INT) err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*size)));
            type_id_t out = T_INT;
            da_push(&type_stack, &out);
            break;
        }
        case OP_LOAD_IDENT:
        case OP_LOAD_ADDR:
        case OP_LOAD_HWORD:
        case OP_LOAD_SWORD:
        case OP_LOAD_DWORD:
        case OP_LOAD_QWORD:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));
            type_id_t *idx_or_addr = da_pop(&type_stack);
            if (!tc_is_int_compat(*idx_or_addr))
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*idx_or_addr)));
            type_id_t out = T_INT;
            da_push(&type_stack, &out);
            break;
        }
        case OP_STRING_LITERAL:
        {
            type_id_t t = T_STRING_LIT;
            da_push(&type_stack, &t);
            break;
        }
        case OP_SYSCALL3:
        {
            // TODO: consider typechecking syscalls using a table that maps each syscall to expected types and a return type.
            da_pop(&type_stack);
            da_pop(&type_stack);
            da_pop(&type_stack);
            da_pop(&type_stack);
            type_id_t out = T_INT;
            da_push(&type_stack, &out);
            break;
        }
        case OP_MACRO:
        case OP_ENDMACRO:
        assert(false && "Unreachable reached: macros should be extended fully at parsing step.");
        case OP_INCLUDE:
        {
            if (type_stack.length < 1) err_throw(ERR_STACK_UNDERFLOW, ERR_CTX(op->location, op_type_name(op->type)));

            type_id_t *a = da_pop(&type_stack);
            
            if (*a == T_STRING_LIT ) {
                (void)0;
            }
            else {
                err_throw(ERR_TYPE_MISSMATCH, ERR_CTX(op->location, type_id_name(*a)));
            }

            break;
        }
        case OP_COUNT:
            assert(false && "Not implemented");
            break;
        }
    }

    if (type_stack.length != 0) {
        err_print(ERR_STACK_NOT_EMPTY, ERR_CTX(op->location, NULL));
        tc_print_stack("    stack state:", &type_stack);
        exit(1);
    }
}

const char *type_id_name(type_id_t t) {
    _Static_assert(T_COUNT == 3,
                   "Exhaustive handling of types inside type_id_name");
    switch(t) {
    case T_INT:        return "T_INT";
    case T_STRING_LIT: return "T_STRING_LIT";
    case T_UNKNOWN:    return "T_UNKNOWN";
    default:           return "T_UNKNOWN";
    }
}

type_id_t name_type_id(const char *name) {
    _Static_assert(T_COUNT == 3,
                   "Exhaustive handling of types inside name_type_id");

    if (strcmp(name, "T_INT")        == 0) return T_INT;
    if (strcmp(name, "T_STRING_LIT") == 0) return T_STRING_LIT;
    if (strcmp(name, "T_UNKNOWN")    == 0) return T_UNKNOWN;

    return T_COUNT;
}
