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

bool is_number(const text_token_t *tt) {
    const char *str = tt->token;

    if (!isdigit(*str)) return false;
    for (int i = 1; i < tt->token_len; i++) {
        if (!isdigit(str[i])) return false;
    }
    return true;
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
        tok.type = TOKEN_EOF;
        return tok;
    }

    text_token_t cur_tt = *(text_token_t *)da_get(&l->src, l->pos);

    _Static_assert(TOKEN_COUNT == 4, "Exhaustive handling of token types inside lexer_next");
    
    if(*cur_tt.token == '"') {
        tok.type = TOKEN_STRING;
        tok.str_value = malloc(cur_tt.token_len - 1); // - 2 * '"', + '\0'
        if(!tok.str_value) perror("malloc");
        strncpy(tok.str_value, cur_tt.token + 1, cur_tt.token_len - 2);
        tok.str_value[cur_tt.token_len - 2] = '\0'; 
    }
    else if (is_number(&cur_tt)) {
        tok.type = TOKEN_NUMBER;
        tok.ival = str_to_int(cur_tt.token, cur_tt.token_len);
    } else {
        tok.type = TOKEN_IDENT;
        strncpy(tok.name, cur_tt.token, cur_tt.token_len);
    }

    tok.location.file = cur_tt.file;
    tok.location.row = cur_tt.row;
    tok.location.col = cur_tt.col;

    l->pos++;
    return tok;
}

token_t lexer_peek_next(lexer_t *l, bool *success) {
    if (l->pos == l->src.length) {
        *success = false;
        return (token_t){0};
    }

    token_t next = lexer_next(l);
    l->pos--;
    return next;
}
