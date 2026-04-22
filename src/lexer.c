#include "lexer.h"
#include "file_reader.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

long long str_to_int(const char *str, size_t len) {
    long long retval = 0;

    while (*str && isdigit(*str) && len--) {
        int digit = *str++ - '0';
        if (retval > (LLONG_MAX - digit) / 10) {
            fprintf(stderr, "error: integer literal overflow\n");
            exit(1);
        }
        retval = retval * 10 + digit;
    }

    return retval;
}

void lexer_init(lexer_t *l, const char *src) {
    l->buf = NULL;
    read_entire_file(src, &l->buf);
    tokenize_file(l->buf, src, &l->src);
    l->pos = 0;
}

void lexer_free(lexer_t *l) {
    da_free(&l->src);
    free(l->buf);
    l->buf = NULL;
}

token_t lexer_next(lexer_t *l) {
    token_t tok = {0};

    // end
    if (l->pos == l->src.length) {
        tok.type = TOK_EOF;
        return tok;
    }

    text_token_t cur_tt = *(text_token_t *)da_get(&l->src, l->pos);
    const char *text = cur_tt.token;

    _Static_assert(TOK_COUNT == 8,
                   "Exhaustive handling of token types inside lexer_next()");

    if (*text == '+') {
        tok.type = TOK_PLUS;
        l->pos++;
        return tok;
    }
    if (*text == '-') {
        tok.type = TOK_MINUS;
        l->pos++;
        return tok;
    }
    if (*text == '*') {
        tok.type = TOK_STAR;
        l->pos++;
        return tok;
    }
    if (*text == '/') {
        tok.type = TOK_SLASH;
        l->pos++;
        return tok;
    }

    // integer literal
    if (isdigit(*text)) {
        tok.type = TOK_INT;

        tok.ival = str_to_int(text, cur_tt.token_len);
        l->pos++;

        return tok;
    }

    if (isalpha(*text)) {
        if (cur_tt.token_len == 4 && __builtin_memcmp(text, "dump", 4) == 0) {
            tok.type = TOK_DUMP;
            l->pos++;
            return tok;
        }

        fprintf(stderr, "%s:%llu:%llu: error: unknown symbol: %.*s\n",
                cur_tt.file_name, cur_tt.row, cur_tt.column, cur_tt.token_len,
                cur_tt.token);
        exit(1);
    }

    tok.type = TOK_UNKNOWN;
    l->pos++;
    return tok;
}
