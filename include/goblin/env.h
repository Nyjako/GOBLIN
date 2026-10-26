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

#ifdef __cplusplus
}
namespace goblin {
    static inline bool setenv(const char *name, const char *value, int overwrite) { return goblin_setenv(name, value, overwrite); }
}
#endif

#ifdef GOBLIN_ENV_IMPLEMENTATION

#if defined(_WIN32)

#include <stdlib.h>

bool goblin_setenv(const char *name, const char *value, int overwrite)
{
    if (!name || !value) return false;
    if (!overwrite && getenv(name)) return true;
    return _putenv_s(name, value) == 0;
}

#else

#include <stdlib.h>

bool goblin_setenv(const char *name, const char *value, int overwrite)
{
    if (!name || !value) return false;
    return setenv(name, value, overwrite) == 0;
}

#endif

#endif /* GOBLIN_ENV_IMPLEMENTATION */

#endif /* GOBLIN_ENV_H */
