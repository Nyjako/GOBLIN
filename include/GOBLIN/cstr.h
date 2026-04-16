#ifndef GOBLIN_CSTR_H
#define GOBLIN_CSTR_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char *goblin_copy_cstr(const char *s);

#ifdef __cplusplus
}
namespace goblin {
    static inline char *copy_cstr(const char *s) {
        return goblin_copy_cstr(s);
    }
}
#endif

#ifdef GOBLIN_CSTR_IMPLEMENTATION

char *goblin_copy_cstr(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

#endif // GOBLIN_CSTR_IMPLEMENTATION

#endif // GOBLIN_CSTR_H