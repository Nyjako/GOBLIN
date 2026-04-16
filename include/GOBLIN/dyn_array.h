#ifndef GOBLIN_DYN_ARRAY_H
#define GOBLIN_DYN_ARRAY_H

#include <stddef.h>
#include <stdlib.h>

#define GOBLIN_DYN_ARRAY_DECL(NAME, TYPE)                                           \
typedef struct NAME##_array {                                                       \
    TYPE *items;                                                                    \
    size_t count;                                                                   \
    size_t capacity;                                                                \
} NAME##_array;                                                                     \
                                                                                    \
static inline int NAME##_array_reserve(NAME##_array *a, size_t new_capacity) {      \
    if (new_capacity <= a->capacity) return 1;                                      \
    if (new_capacity > SIZE_MAX / sizeof(*a->items)) return 0;                      \
    TYPE *new_items = (TYPE *)realloc(a->items, new_capacity * sizeof(*new_items)); \
    if (!new_items) return 0;                                                       \
    a->items = new_items;                                                           \
    a->capacity = new_capacity;                                                     \
    return 1;                                                                       \
}                                                                                   \
                                                                                    \
static inline void NAME##_array_init(NAME##_array *a) {                             \
    a->items = NULL;                                                                \
    a->count = 0;                                                                   \
    a->capacity = 0;                                                                \
}                                                                                   \
                                                                                    \
static inline void NAME##_array_free(NAME##_array *a) {                             \
    free(a->items);                                                                 \
    a->items = NULL;                                                                \
    a->count = 0;                                                                   \
    a->capacity = 0;                                                                \
}                                                                                   \
                                                                                    \
static inline int NAME##_array_push(NAME##_array *a, TYPE value) {                  \
    if (a->count == a->capacity) {                                                  \
        size_t new_capacity = a->capacity ? a->capacity * 2 : 8;                    \
        if (!NAME##_array_reserve(a, new_capacity)) return 0;                       \
    }                                                                               \
    a->items[a->count++] = value;                                                   \
    return 1;                                                                       \
}

// typedef struct {
//     int *items;
//     size_t count;
//     size_t capacity;
// } int_array;

// static int int_array_reserve(int_array *a, size_t new_capacity) {
//     if (new_capacity <= a->capacity) return 1;

//     int *new_items = realloc(a->items, new_capacity * sizeof(*new_items));
//     if (!new_items) return 0;

//     a->items = new_items;
//     a->capacity = new_capacity;
//     return 1;
// }

// void int_array_init(int_array *a) {
//     a->items = NULL;
//     a->count = 0;
//     a->capacity = 0;
// }

// void int_array_free(int_array *a) {
//     free(a->items);
//     a->items = NULL;
//     a->count = 0;
//     a->capacity = 0;
// }

// int int_array_push(int_array *a, int value) {
//     if (a->count == a->capacity) {
//         size_t new_capacity = a->capacity ? a->capacity * 2 : 8;
//         if (!int_array_reserve(a, new_capacity)) {
//             return 0;
//         }
//     }

//     a->items[a->count++] = value;
//     return 1;
// }


#endif // GOBLIN_DYN_ARRAY_H