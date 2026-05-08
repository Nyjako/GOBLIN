/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    fileio.h — file I/O utilities
*/

#ifndef GOBLIN_FILEIO_H
#define GOBLIN_FILEIO_H

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

#ifdef __cplusplus
}
namespace goblin {
    static inline int fseek64(FILE *stream, int64_t offset, int origin) { return goblin_fseek64(stream, offset, origin); }
    static inline int64_t ftell64(FILE *stream) { return goblin_ftell64(stream); }
    static inline char *read_line(FILE *fp, size_t initial_cap, size_t *out_len) { return goblin_read_line(fp, initial_cap, out_len); }
}
#endif

#ifdef GOBLIN_FILEIO_IMPLEMENTATION

#if defined(_WIN32)

    #include <io.h>

    int goblin_fseek64(FILE *stream, int64_t offset, int origin)
    {
        return _fseeki64(stream, offset, origin);
    }

    int64_t goblin_ftell64(FILE *stream)
    {
        return _ftelli64(stream);
    }

#else

    #include <sys/types.h>

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
