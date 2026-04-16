#ifndef GOBLIN_FNV1A_H
#define GOBLIN_FNV1A_H

#include <stdint.h>

#ifndef GOBLIN_FNV1A_BITS
    #define GOBLIN_FNV1A_BITS 32
#endif
#if GOBLIN_FNV1A_BITS == 32
    #define GOBLIN_FNV1A_TYPE uint32_t
    #define GOBLIN_FNV1A_OFFSET_BASIS 2166136261u
    #define GOBLIN_FNV1A_FNV_PRIME 2166136261u
#elif GOBLIN_FNV1A_BITS == 64
    #define GOBLIN_FNV1A_TYPE uint64_t
    #define GOBLIN_FNV1A_OFFSET_BASIS 14695981039346656037ull
    #define GOBLIN_FNV1A_FNV_PRIME 1099511628211ull
#else
    #error GOBLIN_FNV1A_BITS can only be set to 64 or 32
#endif

#ifdef __cplusplus
extern "C" {
#endif

GOBLIN_FNV1A_TYPE goblin_fnv1a_cstr(const char *s);

#ifdef __cplusplus
}
namespace goblin {
    static inline GOBLIN_FNV1A_TYPE fnv1a_cstr(const char *s) {
        return goblin_fnv1a_cstr(s);
    }
}
#endif

#ifdef GOBLIN_CSTR_IMPLEMENTATION

GOBLIN_FNV1A_TYPE goblin_fnv1a_cstr(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    GOBLIN_FNV1A_TYPE hash = GOBLIN_FNV1A_OFFSET_BASIS;

    while (*p) {
        hash ^= *p++;
        hash *= GOBLIN_FNV1A_FNV_PRIME;
    }

    return hash;
}

#endif // GOBLIN_CSTR_IMPLEMENTATION

#endif // GOBLIN_FNV1A_H