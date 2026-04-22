#include "lexer.h"
#include "file_reader.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

long long str_to_int(const char *str, size_t len) {
    long long retval = 0;

    while (*str && isdigit(*str) && len--) {
        retval = retval * 10 + (*str++ - '0');
    }

    return retval;
}

void lexer_init(lexer_t *l, const char *src) {
    char *buf = NULL;
    read_entire_file(src, &buf);
    tokenize_file(buf, src, &l->src);
    l->pos = 0;
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

    assert(TOK_COUNT == 8 &&
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
