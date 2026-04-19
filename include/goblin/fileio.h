/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    fileio.h — 64-bit file position helpers.
*/

#ifndef GOBLIN_FILEIO_H
#define GOBLIN_FILEIO_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int goblin_fseek64(FILE *stream, int64_t offset, int origin);
int64_t goblin_ftell64(FILE *stream);

#ifdef __cplusplus
}
namespace goblin {
    static inline int fseek64(FILE *stream, int64_t offset, int origin) {
        return goblin_fseek64(stream, offset, origin);
    }
    static inline int64_t ftell64(FILE *stream) {
        return goblin_ftell64(stream);
    }
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

#endif // GOBLIN_FILEIO_IMPLEMENTATION

#endif // GOBLIN_FILEIO_H