#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>

#define GOBLIN_HIGH_RES_TIMER_IMPLEMENTATION
#include "../include/goblin/high_res_timer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(void) {
    goblin_timer t;
    goblin_timer_start(&t);

#ifdef _WIN32
    Sleep(50);
#else
    struct timespec req = { .tv_sec = 0, .tv_nsec = 50 * 1000 * 1000 };
    nanosleep(&req, NULL);
#endif

    double s = goblin_timer_elapsed_seconds(&t);
    assert(s >= 0.0);

    puts("delta_timer tests passed");
    return 0;
}
