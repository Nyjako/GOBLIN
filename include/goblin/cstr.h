/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    cstr.h — small helpers for C strings.
*/

#ifndef GOBLIN_CSTR_H
#define GOBLIN_CSTR_H

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef _WIN32
    #ifndef _SSIZE_T_DEFINED
    #ifdef  _WIN64
    typedef unsigned __int64    ssize_t;
    #else
    typedef _W64 unsigned int   ssize_t;
    #endif
    #define _SSIZE_T_DEFINED
    #endif
#else
    #include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

char *goblin_copy_cstr(const char *s);

char *goblin_concat_cstr(const char *s1, const char *s2);
char *goblin_variadic_concat_cstr(size_t n, ...);

void goblin_trim_start_cstr(char *s);
void goblin_trim_end_cstr(char *s);
void goblin_trim_cstr(char *s);

/*
    Returns index of start of substring or -1 if not found.
*/
ssize_t goblin_contains_cstr(const char *s, const char *needle);
bool goblin_starts_with_cstr(const char *s, const char *prefix);
bool goblin_ends_with_cstr(const char *s, const char *suffix);

char *goblin_slice_cstr(const char *s, size_t start, size_t end);
char *goblin_join_cstr(const char **str_arr, const char *separator);

#ifdef __cplusplus
}
namespace goblin {
    static inline char *copy_cstr(const char *s) { return goblin_copy_cstr(s); }
    static inline char *concat_cstr(const char *s1, const char *s2) { return goblin_concat_cstr(s1, s2); }

    // Not a huge fan of this but feels better than additional functions
    template <typename... Args>
    inline char *variadic_concat_cstr(Args... args)
    {
        return goblin_variadic_concat_cstr(sizeof...(Args), args...);
    }

    static inline void trim_start_cstr(char *s) { return goblin_trim_start_cstr(s); }
    static inline void trim_end_cstr(char *s) { return goblin_trim_end_cstr(s); }
    static inline void trim_cstr(char *s) { return goblin_trim_cstr(s); }

    static inline bool starts_with_cstr(const char *s, const char *prefix) { return goblin_starts_with_cstr(s, prefix); }
    static inline bool ends_with_cstr(const char *s, const char *suffix) { return goblin_ends_with_cstr(s, suffix); }
    static inline ssize_t contains_cstr(const char *s, const char *needle) { return goblin_contains_cstr(s, needle); }

    static inline ssize_t slice_cstr(const char *s, size_t start, size_t end) { return goblin_slice_cstr(s, start, end); }
}
#endif

#define GOBLIN_CSTR_IMPLEMENTATION
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

bool goblin_starts_with_cstr(const char *s, const char *prefix)
{
    if (!s || !prefix) return false;

    while (*s && *prefix) {
        if (*s != *prefix) {
            return false;
        }
        ++s;
        ++prefix;
    }

    return *prefix == '\0';
}

bool goblin_ends_with_cstr(const char *s, const char *suffix)
{
    // FIXME: I think it can be made faster and cleaner using memcmp but for now this thing will stay

    if (!s || !suffix) return false;

    const char *str_end = s;
    const char *suf_end = suffix;

    while (*str_end) { ++str_end; }
    while (*suf_end) { ++suf_end; }

    while (str_end > s && suf_end > suffix) {
        if (str_end[-1] != suf_end[-1]) {
            return false;
        }
        --str_end;
        --suf_end;
    }

    return suf_end == suffix;
}

ssize_t goblin_contains_cstr(const char *s, const char *needle)
{
    if (!s || !needle) return -1;
    if (*needle == '\0') return 0;

    for (size_t counter = 0; *s; ++s, ++counter) {
        const char *a = s;
        const char *b = needle;

        while (*a && *b && *a == *b) {
            ++a;
            ++b;
        }

        if (*b == '\0') return (ssize_t)counter;
    }

    return -1;
}

char *goblin_slice_cstr(const char *s, size_t start, size_t end)
{
    if (!s || end <= start) { return NULL; }
    if (end > strlen(s)) { return NULL; }

    size_t slice_len = end - start;

    char *result = malloc(end - start + 1);
        if (!result) {
        return NULL;
    }

    memcpy(result, s + start, slice_len);
    result[slice_len] = '\0';

    return result;
}

char *goblin_join_cstr(const char **str_arr, const char *separator)
{
    if (!str_arr || !separator) { return NULL; }

    size_t sep_len = strlen(separator);
    size_t total_len = 0;
    size_t count = 0;

    for (const char **p = str_arr; *p; ++p) {
        total_len += strlen(*p);
        ++count;
    }

    if (count == 0) {
        char *empty = malloc(1);
        if (!empty) return NULL;
        empty[0] = '\0';
        return empty;
    }

    total_len += sep_len * (count - 1);

    char *result = malloc(total_len + 1);
    if (!result) {
        return NULL;
    }

    char *dst = result;

    for (size_t i = 0; str_arr[i]; ++i) {
        size_t len = strlen(str_arr[i]);
        memcpy(dst, str_arr[i], len);
        dst += len;

        if (str_arr[i + 1]) {
            memcpy(dst, separator, sep_len);
            dst += sep_len;
        }
    }

    *dst = '\0';
    return result;
}

#endif // GOBLIN_CSTR_IMPLEMENTATION
#endif // GOBLIN_CSTR_H
