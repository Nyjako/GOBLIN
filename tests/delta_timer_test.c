#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>

#define GOBLIN_DELTA_TIMER_IMPLEMENTATION
#include "../include/GOBLIN/delta_timer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(void) {
    goblin_DeltaTimer t;
    goblin_DeltaTimer_Start(&t);

#ifdef _WIN32
    Sleep(50);
#else
    struct timespec req = { .tv_sec = 0, .tv_nsec = 50 * 1000 * 1000 };
    nanosleep(&req, NULL);
#endif

    double s = goblin_DeltaTimer_QuerySeconds(&t);
    assert(s >= 0.0);

    puts("delta_timer tests passed");
    return 0;
}
