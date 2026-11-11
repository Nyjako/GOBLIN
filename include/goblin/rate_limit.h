/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    rate_limit.h — blocking rate limiter: each call to goblin_rate_limiter_wait
    blocks until at least min_interval_sec has passed since the previous
    call. Useful for pacing outgoing requests to something that enforces
    (or just appreciates) a minimum gap between them.

    Reads the same monotonic clock as high_res_timer.h, duplicated rather
    than included so this header stays standalone like every other one here.
*/

#ifndef GOBLIN_RATE_LIMIT_H
#define GOBLIN_RATE_LIMIT_H

#ifdef _WIN32
  #include <windows.h>
#else
  #include <time.h>
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double min_interval_sec;
    bool has_last_call;
#ifdef _WIN32
    LARGE_INTEGER last_call;
    LARGE_INTEGER freq;
#else
    struct timespec last_call;
#endif
} goblin_rate_limiter;

void goblin_rate_limiter_init(goblin_rate_limiter *limiter, double min_interval_sec);

/*
    Blocks the calling thread until min_interval_sec has elapsed since the
    previous call (a no-op on the first call, and whenever min_interval_sec
    is <= 0).
*/
void goblin_rate_limiter_wait(goblin_rate_limiter *limiter);

#ifdef __cplusplus
}
namespace goblin {
    static inline void rate_limiter_init(goblin_rate_limiter *limiter, double min_interval_sec) { goblin_rate_limiter_init(limiter, min_interval_sec); }
    static inline void rate_limiter_wait(goblin_rate_limiter *limiter) { goblin_rate_limiter_wait(limiter); }
}
#endif

#ifdef GOBLIN_RATE_LIMIT_IMPLEMENTATION

#ifdef _WIN32

static double goblin_rate_limit_elapsed_seconds(const LARGE_INTEGER *since, const LARGE_INTEGER *freq)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - since->QuadPart) / (double)freq->QuadPart;
}

void goblin_rate_limiter_init(goblin_rate_limiter *limiter, double min_interval_sec)
{
    limiter->min_interval_sec = min_interval_sec;
    limiter->has_last_call = false;
    QueryPerformanceFrequency(&limiter->freq);
}

void goblin_rate_limiter_wait(goblin_rate_limiter *limiter)
{
    if (limiter->min_interval_sec <= 0) {
        QueryPerformanceCounter(&limiter->last_call);
        limiter->has_last_call = true;
        return;
    }

    if (limiter->has_last_call) {
        double elapsed = goblin_rate_limit_elapsed_seconds(&limiter->last_call, &limiter->freq);
        double remaining = limiter->min_interval_sec - elapsed;
        if (remaining > 0) {
            Sleep((DWORD)(remaining * 1000.0));
        }
    }

    QueryPerformanceCounter(&limiter->last_call);
    limiter->has_last_call = true;
}

#else /* !_WIN32 */

#include <errno.h>

static double goblin_rate_limit_elapsed_seconds(const struct timespec *since)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double sec = (double)(now.tv_sec - since->tv_sec);
    double nsec = (double)(now.tv_nsec - since->tv_nsec);
    return sec + nsec / 1e9;
}

void goblin_rate_limiter_init(goblin_rate_limiter *limiter, double min_interval_sec)
{
    limiter->min_interval_sec = min_interval_sec;
    limiter->has_last_call = false;
}

void goblin_rate_limiter_wait(goblin_rate_limiter *limiter)
{
    if (limiter->min_interval_sec <= 0) {
        clock_gettime(CLOCK_MONOTONIC, &limiter->last_call);
        limiter->has_last_call = true;
        return;
    }

    if (limiter->has_last_call) {
        double elapsed = goblin_rate_limit_elapsed_seconds(&limiter->last_call);
        double remaining = limiter->min_interval_sec - elapsed;
        if (remaining > 0) {
            struct timespec req;
            req.tv_sec = (time_t)remaining;
            req.tv_nsec = (long)((remaining - (double)req.tv_sec) * 1e9);
            while (nanosleep(&req, &req) == -1 && errno == EINTR) {
                /* interrupted by a signal -- nanosleep already wrote the
                   remaining time back into req, so just retry it */
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &limiter->last_call);
    limiter->has_last_call = true;
}

#endif /* _WIN32 */

#endif /* GOBLIN_RATE_LIMIT_IMPLEMENTATION */

#endif /* GOBLIN_RATE_LIMIT_H */
