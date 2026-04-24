/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    high_res_timer.h — simple cross-platform high definition elapsed-time measurement.
*/

#ifndef GOBLIN_HIGH_RES_TIMER_H
#define GOBLIN_HIGH_RES_TIMER_H

#ifdef _WIN32
  #include <windows.h>
#else
  #include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#ifdef _WIN32
    LARGE_INTEGER start;
    LARGE_INTEGER freq;
#else
    struct timespec start;
#endif
} goblin_timer;


void goblin_timer_start(goblin_timer *t);
double goblin_timer_elapsed_seconds(const goblin_timer *t);

#ifdef __cplusplus
}
namespace goblin {
    static inline void timer_start(goblin_timer *t) { return goblin_timer_start(t); }
    static inline double timer_elapsed_seconds(const goblin_timer *t) { return goblin_timer_elapsed_seconds(t); }
}
#endif

#ifdef GOBLIN_HIGH_RES_TIMER_IMPLEMENTATION

void goblin_timer_start(goblin_timer *t) {
#ifdef _WIN32
    QueryPerformanceFrequency(&t->freq);
    QueryPerformanceCounter(&t->start);
#else
    clock_gettime(CLOCK_MONOTONIC, &t->start);
#endif
}

double goblin_timer_elapsed_seconds(const goblin_timer *t) {
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - t->start.QuadPart) / (double)t->freq.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long sec = now.tv_sec - t->start.tv_sec;
    long nsec = now.tv_nsec - t->start.tv_nsec;
    return (double)sec + (double)nsec / 1e9;
#endif
}

#endif // GOBLIN_HIGH_RES_TIMER_IMPLEMENTATION
#endif // GOBLIN_HIGH_RES_TIMER_H
