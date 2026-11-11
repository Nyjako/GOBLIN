/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    env.h — cross-platform environment variable helpers.
*/

#ifndef GOBLIN_ENV_H
#define GOBLIN_ENV_H

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool goblin_setenv(const char *name, const char *value, int overwrite);

/*
    Reads name, falling back to default_value if it's unset. The returned
    string is always a fresh goblin_copy_cstr-style allocation (including
    the fallback case), so the caller always owns it and always frees it
    the same way.
*/
char *goblin_getenv_or(const char *name, const char *default_value);

/*
    Parses name as a base-10 int (strtol under the hood). Falls back to
    default_value if name is unset, empty, or doesn't parse as a whole
    number (trailing garbage after the digits counts as not parsing).
*/
int goblin_getenv_int_or(const char *name, int default_value);

/*
    "1"/"true"/"yes"/"on" (case-insensitive) -> true, "0"/"false"/"no"/"off"
    -> false, anything else (including unset) -> default_value.
*/
bool goblin_getenv_bool_or(const char *name, bool default_value);

#ifdef __cplusplus
}
namespace goblin {
    static inline bool setenv(const char *name, const char *value, int overwrite) { return goblin_setenv(name, value, overwrite); }
    static inline char *getenv_or(const char *name, const char *default_value) { return goblin_getenv_or(name, default_value); }
    static inline int getenv_int_or(const char *name, int default_value) { return goblin_getenv_int_or(name, default_value); }
    static inline bool getenv_bool_or(const char *name, bool default_value) { return goblin_getenv_bool_or(name, default_value); }
}
#endif

#ifdef GOBLIN_ENV_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

bool goblin_setenv(const char *name, const char *value, int overwrite)
{
    if (!name || !value) return false;
    if (!overwrite && getenv(name)) return true;
    return _putenv_s(name, value) == 0;
}

#else

bool goblin_setenv(const char *name, const char *value, int overwrite)
{
    if (!name || !value) return false;
    return setenv(name, value, overwrite) == 0;
}

#endif

char *goblin_getenv_or(const char *name, const char *default_value)
{
    const char *value = name ? getenv(name) : NULL;
    if (!value) value = default_value;
    if (!value) return NULL;

    size_t n = strlen(value) + 1;
    char *copy = (char *)malloc(n);
    if (copy) memcpy(copy, value, n);
    return copy;
}

int goblin_getenv_int_or(const char *name, int default_value)
{
    const char *value = name ? getenv(name) : NULL;
    if (!value || *value == '\0') return default_value;

    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0') return default_value;
    return (int)parsed;
}

bool goblin_getenv_bool_or(const char *name, bool default_value)
{
    const char *value = name ? getenv(name) : NULL;
    if (!value || *value == '\0') return default_value;

    if (strcmp(value, "1") == 0) return true;
    if (strcmp(value, "0") == 0) return false;

    char lower[8];
    size_t len = strlen(value);
    if (len >= sizeof(lower)) return default_value;
    for (size_t i = 0; i < len; i++) {
        char c = value[i];
        lower[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    lower[len] = '\0';

    if (strcmp(lower, "true") == 0 || strcmp(lower, "yes") == 0 || strcmp(lower, "on") == 0) return true;
    if (strcmp(lower, "false") == 0 || strcmp(lower, "no") == 0 || strcmp(lower, "off") == 0) return false;
    return default_value;
}

#endif /* GOBLIN_ENV_IMPLEMENTATION */

#endif /* GOBLIN_ENV_H */
