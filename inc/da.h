#ifndef DA_H
#define DA_H

#include <stdlib.h>

typedef struct da da_t;

typedef int (*compare_index_cb)(const da_t *arr, size_t i, void *data);

struct da {
    void *data;
    size_t length;
    size_t cap;
    size_t stride;
    bool is_static;
    compare_index_cb compare_cb;
};

void da_init(da_t *arr, size_t init_cap, size_t stride);

static inline da_t _da_new(size_t stride) {
    da_t __new_array = {0};
    da_init(&__new_array, 10, stride);
    return __new_array;
}

#define da_new(T) _da_new(sizeof(T));

#define da_new_static(name, type, cmp_cb, ...)           \
    static type name##_backing[] = {__VA_ARGS__};        \
    static const da_t name = {                           \
        .data = name##_backing,                          \
        .length = sizeof(name##_backing) / sizeof(type), \
        .cap = sizeof(name##_backing) / sizeof(type),    \
        .stride = sizeof(type),                          \
        .compare_cb = cmp_cb,                            \
    }

void *da_push(da_t *arr, const void *data);

void *da_pop(da_t *arr);

void *da_get(const da_t *arr, size_t i);

void da_remove(da_t *arr, size_t i);

void *da_insert(da_t *arr, size_t i, const void *data);

void da_free(da_t *arr);

bool da_has(const da_t *arr, void *data);

size_t da_idx_of(const da_t *arr, void *data);

#endif
