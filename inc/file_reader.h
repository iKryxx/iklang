#ifndef FILE_READER_H
#define FILE_READER_H

#include "da.h"

typedef struct {
    const char *file_name;
    unsigned long long row;
    unsigned long long column;
    const char *token;
    int token_len;
} text_token_t;

void read_entire_file(const char *path, char **out);

void tokenize_file(const char *text, const char *file_name, da_t *out);

void print_tokenized_file(const da_t *tokens);
#endif
