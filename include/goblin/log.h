/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    log.h — leveled, colored stderr logging for a CLI tool. stdout stays
    free for a program's own data (e.g. JSON a user might pipe into `jq`).
*/

#ifndef GOBLIN_LOG_H
#define GOBLIN_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    GOBLIN_LOG_DEBUG,
    GOBLIN_LOG_INFO,
    GOBLIN_LOG_SUCCESS,
    GOBLIN_LOG_WARN,
    GOBLIN_LOG_ERROR,
} goblin_log_level;

/* Call once at startup, before any goblin_log call. Skips color when
    stderr isn't a terminal, and enables ANSI escapes on Windows consoles. */
void goblin_log_init(void);

/* printf-style; always ends the line with exactly one trailing '\n'. */
void goblin_log(goblin_log_level level, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#ifdef __cplusplus
}
namespace goblin {
    static inline void log_init(void) { goblin_log_init(); }
    template <typename... Args>
    static inline void log(goblin_log_level level, const char *fmt, Args &&...args) { goblin_log(level, fmt, args...); }
}
#endif

#ifdef GOBLIN_LOG_IMPLEMENTATION

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

static bool goblin_log_color_enabled = false;

static bool goblin_log_stderr_is_tty(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stderr)) != 0;
#else
    return isatty(STDERR_FILENO) != 0;
#endif
}

void goblin_log_init(void)
{
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode)) {
        SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    goblin_log_color_enabled = goblin_log_stderr_is_tty();
}

static const char *goblin_log_level_tag(goblin_log_level level)
{
    switch (level) {
        case GOBLIN_LOG_DEBUG:
            return "DEBUG";
        case GOBLIN_LOG_INFO:
            return "INFO";
        case GOBLIN_LOG_SUCCESS:
            return "OK";
        case GOBLIN_LOG_WARN:
            return "WARN";
        case GOBLIN_LOG_ERROR:
            return "ERROR";
    }
    return "?";
}

static const char *goblin_log_level_color(goblin_log_level level)
{
    switch (level) {
        case GOBLIN_LOG_DEBUG:
            return "\x1b[2m"; /* dim */
        case GOBLIN_LOG_INFO:
            return "\x1b[36m"; /* cyan */
        case GOBLIN_LOG_SUCCESS:
            return "\x1b[32m"; /* green */
        case GOBLIN_LOG_WARN:
            return "\x1b[33m"; /* yellow */
        case GOBLIN_LOG_ERROR:
            return "\x1b[31m"; /* red */
    }
    return "";
}

void goblin_log(goblin_log_level level, const char *fmt, ...)
{
    if (goblin_log_color_enabled) {
        fprintf(stderr, "%s[%s]\x1b[0m ", goblin_log_level_color(level), goblin_log_level_tag(level));
    } else {
        fprintf(stderr, "[%s] ", goblin_log_level_tag(level));
    }

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }

    fprintf(stderr, "\n");
}

#endif /* GOBLIN_LOG_IMPLEMENTATION */

#endif /* GOBLIN_LOG_H */
