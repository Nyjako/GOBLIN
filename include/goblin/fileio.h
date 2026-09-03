/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    fileio.h — file I/O utilities
*/

#ifndef GOBLIN_FILEIO_H
#define GOBLIN_FILEIO_H

/* fseeko/ftello (64-bit-safe seek/tell on POSIX) are POSIX, not C99, so
 * glibc (and other libcs following the same convention) only declares
 * them when a feature-test macro asks for at least POSIX.1-2001 - and
 * only if that macro is defined before the *first* time <stdio.h> is
 * included anywhere in the translation unit, since that's when the
 * declarations are fixed by the header guard. Defining it here,
 * defensively, covers the common case of including this header before
 * other system headers. If something else in the same translation unit
 * already included <stdio.h> first without requesting POSIX (e.g. a
 * strict -std=c99 build with nothing else defined), this comes too late
 * to help - define _POSIX_C_SOURCE (or _XOPEN_SOURCE/_DEFAULT_SOURCE)
 * before that first include instead, or make sure this header is
 * included first. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int goblin_fseek64(FILE *stream, int64_t offset, int origin);
int64_t goblin_ftell64(FILE *stream);

char *goblin_read_line(FILE *fp, size_t initial_cap, size_t *out_len);

/* Cross-platform directory/path helpers. The underlying functions
 * (mkdir/access on POSIX, _mkdir/_access on Windows) and the headers
 * that declare them (<unistd.h>, <sys/stat.h> vs <direct.h>, <io.h>)
 * differ enough between POSIX and MSVC that code calling them directly
 * needs its own #ifdef _WIN32 - these wrap that difference away.
 * goblin_mkdir creates a single directory level (like mkdir(), not
 * mkdir -p) and treats an already-existing directory as success. */
bool goblin_mkdir(const char *path);
bool goblin_path_exists(const char *path);

#ifdef __cplusplus
}
namespace goblin {
    static inline int fseek64(FILE *stream, int64_t offset, int origin) { return goblin_fseek64(stream, offset, origin); }
    static inline int64_t ftell64(FILE *stream) { return goblin_ftell64(stream); }
    static inline char *read_line(FILE *fp, size_t initial_cap, size_t *out_len) { return goblin_read_line(fp, initial_cap, out_len); }
    static inline bool mkdir(const char *path) { return goblin_mkdir(path); }
    static inline bool path_exists(const char *path) { return goblin_path_exists(path); }
}
#endif

#ifdef GOBLIN_FILEIO_IMPLEMENTATION

#if defined(_WIN32)

    #include <direct.h>
    #include <errno.h>
    #include <io.h>

    int goblin_fseek64(FILE *stream, int64_t offset, int origin)
    {
        return _fseeki64(stream, offset, origin);
    }

    int64_t goblin_ftell64(FILE *stream)
    {
        return _ftelli64(stream);
    }

    bool goblin_mkdir(const char *path)
    {
        if (!path) return false;
        if (_mkdir(path) == 0) return true;
        return errno == EEXIST;
    }

    bool goblin_path_exists(const char *path)
    {
        if (!path) return false;
        return _access(path, 0) == 0;
    }

#else

    #include <errno.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>

    int goblin_fseek64(FILE *stream, int64_t offset, int origin)
    {
        return fseeko(stream, (off_t)offset, origin);
    }

    int64_t goblin_ftell64(FILE *stream)
    {
        off_t pos = ftello(stream);
        if (pos == (off_t)-1) {
            return -1;
        }
        return (int64_t)pos;
    }

    bool goblin_mkdir(const char *path)
    {
        if (!path) return false;
        if (mkdir(path, 0755) == 0) return true;
        return errno == EEXIST;
    }

    bool goblin_path_exists(const char *path)
    {
        if (!path) return false;
        return access(path, F_OK) == 0;
    }

#endif

char *goblin_read_line(FILE *fp, size_t initial_cap, size_t *out_len)
{
    if (!fp || initial_cap == 0) {
        return NULL;
    }

    size_t cap = initial_cap;
    size_t len = 0;
    char *buf = malloc(cap);

    if (!buf) {
        return NULL;
    }

    for (;;) {
        if (fgets(buf + len, (int)(cap - len), fp) == NULL) {
            if (len == 0) {
                free(buf);
                return NULL;
            }
            break;
        }

        len += strlen(buf + len);

        if (len > 0 && buf[len - 1] == '\n') {
            break;
        }

        size_t new_cap = cap * 2;
        if (new_cap <= cap) {
            free(buf);
            return NULL;
        }

        char *tmp = realloc(buf, new_cap);
        if (!tmp) {
            free(buf);
            return NULL;
        }

        buf = tmp;
        cap = new_cap;
    }

    if (out_len) {
        *out_len = len;
    }

    return buf;
}

#endif // GOBLIN_FILEIO_IMPLEMENTATION
#endif // GOBLIN_FILEIO_H
