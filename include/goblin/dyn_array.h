/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    dyn_array.h — macro-based dynamic array helpers.
*/

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
}                                                                                   \
                                                                                    \
/* Removes the element at `index` in O(1) by moving the last element                \
 * into its place, rather than shifting everything after it down */                 \
static inline void NAME##_array_remove_swap(NAME##_array *a, size_t index) {        \
    if (index >= a->count) return;                                                  \
    a->items[index] = a->items[a->count - 1];                                       \
    a->count--;                                                                     \
}                                                                                   \
                                                                                    \
/* Resets count to 0 without freeing the backing allocation, so the                 \
 * same array can be refilled  */                                                   \
static inline void NAME##_array_clear(NAME##_array *a) {                            \
    a->count = 0;                                                                   \
}


#endif // GOBLIN_DYN_ARRAY_H
