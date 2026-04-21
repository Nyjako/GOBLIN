#ifdef _WIN32

#include <stdio.h>

int main(void)
{
    puts("diagnostic test not inplemented for windows");

    return 0;
}

#else

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/goblin/diagnostic.h"

typedef void (*case_fn)(void);

static void case_unimplemented(void) {
    UNIMPLEMENTED("feature %d", 42);
}

static void case_todo(void) {
    TODO("refactor %s", "parser");
}

static void case_unreachable(void) {
    UNREACHABLE("bad state");
}

static void case_assert_not_reached(void) {
    ASSERT_NOT_REACHED();
}

static char *read_all_fd(int fd)
{
    size_t cap = 256;
    size_t len = 0;
    char *out = (char *)malloc(cap);
    if (!out) {
        perror("malloc");
        abort();
    }

    for (;;) {
        char buf[256];
        ssize_t n = read(fd, buf, sizeof buf);

        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            free(out);
            abort();
        }

        if (n == 0) break;

        if (len + (size_t)n + 1 > cap) {
            while (len + (size_t)n + 1 > cap) {
                cap *= 2;
            }
            char *tmp = (char *)realloc(out, cap);
            if (!tmp) {
                perror("realloc");
                free(out);
                abort();
            }
            out = tmp;
        }

        memcpy(out + len, buf, (size_t)n);
        len += (size_t)n;
    }

    out[len] = '\0';
    return out;
}

static char *run_case(case_fn fn, int *status_out)
{
    int pipefd[2];
    assert(pipe(pipefd) == 0);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        close(pipefd[0]);

        assert(dup2(pipefd[1], STDERR_FILENO) >= 0);
        close(pipefd[1]);

        setvbuf(stderr, NULL, _IONBF, 0);

        fn();
        _exit(123);
    }

    close(pipefd[1]);

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);

    char *output = read_all_fd(pipefd[0]);
    close(pipefd[0]);

    *status_out = status;
    return output;
}

static void assert_aborts_with(case_fn fn, const char *expected_prefix, const char *expected_func, const char *expected_msg)
{
    int status = 0;
    char *output = run_case(fn, &status);

    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGABRT);

    assert(strstr(output, expected_prefix) != NULL);
    assert(strstr(output, expected_func) != NULL);
    if (expected_msg) {
        assert(strstr(output, expected_msg) != NULL);
    }

    free(output);
}

int main(void)
{
    assert_aborts_with(case_unimplemented, "UNIMPLEMENTED:", "case_unimplemented", "feature 42");

    assert_aborts_with(case_todo, "TODO:", "case_todo", "refactor parser");

    assert_aborts_with(case_unreachable, "UNREACHABLE:",  "case_unreachable", "bad state");

    assert_aborts_with(case_assert_not_reached, "ASSERT_NOT_REACHED:", "case_assert_not_reached", NULL);

    puts("diagnostic tests passed");
    return 0;
}

#endif