#ifndef DA_H
#define DA_H

#include <stdlib.h>

typedef struct {
    void *data;
    size_t length;
    size_t cap;
    size_t stride;
} da_t;

void da_init(da_t *arr, size_t init_cap, size_t stride);

static inline da_t _da_new(size_t stride) {
    da_t __new_array = {0};
    da_init(&__new_array, 10, stride);
    return __new_array;
}

#define da_new(T) _da_new(sizeof(T));

void da_push(da_t *arr, void *data);

void *da_pop(da_t *arr);

void *da_get(const da_t *arr, size_t i);

void da_free(da_t *arr);

#endif
