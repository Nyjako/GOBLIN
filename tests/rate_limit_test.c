#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>

#define GOBLIN_RATE_LIMIT_IMPLEMENTATION
#include "../include/goblin/rate_limit.h"

#define GOBLIN_HIGH_RES_TIMER_IMPLEMENTATION
#include "../include/goblin/high_res_timer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static void test_sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec req = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000 * 1000};
    nanosleep(&req, NULL);
#endif
}

static void test_first_wait_does_not_block(void)
{
    goblin_rate_limiter limiter;
    goblin_rate_limiter_init(&limiter, 0.2);

    goblin_timer t;
    goblin_timer_start(&t);
    goblin_rate_limiter_wait(&limiter);
    double elapsed = goblin_timer_elapsed_seconds(&t);

    assert(elapsed < 0.1);
}

static void test_second_wait_blocks_for_remaining_interval(void)
{
    goblin_rate_limiter limiter;
    goblin_rate_limiter_init(&limiter, 0.15);
    goblin_rate_limiter_wait(&limiter); /* primes last-call time, does not block */

    goblin_timer t;
    goblin_timer_start(&t);
    goblin_rate_limiter_wait(&limiter);
    double elapsed = goblin_timer_elapsed_seconds(&t);

    assert(elapsed >= 0.1); /* some slack below 0.15 for timer/sleep resolution */
    assert(elapsed < 0.6);  /* generous upper bound so a loaded CI box doesn't flake */
}

static void test_wait_does_not_block_if_interval_already_elapsed(void)
{
    goblin_rate_limiter limiter;
    goblin_rate_limiter_init(&limiter, 0.05);
    goblin_rate_limiter_wait(&limiter);

    test_sleep_ms(80); /* already longer than the 50ms interval */

    goblin_timer t;
    goblin_timer_start(&t);
    goblin_rate_limiter_wait(&limiter);
    double elapsed = goblin_timer_elapsed_seconds(&t);

    assert(elapsed < 0.05);
}

static void test_non_positive_interval_never_blocks(void)
{
    goblin_rate_limiter limiter;
    goblin_rate_limiter_init(&limiter, 0.0);

    goblin_timer t;
    goblin_timer_start(&t);
    for (int i = 0; i < 5; i++) {
        goblin_rate_limiter_wait(&limiter);
    }
    double elapsed = goblin_timer_elapsed_seconds(&t);

    assert(elapsed < 0.05);
}

int main(void)
{
    test_first_wait_does_not_block();
    test_second_wait_blocks_for_remaining_interval();
    test_wait_does_not_block_if_interval_already_elapsed();
    test_non_positive_interval_never_blocks();

    puts("rate_limit tests passed");
    return 0;
}
