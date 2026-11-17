/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    argv.h — tiny argv-scanning helpers: is a flag present, what value
    follows it, and where does it sit. Not a full argument parser (no
    subcommands, no --help, no type coercion) -- use getopt or a real
    parser for that.
*/

#ifndef GOBLIN_ARGV_H
#define GOBLIN_ARGV_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* True if `flag` appears in argv[1..argc-1] (argv[0] is never matched). */
bool goblin_argv_has_flag(int argc, char **argv, const char *flag);

/* The token right after the first occurrence of `flag`, or NULL if it's
    missing or the last token. */
const char *goblin_argv_find_value(int argc, char **argv, const char *flag);

/* Index of the first occurrence of `flag` in argv, or -1 if missing.
    Useful for a flag that takes a run of positional arguments after it. */
int goblin_argv_find_index(int argc, char **argv, const char *flag);

#ifdef __cplusplus
}
namespace goblin {
    static inline bool argv_has_flag(int argc, char **argv, const char *flag) { return goblin_argv_has_flag(argc, argv, flag); }
    static inline const char *argv_find_value(int argc, char **argv, const char *flag) { return goblin_argv_find_value(argc, argv, flag); }
    static inline int argv_find_index(int argc, char **argv, const char *flag) { return goblin_argv_find_index(argc, argv, flag); }
}
#endif

#ifdef GOBLIN_ARGV_IMPLEMENTATION

#include <string.h>

bool goblin_argv_has_flag(int argc, char **argv, const char *flag)
{
    if (!argv || !flag) return false;
    for (int i = 1; i < argc; i++) {
        if (argv[i] && strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

const char *goblin_argv_find_value(int argc, char **argv, const char *flag)
{
    if (!argv || !flag) return NULL;
    for (int i = 1; i < argc - 1; i++) {
        if (argv[i] && strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return NULL;
}

int goblin_argv_find_index(int argc, char **argv, const char *flag)
{
    if (!argv || !flag) return -1;
    for (int i = 1; i < argc; i++) {
        if (argv[i] && strcmp(argv[i], flag) == 0) return i;
    }
    return -1;
}

#endif /* GOBLIN_ARGV_IMPLEMENTATION */

#endif /* GOBLIN_ARGV_H */
