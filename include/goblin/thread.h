/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    thread.h — cross-platform mutex / condition-variable / thread
    primitives. Wraps pthreads on POSIX and the native Win32 primitives on
    Windows (MSVC ships no pthreads at all, and no equivalent without a
    separate shim).
*/

#ifndef GOBLIN_THREAD_H
#define GOBLIN_THREAD_H

#ifdef _WIN32
  #include <windows.h>
#else
  #include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef CRITICAL_SECTION goblin_mutex;
typedef CONDITION_VARIABLE goblin_cond;
typedef HANDLE goblin_thread;
#else
typedef pthread_mutex_t goblin_mutex;
typedef pthread_cond_t goblin_cond;
typedef pthread_t goblin_thread;
#endif

int goblin_mutex_init(goblin_mutex *mutex);
void goblin_mutex_destroy(goblin_mutex *mutex);
void goblin_mutex_lock(goblin_mutex *mutex);
void goblin_mutex_unlock(goblin_mutex *mutex);

int goblin_cond_init(goblin_cond *cond);
void goblin_cond_destroy(goblin_cond *cond);
void goblin_cond_wait(goblin_cond *cond, goblin_mutex *mutex);
void goblin_cond_signal(goblin_cond *cond);
void goblin_cond_broadcast(goblin_cond *cond);

/*
    fn/arg follow pthread_create's void *(*)(void *) signature on every
    platform, including Windows (trampolined internally to match
    CreateThread's own DWORD WINAPI(LPVOID) signature).
*/
int goblin_thread_create(goblin_thread *thread, void *(*fn)(void *), void *arg);
int goblin_thread_join(goblin_thread thread);

/*
    Releases the handle without waiting for the thread -- it keeps running
    to completion regardless; this just gives up the ability to join or
    query it again.
*/
void goblin_thread_detach(goblin_thread thread);

/*
    Whole-second sleep. POSIX's own sleep() needs <unistd.h>, which does
    not exist on MSVC.
*/
void goblin_sleep_seconds(unsigned int seconds);

#ifdef __cplusplus
}
namespace goblin {
    static inline int mutex_init(goblin_mutex *mutex) { return goblin_mutex_init(mutex); }
    static inline void mutex_destroy(goblin_mutex *mutex) { goblin_mutex_destroy(mutex); }
    static inline void mutex_lock(goblin_mutex *mutex) { goblin_mutex_lock(mutex); }
    static inline void mutex_unlock(goblin_mutex *mutex) { goblin_mutex_unlock(mutex); }

    static inline int cond_init(goblin_cond *cond) { return goblin_cond_init(cond); }
    static inline void cond_destroy(goblin_cond *cond) { goblin_cond_destroy(cond); }
    static inline void cond_wait(goblin_cond *cond, goblin_mutex *mutex) { goblin_cond_wait(cond, mutex); }
    static inline void cond_signal(goblin_cond *cond) { goblin_cond_signal(cond); }
    static inline void cond_broadcast(goblin_cond *cond) { goblin_cond_broadcast(cond); }

    static inline int thread_create(goblin_thread *thread, void *(*fn)(void *), void *arg) { return goblin_thread_create(thread, fn, arg); }
    static inline int thread_join(goblin_thread thread) { return goblin_thread_join(thread); }
    static inline void thread_detach(goblin_thread thread) { goblin_thread_detach(thread); }

    static inline void sleep_seconds(unsigned int seconds) { goblin_sleep_seconds(seconds); }
}
#endif

#ifdef GOBLIN_THREAD_IMPLEMENTATION

#ifdef _WIN32

#include <stdlib.h>

int goblin_mutex_init(goblin_mutex *mutex)
{
    InitializeCriticalSection(mutex);
    return 0;
}

void goblin_mutex_destroy(goblin_mutex *mutex)
{
    DeleteCriticalSection(mutex);
}

void goblin_mutex_lock(goblin_mutex *mutex)
{
    EnterCriticalSection(mutex);
}

void goblin_mutex_unlock(goblin_mutex *mutex)
{
    LeaveCriticalSection(mutex);
}

int goblin_cond_init(goblin_cond *cond)
{
    InitializeConditionVariable(cond);
    return 0;
}

void goblin_cond_destroy(goblin_cond *cond)
{
    (void)cond; /* nothing to release on Windows */
}

void goblin_cond_wait(goblin_cond *cond, goblin_mutex *mutex)
{
    SleepConditionVariableCS(cond, mutex, INFINITE);
}

void goblin_cond_signal(goblin_cond *cond)
{
    WakeConditionVariable(cond);
}

void goblin_cond_broadcast(goblin_cond *cond)
{
    WakeAllConditionVariable(cond);
}

typedef struct {
    void *(*fn)(void *);
    void *arg;
} goblin_thread_start;

/*
    CreateThread needs a DWORD WINAPI(LPVOID) proc, not pthread's
    void *(*)(void *) -- casting a function pointer between the two is
    undefined behavior (differing return type), so this trampolines
    through a real matching signature instead of relying on the cast
    happening to work.
*/
static DWORD WINAPI goblin_thread_trampoline(LPVOID param)
{
    goblin_thread_start *start = (goblin_thread_start *)param;
    void *(*fn)(void *) = start->fn;
    void *arg = start->arg;
    free(start);
    fn(arg);
    return 0;
}

int goblin_thread_create(goblin_thread *thread, void *(*fn)(void *), void *arg)
{
    goblin_thread_start *start = (goblin_thread_start *)malloc(sizeof(*start));
    if (!start) return -1;

    start->fn = fn;
    start->arg = arg;
    *thread = CreateThread(NULL, 0, goblin_thread_trampoline, start, 0, NULL);
    if (!*thread) {
        free(start);
        return -1;
    }
    return 0;
}

int goblin_thread_join(goblin_thread thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

void goblin_thread_detach(goblin_thread thread)
{
    CloseHandle(thread);
}

void goblin_sleep_seconds(unsigned int seconds)
{
    Sleep(seconds * 1000);
}

#else /* !_WIN32 */

#include <unistd.h>

int goblin_mutex_init(goblin_mutex *mutex)
{
    return pthread_mutex_init(mutex, NULL);
}

void goblin_mutex_destroy(goblin_mutex *mutex)
{
    pthread_mutex_destroy(mutex);
}

void goblin_mutex_lock(goblin_mutex *mutex)
{
    pthread_mutex_lock(mutex);
}

void goblin_mutex_unlock(goblin_mutex *mutex)
{
    pthread_mutex_unlock(mutex);
}

int goblin_cond_init(goblin_cond *cond)
{
    return pthread_cond_init(cond, NULL);
}

void goblin_cond_destroy(goblin_cond *cond)
{
    pthread_cond_destroy(cond);
}

void goblin_cond_wait(goblin_cond *cond, goblin_mutex *mutex)
{
    pthread_cond_wait(cond, mutex);
}

void goblin_cond_signal(goblin_cond *cond)
{
    pthread_cond_signal(cond);
}

void goblin_cond_broadcast(goblin_cond *cond)
{
    pthread_cond_broadcast(cond);
}

int goblin_thread_create(goblin_thread *thread, void *(*fn)(void *), void *arg)
{
    return pthread_create(thread, NULL, fn, arg);
}

int goblin_thread_join(goblin_thread thread)
{
    return pthread_join(thread, NULL);
}

void goblin_thread_detach(goblin_thread thread)
{
    pthread_detach(thread);
}

void goblin_sleep_seconds(unsigned int seconds)
{
    sleep(seconds);
}

#endif /* _WIN32 */

#endif /* GOBLIN_THREAD_IMPLEMENTATION */

#endif /* GOBLIN_THREAD_H */
