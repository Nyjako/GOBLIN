/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    cstr.h — small helpers for C strings.
*/

#ifndef GOBLIN_CSTR_H
#define GOBLIN_CSTR_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

char *goblin_copy_cstr(const char *s);
char *goblin_concat_cstr(const char *s1, const char *s2);
char *goblin_variadic_concat_cstr(size_t n, ...);
void goblin_trim_start_cstr(char *s);
void goblin_trim_end_cstr(char *s);
void goblin_trim_cstr(char *s);

#ifdef __cplusplus
}
namespace goblin {
    static inline char *copy_cstr(const char *s) { return goblin_copy_cstr(s); }
    static inline char *concat_cstr(const char *s1, const char *s2) { return goblin_concat_cstr(s1, s2); }
    static inline char *variadic_concat_cstr(size_t n, ...) {
        assert(1 && "Not sure how to do this nicely yet.");
        // TODO: Implement variadic function in c++ comp layer.
        return NULL;
    }
    static inline void trim_start_cstr(char *s) { return goblin_trim_start_cstr(s); }
    static inline void trim_end_cstr(char *s) { return goblin_trim_end_cstr(s); }
    static inline void trim_cstr(char *s) { return goblin_trim_cstr(s); }
}
#endif

#ifdef GOBLIN_CSTR_IMPLEMENTATION

static inline int goblin_is_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

char *goblin_copy_cstr(const char *s)
{
    if (!s) return NULL;

    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;

    memcpy(p, s, n);
    return p;
}

char *goblin_concat_cstr(const char *s1, const char *s2)
{
    if (!s1 || !s2) return NULL;

    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);

    if (n1 > SIZE_MAX - n2 - 1) return NULL;

    char *result = (char *)malloc(n1 + n2 + 1);
    if (!result) return NULL;

    memcpy(result, s1, n1);
    memcpy(result + n1, s2, n2 + 1);
    return result;
}

char *goblin_variadic_concat_cstr(size_t n, ...)
{
    va_list args;
    size_t total = 1;

    va_start(args, n);
    for (size_t i = 0; i < n; ++i) {
        const char *s = va_arg(args, const char *);
        if (!s) {
            va_end(args);
            return NULL;
        }

        size_t len = strlen(s);
        if (len > SIZE_MAX - total) {
            va_end(args);
            return NULL;
        }
        total += len;
    }
    va_end(args);

    char *result = (char *)malloc(total);
    if (!result) return NULL;

    char *dst = result;

    va_start(args, n);
    for (size_t i = 0; i < n; ++i) {
        const char *s = va_arg(args, const char *);
        size_t len = strlen(s);
        memcpy(dst, s, len);
        dst += len;
    }
    va_end(args);

    *dst = '\0';
    return result;
}

void goblin_trim_start_cstr(char *s)
{
    if (!s) return;

    char *src = s;

    while (*src && goblin_is_space((unsigned char)*src)) {
        ++src;
    }

    if (src != s) {
        memmove(s, src, strlen(src) + 1);
    }
}

void goblin_trim_end_cstr(char *s)
{
    if (!s) return;

    char *end = s;

    while (*end) {
        ++end;
    }

    while (end > s && goblin_is_space((unsigned char)end[-1])) {
        --end;
    }

    *end = '\0';
}

void goblin_trim_cstr(char *s)
{
    if (!s) return;

    char *start = s;
    while (*start && goblin_is_space((unsigned char)*start)) {
        ++start;
    }

    char *end = start;
    while (*end) {
        ++end;
    }

    while (end > start && goblin_is_space((unsigned char)end[-1])) {
        --end;
    }

    size_t n = (size_t)(end - start);
    if (start != s) {
        memmove(s, start, n);
    }
    s[n] = '\0';
}

#endif // GOBLIN_CSTR_IMPLEMENTATION
#endif // GOBLIN_CSTR_H