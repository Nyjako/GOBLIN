/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    secret_prompt.h — read a password/secret from an interactive terminal
    with input echo disabled, cross-platform.
*/

#ifndef GOBLIN_SECRET_PROMPT_H
#define GOBLIN_SECRET_PROMPT_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* True when stdin is an interactive terminal. Check this first -- on a
    piped/non-interactive stdin, prompting would just read garbage. */
bool goblin_stdin_is_tty(void);

/*
    Prints `prompt` to stderr and reads one line from stdin with echo
    disabled, writing it into `out_buf` NUL-terminated and with the
    trailing newline stripped (truncated to fit). Restores normal echo
    before returning, even on error or EOF. NULL out_buf or zero
    out_buf_size is a no-op.
*/
void goblin_prompt_secret(const char *prompt, char *out_buf, size_t out_buf_size);

#ifdef __cplusplus
}
namespace goblin {
    static inline bool stdin_is_tty(void) { return goblin_stdin_is_tty(); }
    static inline void prompt_secret(const char *prompt, char *out_buf, size_t out_buf_size) { goblin_prompt_secret(prompt, out_buf, out_buf_size); }
}
#endif

#ifdef GOBLIN_SECRET_PROMPT_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

bool goblin_stdin_is_tty(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

static void goblin_prompt_secret_read_line(char *out_buf, size_t out_buf_size)
{
    fflush(stderr);
    clearerr(stdin); /* clear a stale EOF/error flag left by a previous read */
    if (fgets(out_buf, (int)out_buf_size, stdin)) {
        size_t len = strlen(out_buf);
        if (len > 0 && out_buf[len - 1] == '\n') {
            out_buf[len - 1] = '\0';
        }
    } else {
        out_buf[0] = '\0';
    }
}

#ifndef _WIN32

void goblin_prompt_secret(const char *prompt, char *out_buf, size_t out_buf_size)
{
    if (!out_buf || out_buf_size == 0) return;

    struct termios old_termios;
    struct termios new_termios;
    bool have_termios = tcgetattr(STDIN_FILENO, &old_termios) == 0;
    if (have_termios) {
        new_termios = old_termios;
        new_termios.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    }

    fprintf(stderr, "%s", prompt ? prompt : "");
    goblin_prompt_secret_read_line(out_buf, out_buf_size);

    if (have_termios) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    }
    fprintf(stderr, "\n");
}

#else /* _WIN32 */

void goblin_prompt_secret(const char *prompt, char *out_buf, size_t out_buf_size)
{
    if (!out_buf || out_buf_size == 0) return;

    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD old_mode = 0;
    bool have_console_mode = handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &old_mode) != 0;
    if (have_console_mode) {
        SetConsoleMode(handle, old_mode & ~(DWORD)ENABLE_ECHO_INPUT);
    }

    fprintf(stderr, "%s", prompt ? prompt : "");
    goblin_prompt_secret_read_line(out_buf, out_buf_size);

    if (have_console_mode) {
        SetConsoleMode(handle, old_mode);
    }
    fprintf(stderr, "\n");
}

#endif

#endif /* GOBLIN_SECRET_PROMPT_IMPLEMENTATION */

#endif /* GOBLIN_SECRET_PROMPT_H */
