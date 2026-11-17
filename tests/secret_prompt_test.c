#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* for fileno() under -std=c99 -pedantic */
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_SECRET_PROMPT_IMPLEMENTATION
#include "../include/goblin/secret_prompt.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define GOBLIN_PIPE(fds) _pipe((fds), 256, _O_BINARY)
#define GOBLIN_WRITE _write
#define GOBLIN_CLOSE _close
#define GOBLIN_DUP _dup
#define GOBLIN_DUP2 _dup2
#define GOBLIN_FILENO _fileno
#define GOBLIN_STDIN_FD 0
#define GOBLIN_STDERR_FD 2
#else
#include <unistd.h>
#define GOBLIN_PIPE(fds) pipe(fds)
#define GOBLIN_WRITE write
#define GOBLIN_CLOSE close
#define GOBLIN_DUP dup
#define GOBLIN_DUP2 dup2
#define GOBLIN_FILENO fileno
#define GOBLIN_STDIN_FD STDIN_FILENO
#define GOBLIN_STDERR_FD STDERR_FILENO
#endif

/* Feeds `input` to the process's own stdin (via a pipe, swapped in for the
 * real fd 0 and back out again) and calls goblin_prompt_secret against it --
 * the same trick used to unit test any code that reads from the real stdin
 * stream rather than a parameter. The terminal-echo toggling itself becomes
 * a harmless no-op on a pipe (tcgetattr/GetConsoleMode simply fail and
 * goblin_prompt_secret's own have_termios/have_console_mode guard skips the
 * restore step), so this only exercises the read/strip/truncate logic, not
 * the actual echo suppression -- that part only matters on a real
 * interactive terminal, which a CI test can't provide anyway. */
static void prompt_with_piped_stdin(const char *input, char *out_buf, size_t out_buf_size)
{
    int fds[2];
    int ok = GOBLIN_PIPE(fds) == 0;
    assert(ok);
    if (input) {
        GOBLIN_WRITE(fds[1], input, (unsigned int)strlen(input));
    }
    GOBLIN_CLOSE(fds[1]);

    int saved_stdin = GOBLIN_DUP(GOBLIN_STDIN_FD);
    GOBLIN_DUP2(fds[0], GOBLIN_STDIN_FD);
    GOBLIN_CLOSE(fds[0]);

    fflush(stderr);
    int saved_stderr = GOBLIN_DUP(GOBLIN_STDERR_FD);
    FILE *null_stream = fopen(
#ifdef _WIN32
        "NUL",
#else
        "/dev/null",
#endif
        "w");
    if (null_stream) GOBLIN_DUP2(GOBLIN_FILENO(null_stream), GOBLIN_STDERR_FD);

    goblin_prompt_secret("secret: ", out_buf, out_buf_size);

    fflush(stderr);
    GOBLIN_DUP2(saved_stderr, GOBLIN_STDERR_FD);
    GOBLIN_CLOSE(saved_stderr);
    if (null_stream) fclose(null_stream);

    GOBLIN_DUP2(saved_stdin, GOBLIN_STDIN_FD);
    GOBLIN_CLOSE(saved_stdin);
}

static void test_strips_trailing_newline_and_nul_terminates(void)
{
    char buf[64];
    prompt_with_piped_stdin("hunter2\n", buf, sizeof(buf));
    assert(strcmp(buf, "hunter2") == 0);
}

static void test_missing_trailing_newline_is_kept_as_is(void)
{
    char buf[64];
    prompt_with_piped_stdin("hunter2", buf, sizeof(buf));
    assert(strcmp(buf, "hunter2") == 0);
}

static void test_immediate_eof_yields_an_empty_string(void)
{
    char buf[64] = {"untouched"};
    prompt_with_piped_stdin(NULL, buf, sizeof(buf));
    assert(strcmp(buf, "") == 0);
}

static void test_long_input_is_truncated_to_fit_the_buffer(void)
{
    char buf[8];
    prompt_with_piped_stdin("abcdefghijklmnop\n", buf, sizeof(buf));
    assert(strlen(buf) == sizeof(buf) - 1);
    assert(buf[sizeof(buf) - 1] == '\0');
}

static void test_stdin_is_tty_is_false_for_a_pipe(void)
{
    int fds[2];
    int ok = GOBLIN_PIPE(fds) == 0;
    assert(ok);
    GOBLIN_CLOSE(fds[1]);

    int saved_stdin = GOBLIN_DUP(GOBLIN_STDIN_FD);
    GOBLIN_DUP2(fds[0], GOBLIN_STDIN_FD);
    GOBLIN_CLOSE(fds[0]);

    assert(goblin_stdin_is_tty() == false);

    GOBLIN_DUP2(saved_stdin, GOBLIN_STDIN_FD);
    GOBLIN_CLOSE(saved_stdin);
}

static void test_null_out_buf_or_zero_size_is_a_safe_no_op(void)
{
    goblin_prompt_secret("secret: ", NULL, 64);
    char buf[8] = "intact";
    goblin_prompt_secret("secret: ", buf, 0);
    assert(strcmp(buf, "intact") == 0);
}

int main(void)
{
    test_stdin_is_tty_is_false_for_a_pipe();
    test_strips_trailing_newline_and_nul_terminates();
    test_missing_trailing_newline_is_kept_as_is();
    test_immediate_eof_yields_an_empty_string();
    test_long_input_is_truncated_to_fit_the_buffer();
    test_null_out_buf_or_zero_size_is_a_safe_no_op();

    puts("secret_prompt tests passed");
    return 0;
}
