/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    delta_timer.h — simple cross-platform high definition elapsed-time measurement.
*/

#ifndef GOBLIN_DELTA_TIMER_H
#define GOBLIN_DELTA_TIMER_H

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
} goblin_DeltaTimer;


void goblin_DeltaTimer_Start(goblin_DeltaTimer *t);
double goblin_DeltaTimer_QuerySeconds(const goblin_DeltaTimer *t);

#ifdef __cplusplus
}
namespace goblin {
    static inline void DeltaTimer_Start(goblin_DeltaTimer *t) {
        goblin_DeltaTimer_Start(t);
    }
    static inline double DeltaTimer_QuerySeconds(const goblin_DeltaTimer *t) {
        return goblin_DeltaTimer_QuerySeconds(t);
    }
}
#endif


#ifdef GOBLIN_DELTA_TIMER_IMPLEMENTATION

void goblin_DeltaTimer_Start(goblin_DeltaTimer *t) {
#ifdef _WIN32
    QueryPerformanceFrequency(&t->freq);
    QueryPerformanceCounter(&t->start);
#else
    clock_gettime(CLOCK_MONOTONIC, &t->start);
#endif
}

double goblin_DeltaTimer_QuerySeconds(const goblin_DeltaTimer *t) {
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

#endif // GOBLIN_DELTA_TIMER_IMPLEMENTATION

#endif // GOBLIN_DELTA_TIMER_H