#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <stdio.h>
#include <stdlib.h>

#define DIAG_PRINT(prefix, ...)              \
do {                                         \
    fprintf(stderr, prefix " %s:%d (%s): ",  \
            __FILE__, __LINE__, __func__);   \
    fprintf(stderr, __VA_ARGS__);            \
    fprintf(stderr, "\n"); \
} while (0)

#define UNIMPLEMENTED(...)                      \
do {                                            \
    DIAG_PRINT("UNIMPLEMENTED:", __VA_ARGS__);  \
    abort();                                    \
} while (0)

#define TODO(...)                      \
do {                                   \
    DIAG_PRINT("TODO:", __VA_ARGS__);  \
    abort();                           \
} while (0)

#define UNREACHABLE(...)                      \
do {                                          \
    DIAG_PRINT("UNREACHABLE:", __VA_ARGS__);  \
    abort();                                  \
} while (0)

#define ASSERT_NOT_REACHED()                             \
do {                                                     \
    fprintf(stderr, "ASSERT_NOT_REACHED: %s:%d (%s)\n",  \
            __FILE__, __LINE__, __func__);               \
    abort();                                             \
} while (0)

#endif // DIAGNOSTIC_H