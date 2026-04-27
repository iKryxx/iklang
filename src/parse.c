#include "parse.h"
#include "file_reader.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void parse(da_t *prog, const char *src) {
    _Static_assert(TOK_COUNT == 17,
                   "Exhaustive handling of token types inside parse");

    lexer_t lex;
    lexer_init(&lex, src);

    *prog = da_new(op_t);

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
            op.type = OP_IF;
            break;
        case TOK_EOF:
            lexer_free(&lex);
            return;
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
    lexer_free(&lex);
}
