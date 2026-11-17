#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_LOG_IMPLEMENTATION
#include "../include/goblin/log.h"

#ifdef _WIN32
#include <io.h>
#define GOBLIN_CLOSE _close
#define GOBLIN_DUP _dup
#define GOBLIN_DUP2 _dup2
#define GOBLIN_STDERR_FD 2
#else
#include <unistd.h>
#define GOBLIN_CLOSE close
#define GOBLIN_DUP dup
#define GOBLIN_DUP2 dup2
#define GOBLIN_STDERR_FD STDERR_FILENO
#endif

/* Swaps the real stderr fd for a temp file for the duration of `fn`, then
 * reads back everything written to it -- goblin_log always writes to the
 * real stderr stream directly (fprintf(stderr, ...)), not a parameter, so
 * capturing its output means redirecting the fd itself, the same approach
 * secret_prompt_test.c uses for stdin. `g_color_enabled` is a file-local
 * static inside log.h's IMPLEMENTATION block, so tests can't reset it
 * directly -- instead each test re-runs goblin_log_init() right after the
 * redirect, which is exactly how a real caller establishes it too. */
static void capture_stderr(void (*fn)(void), char *out_buf, size_t out_buf_size)
{
    fflush(stderr);
    FILE *tmp = tmpfile();
    assert(tmp != NULL);
#ifdef _WIN32
    int tmp_fd = _fileno(tmp);
#else
    int tmp_fd = fileno(tmp);
#endif

    int saved_stderr = GOBLIN_DUP(GOBLIN_STDERR_FD);
    GOBLIN_DUP2(tmp_fd, GOBLIN_STDERR_FD);

    fn();

    fflush(stderr);
    GOBLIN_DUP2(saved_stderr, GOBLIN_STDERR_FD);
    GOBLIN_CLOSE(saved_stderr);

    fseek(tmp, 0, SEEK_SET);
    size_t n = fread(out_buf, 1, out_buf_size - 1, tmp);
    out_buf[n] = '\0';
    fclose(tmp);
}

static void run_info_log(void)
{
    goblin_log_init(); /* stderr is now the redirected temp file -- never a tty */
    goblin_log(GOBLIN_LOG_INFO, "hello %s, %d", "world", 42);
}

static void test_message_and_arguments_are_formatted(void)
{
    char buf[256];
    capture_stderr(run_info_log, buf, sizeof(buf));
    assert(strstr(buf, "hello world, 42") != NULL);
}

static void test_ends_with_exactly_one_trailing_newline(void)
{
    char buf[256];
    capture_stderr(run_info_log, buf, sizeof(buf));
    size_t len = strlen(buf);
    assert(len > 0 && buf[len - 1] == '\n');
    assert(len < 2 || buf[len - 2] != '\n');
}

static void test_a_non_tty_stderr_carries_no_color_codes(void)
{
    char buf[256];
    capture_stderr(run_info_log, buf, sizeof(buf));
    assert(strstr(buf, "\x1b[") == NULL);
}

typedef struct
{
    goblin_log_level level;
    const char *expected_tag;
} level_case_t;

static goblin_log_level g_case_level;

static void run_case_log(void)
{
    goblin_log_init();
    goblin_log(g_case_level, "x");
}

static void test_every_level_prints_its_own_tag(void)
{
    level_case_t cases[] = {
        {GOBLIN_LOG_DEBUG, "[DEBUG]"},
        {GOBLIN_LOG_INFO, "[INFO]"},
        {GOBLIN_LOG_SUCCESS, "[OK]"},
        {GOBLIN_LOG_WARN, "[WARN]"},
        {GOBLIN_LOG_ERROR, "[ERROR]"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        g_case_level = cases[i].level;
        char buf[256];
        capture_stderr(run_case_log, buf, sizeof(buf));
        assert(strstr(buf, cases[i].expected_tag) != NULL);
    }
}

static void run_null_format_log(void)
{
    goblin_log_init();
    goblin_log(GOBLIN_LOG_ERROR, NULL);
}

static void test_a_null_format_does_not_crash(void)
{
    char buf[256];
    capture_stderr(run_null_format_log, buf, sizeof(buf));
    assert(strstr(buf, "[ERROR]") != NULL);
}

#ifndef _WIN32
#include <pty.h>

/* The one branch none of the fd-redirection tests above can reach: a
 * genuinely interactive terminal, where goblin_log_init() should turn
 * color on and goblin_log should actually emit ANSI escapes. A pipe or a
 * plain temp file is never a tty, so this needs a real pseudo-terminal. */
static void test_a_real_tty_stderr_gets_color_codes(void)
{
    int master_fd, slave_fd;
    int ok = openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == 0;
    assert(ok);

    int saved_stderr = dup(STDERR_FILENO);
    fflush(stderr);
    dup2(slave_fd, STDERR_FILENO);
    close(slave_fd);

    goblin_log_init();
    goblin_log(GOBLIN_LOG_WARN, "careful");

    fflush(stderr);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    char buf[256];
    ssize_t n = read(master_fd, buf, sizeof(buf) - 1);
    assert(n > 0);
    buf[n] = '\0';
    close(master_fd);

    assert(strstr(buf, "\x1b[") != NULL);
    assert(strstr(buf, "careful") != NULL);
}
#endif

int main(void)
{
    test_message_and_arguments_are_formatted();
    test_ends_with_exactly_one_trailing_newline();
    test_a_non_tty_stderr_carries_no_color_codes();
    test_every_level_prints_its_own_tag();
    test_a_null_format_does_not_crash();
#ifndef _WIN32
    test_a_real_tty_stderr_gets_color_codes();
#endif

    puts("log tests passed");
    return 0;
}
