/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    url.h — RFC 3986 percent-encoding for a single query/path component.
*/

#ifndef GOBLIN_URL_H
#define GOBLIN_URL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Unreserved characters (A-Z a-z 0-9 - _ . ~) pass through, everything
    else becomes %XX. Returns NULL on allocation failure or a NULL value.
*/
char *goblin_url_encode(const char *value);

/*
    Reverses goblin_url_encode: %XX -> byte, '+' -> ' ' (common form-encoding
    convention), everything else passes through. Returns NULL on allocation
    failure, a NULL value, or a malformed %-escape.
*/
char *goblin_url_decode(const char *value);

#ifdef __cplusplus
}
namespace goblin {
    static inline char *url_encode(const char *value) { return goblin_url_encode(value); }
    static inline char *url_decode(const char *value) { return goblin_url_decode(value); }
}
#endif

#ifdef GOBLIN_URL_IMPLEMENTATION

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int goblin_url_is_unreserved(unsigned char c)
{
    return isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';
}

char *goblin_url_encode(const char *value)
{
    if (!value) return NULL;

    size_t len = strlen(value);
    char *out = malloc(len * 3 + 1);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];
        if (goblin_url_is_unreserved(c)) {
            out[j++] = (char)c;
        } else {
            snprintf(out + j, 4, "%%%02X", c);
            j += 3;
        }
    }
    out[j] = '\0';
    return out;
}

static inline int goblin_url_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

char *goblin_url_decode(const char *value)
{
    if (!value) return NULL;

    size_t len = strlen(value);
    char *out = malloc(len + 1);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = value[i];
        if (c == '+') {
            out[j++] = ' ';
        } else if (c == '%') {
            if (i + 2 > len) { free(out); return NULL; }
            int hi = goblin_url_nibble(value[i + 1]);
            int lo = goblin_url_nibble(value[i + 2]);
            if (hi < 0 || lo < 0) { free(out); return NULL; }
            out[j++] = (char)((hi << 4) | lo);
            i += 2;
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
    return out;
}

#endif /* GOBLIN_URL_IMPLEMENTATION */

#endif /* GOBLIN_URL_H */
