#include "parse.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void parse(da_t *prog, const char *src) {
    assert(TOKEN_IDENT_COUNT == 16 && "Exhaustive handling of token types inside parse");

    lexer_t lex;
    lexer_init(&lex, src);

    *prog = da_new(op_t);
    da_t cross_reference_stack = da_new(size_t);

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
            if (!da_has(&token_builtins, tok.name)) {
                fprintf(stderr, "%s:%zu:%zu: error: undefined symbol `%s`.", tok.location.file, tok.location.row, tok.location.col, tok.name);
                lexer_free(&lex);
                exit(1);
            }

            op->type = op_name_type(tok.name);

            // cross referencing
            if (op->type == OP_IF) {
                da_push(&cross_reference_stack, &prog->length);
            } else if (op->type == OP_ELSE || op->type == OP_END) {
                if (cross_reference_stack.length == 0) {
                    fprintf(stderr, "%s:%zu:%zu: error: unexpected `%s` keyword.\n", tok.location.file, tok.location.row, tok.location.col, tok.name);
                    lexer_free(&lex);
                    exit(1);
                }

                op_t *block_begin_operator = (op_t *)da_get(prog, *(size_t *)da_pop(&cross_reference_stack) - 1);
                block_begin_operator->jmp_addr = prog->length - 1;

                if (op->type == OP_ELSE) {
                    da_push(&cross_reference_stack, &prog->length);
                }
            }
            break;
        default:
            break;
        }

        memcpy(&op->location, &tok.location, sizeof(tok.location));
    } while (tok.type != TOKEN_EOF);

    if (cross_reference_stack.length != 0) {
        for (size_t i = 0; i < cross_reference_stack.length; i++) {
            op_t *op = *(op_t **)da_pop(&cross_reference_stack);

            fprintf(stderr, "%s:%zu:%zu: error: unclosed `if` block.\n", op->location.file, op->location.row, op->location.col);
        }
        lexer_free(&lex);
        exit(1);
    }
    lexer_free(&lex);
}
/*
void _parse(da_t *prog, const char *src) {
    _Static_assert(TOK_COUNT == 19,
                   "Exhaustive handling of token types inside parse");

    lexer_t lex;
    lexer_init(&lex, src);

    *prog = da_new(op_t);
    da_t cross_reference_stack = da_new(token_t);

    token_t tok;
    do {
        tok = lexer_next(&lex);
        op_t op = {0};

        switch (tok.type) {
        case TOK_INT:
            op.type = OP_PUSH_INT;
            op.ival = tok.ival;
            break;
        case TOK_PLUS:
            op.type = OP_PLUS;
            break;
        case TOK_MINUS:
            op.type = OP_MINUS;
            break;
        case TOK_STAR:
            op.type = OP_STAR;
            break;
        case TOK_SLASH:
            op.type = OP_SLASH;
            break;
        case TOK_EQUALS:
            op.type = OP_EQUALS;
            break;
        case TOK_GREATER:
            op.type = OP_GREATER;
            break;
        case TOK_GREATER_EQUALS:
            op.type = OP_GREATER_EQUALS;
            break;
        case TOK_LESS:
            op.type = OP_LESS;
            break;
        case TOK_LESS_EQUALS:
            op.type = OP_LESS_EQUALS;
            break;
        case TOK_EXCLAM:
            op.type = OP_NOT;
            break;
        case TOK_NOT_EQUALS:
            op.type = OP_NOT_EQUALS;
            break;

        case TOK_DUMP:
            op.type = OP_DUMP;
            break;
        case TOK_DUP:
            op.type = OP_DUP;
            break;
        case TOK_IF:
            tok.ival = prog->length;
            da_push(&cross_reference_stack, &tok);
            op.type = OP_IF;
            break;
        case TOK_END: {
            if (cross_reference_stack.length == 0) {
                text_token_t *last_tt = da_get(&lex.src, lex.pos - 1);
                fprintf(stderr,
                        "%s:%llu:%llu: error: unexpected 'end' keyword.\n",
                        last_tt->file_name, last_tt->row, last_tt->column);
                lexer_free(&lex);
                exit(1);
            }
            token_t *block_begin_token = da_pop(&cross_reference_stack);
            op_t *block_begin_operator = da_get(prog, block_begin_token->ival);
            block_begin_operator->ival = prog->length;
            op.type = OP_END;
            break;
        }
        case TOK_ELSE: {
            tok.ival = prog->length;
            if (cross_reference_stack.length == 0) {
                text_token_t *last_tt = da_get(&lex.src, lex.pos - 1);
                fprintf(stderr,
                        "%s:%llu:%llu: error: unexpected 'else' keyword.\n",
                        last_tt->file_name, last_tt->row, last_tt->column);
                lexer_free(&lex);
                exit(1);
            }
            token_t *block_begin_token = da_pop(&cross_reference_stack);
            op_t *block_begin_operator = da_get(prog, block_begin_token->ival);
            block_begin_operator->ival = prog->length;
            da_push(&cross_reference_stack, &tok);
            op.type = OP_ELSE;
            break;
        }
        case TOK_EOF:
            break;
        default:
            text_token_t *last_tt = da_get(&lex.src, lex.pos - 1);
            fprintf(stderr, "%s:%llu:%llu: error: unknown token: %.*s\n",
                    last_tt->file_name, last_tt->row, last_tt->column,
                    last_tt->token_len, last_tt->token);
            lexer_free(&lex);
            exit(1);
        }

        da_push(prog, &op);
    } while (tok.type != TOK_EOF);

    if (cross_reference_stack.length != 0) {
        token_t *tok = da_pop(&cross_reference_stack);
        text_token_t *last_tt = da_get(&lex.src, tok->ival);

        fprintf(stderr, "%s:%llu:%llu: error: unclosed 'if' block\n",
                last_tt->file_name, last_tt->row, last_tt->column);
        lexer_free(&lex);
        exit(1);
    }
    lexer_free(&lex);
}
*/

const char *op_type_name(op_type_t o) {
    _Static_assert(OP_COUNT == 17, "Exhaustive handling of operator types inside op_type_name");

    switch (o) {
    case OP_PUSH_INT:
        return "OP_PUSH_INT";
    case OP_PLUS:
        return "OP_PLUS";
    case OP_MINUS:
        return "OP_MINUS";
    case OP_STAR:
        return "OP_STAR";
    case OP_SLASH:
        return "OP_SLASH";
    case OP_EQUALS:
        return "OP_EQUALS";
    case OP_GREATER:
        return "OP_GREATER";
    case OP_GREATER_EQUALS:
        return "OP_GREATER_EQUALS";
    case OP_LESS:
        return "OP_LESS";
    case OP_LESS_EQUALS:
        return "OP_LESS_EQUALS";
    case OP_NOT:
        return "OP_NOT";
    case OP_NOT_EQUALS:
        return "OP_NOT_EQUALS";
    case OP_DUMP:
        return "OP_DUMP";
    case OP_DUP:
        return "OP_DUP";
    case OP_IF:
        return "OP_IF";
    case OP_END:
        return "OP_END";
    case OP_ELSE:
        return "OP_ELSE";
    default:
        return "OP_UNKNOWN";
    }
}

op_type_t op_name_type(const char *name) {
    _Static_assert(OP_COUNT == 17, "Exhaustive handling of operator types inside op_type_name");

    if (strcmp(name, "+") == 0) return OP_PLUS;
    if (strcmp(name, "-") == 0) return OP_MINUS;
    if (strcmp(name, "*") == 0) return OP_STAR;
    if (strcmp(name, "/") == 0) return OP_SLASH;
    if (strcmp(name, "=") == 0) return OP_EQUALS;
    if (strcmp(name, ">") == 0) return OP_GREATER;
    if (strcmp(name, ">=") == 0) return OP_GREATER_EQUALS;
    if (strcmp(name, "<") == 0) return OP_LESS;
    if (strcmp(name, "<=") == 0) return OP_LESS_EQUALS;
    if (strcmp(name, "!") == 0) return OP_NOT;
    if (strcmp(name, "!=") == 0) return OP_NOT_EQUALS;
    if (strcmp(name, "dump") == 0) return OP_DUMP;
    if (strcmp(name, "dup") == 0) return OP_DUP;
    if (strcmp(name, "if") == 0) return OP_IF;
    if (strcmp(name, "end") == 0) return OP_END;
    if (strcmp(name, "else") == 0) return OP_ELSE;

    assert(false && "unreachable reached inside op_name_type");
    return 1;
}
