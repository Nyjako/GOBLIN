/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    secure_compare.h — timing-safe equality for secrets (tokens, API keys,
    HMAC digests, ...). A plain strcmp/memcmp bails out on the first
    mismatching byte, which leaks how many leading bytes matched to anyone
    who can measure response time.
*/

#ifndef GOBLIN_SECURE_COMPARE_H
#define GOBLIN_SECURE_COMPARE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Always walks the full length (length itself is not treated as secret --
    comparing it up front is the standard tradeoff this kind of helper
    makes). Returns false immediately, without comparing, when the lengths
    differ.
*/
bool goblin_secure_memcmp(const void *a, const void *b, size_t len_a, size_t len_b);
bool goblin_secure_streq(const char *a, const char *b);

#ifdef __cplusplus
}
namespace goblin {
    static inline bool secure_memcmp(const void *a, const void *b, size_t len_a, size_t len_b) { return goblin_secure_memcmp(a, b, len_a, len_b); }
    static inline bool secure_streq(const char *a, const char *b) { return goblin_secure_streq(a, b); }
}
#endif

#ifdef GOBLIN_SECURE_COMPARE_IMPLEMENTATION

#include <string.h>

bool goblin_secure_memcmp(const void *a, const void *b, size_t len_a, size_t len_b)
{
    if (!a || !b) return false;
    if (len_a != len_b) return false;

    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    unsigned char diff = 0;
    for (size_t i = 0; i < len_a; i++) {
        diff |= pa[i] ^ pb[i];
    }
    return diff == 0;
}

bool goblin_secure_streq(const char *a, const char *b)
{
    if (!a || !b) return false;
    return goblin_secure_memcmp(a, b, strlen(a), strlen(b));
}

#endif /* GOBLIN_SECURE_COMPARE_IMPLEMENTATION */

#endif /* GOBLIN_SECURE_COMPARE_H */
