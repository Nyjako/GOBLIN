#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define GOBLIN_THREAD_IMPLEMENTATION
#include "../include/goblin/thread.h"

#define INCREMENT_THREADS 4
#define INCREMENTS_PER_THREAD 20000

typedef struct {
    goblin_mutex lock;
    long counter;
} counter_state;

static void *increment_worker(void *arg)
{
    counter_state *state = (counter_state *)arg;
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        goblin_mutex_lock(&state->lock);
        state->counter++;
        goblin_mutex_unlock(&state->lock);
    }
    return NULL;
}

static void test_mutex_serializes_concurrent_increments(void)
{
    counter_state state;
    state.counter = 0;
    assert(goblin_mutex_init(&state.lock) == 0);

    goblin_thread threads[INCREMENT_THREADS];
    for (int i = 0; i < INCREMENT_THREADS; i++) {
        assert(goblin_thread_create(&threads[i], increment_worker, &state) == 0);
    }
    for (int i = 0; i < INCREMENT_THREADS; i++) {
        assert(goblin_thread_join(threads[i]) == 0);
    }

    assert(state.counter == (long)INCREMENT_THREADS * INCREMENTS_PER_THREAD);
    goblin_mutex_destroy(&state.lock);
}

typedef struct {
    goblin_mutex lock;
    goblin_cond cond;
    bool ready;
    int value;
} handoff_state;

static void *producer(void *arg)
{
    handoff_state *state = (handoff_state *)arg;
    goblin_mutex_lock(&state->lock);
    state->value = 42;
    state->ready = true;
    goblin_cond_signal(&state->cond);
    goblin_mutex_unlock(&state->lock);
    return NULL;
}

static void test_cond_wait_blocks_until_signaled(void)
{
    handoff_state state;
    state.ready = false;
    state.value = 0;
    assert(goblin_mutex_init(&state.lock) == 0);
    assert(goblin_cond_init(&state.cond) == 0);

    goblin_thread t;
    assert(goblin_thread_create(&t, producer, &state) == 0);

    goblin_mutex_lock(&state.lock);
    while (!state.ready) {
        goblin_cond_wait(&state.cond, &state.lock);
    }
    int value = state.value;
    goblin_mutex_unlock(&state.lock);

    assert(goblin_thread_join(t) == 0);
    assert(value == 42);

    goblin_mutex_destroy(&state.lock);
    goblin_cond_destroy(&state.cond);
}

typedef struct {
    goblin_mutex lock;
    goblin_cond cond;
    bool done;
} detach_state;

static void *detach_worker(void *arg)
{
    detach_state *state = (detach_state *)arg;
    goblin_mutex_lock(&state->lock);
    state->done = true;
    goblin_cond_signal(&state->cond);
    goblin_mutex_unlock(&state->lock);
    return NULL;
}

static void test_thread_detach_runs_to_completion(void)
{
    detach_state state;
    state.done = false;
    assert(goblin_mutex_init(&state.lock) == 0);
    assert(goblin_cond_init(&state.cond) == 0);

    goblin_thread t;
    assert(goblin_thread_create(&t, detach_worker, &state) == 0);
    goblin_thread_detach(t);

    /* Synchronized via the condvar, not a sleep -- detach only means we
       never join the thread, it does not make waiting for it to finish
       nondeterministic. */
    goblin_mutex_lock(&state.lock);
    while (!state.done) {
        goblin_cond_wait(&state.cond, &state.lock);
    }
    goblin_mutex_unlock(&state.lock);

    goblin_mutex_destroy(&state.lock);
    goblin_cond_destroy(&state.cond);
}

static void test_sleep_seconds_returns(void)
{
    goblin_sleep_seconds(0);
}

int main(void)
{
    test_mutex_serializes_concurrent_increments();
    test_cond_wait_blocks_until_signaled();
    test_thread_detach_runs_to_completion();
    test_sleep_seconds_returns();

    puts("thread tests passed");
    return 0;
}
