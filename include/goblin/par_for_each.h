/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    par_for_each.h — parallel helpers for iterating over arrays.
*/

#ifndef GOBLIN_PAR_FOR_EACH_H
#define GOBLIN_PAR_FOR_EACH_H

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#if __STDC_NO_THREADS__ == 1
    #error Threads are disabled "__STDC_NO_THREADS__" is set to 1
#endif

#if !defined(__STDC_VERSION__) && __STDC_VERSION__ < 201112L
    #error For now only c11 or higher is supported
#endif

#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*goblin_par_for_each_fn)(void *elem);
typedef int (*goblin_par_for_each_zip_fn)(void *elem, void *user_elem);
typedef int (*goblin_par_for_each_shared_fn)(void *elem, void *user_elem, mtx_t *mutex);

/*
    Applies fn to each element of arr in parallel using up to max_threads threads.
    Stops early if fn returns a non-zero error code.
*/
int goblin_par_for_each(void *arr, size_t count, size_t elem_size, goblin_par_for_each_fn fn, size_t max_threads);

/*
    Applies fn to pairs of elements from arr and user_arr at the same index in parallel.
    Both arrays must contain at least count elements.
    Stops early if fn returns a non-zero error code.
*/
int goblin_par_for_each_zip(void *arr, void *user_arr, size_t count, size_t elem_size, size_t user_elem_size, goblin_par_for_each_zip_fn fn, size_t max_threads);

/*
    Applies fn to each element of arr in parallel while sharing a common user_elem and mutex.
    Intended for workloads that do most work per element, then lock only for a short update.
    Stops early if fn returns a non-zero error code.
*/
int goblin_par_for_each_shared(void *arr, size_t count, size_t elem_size, void *user_elem, mtx_t *mutex, goblin_par_for_each_shared_fn fn, size_t max_threads);

#ifdef __cplusplus
}
namespace goblin {
    static inline int par_for_each(void *arr, size_t count, size_t elem_size, goblin_par_for_each_fn fn, size_t max_threads) { 
        return goblin_par_for_each(arr, count, elem_size, fn, max_threads); 
    }
    static inline int par_for_each_zip(void *arr, void *user_arr, size_t count, size_t elem_size, size_t user_elem_size, goblin_par_for_each_zip_fn fn, size_t max_threads) { 
        return goblin_par_for_each_zip(arr, user_arr, count, elem_size, user_elem_size, fn, max_threads); 
    }
    static inline int par_for_each_shared(void *arr, size_t count, size_t elem_size, void *user_elem, mtx_t *mutex, goblin_par_for_each_shared_fn fn, size_t max_threads) { 
        return goblin_par_for_each_shared(arr, count, elem_size, user_elem, mutex, fn, max_threads); 
    }
}
#endif

#ifdef GOBLIN_PAR_FOR_EACH_IMPLEMENTATION

typedef struct {
    unsigned char *base;
    size_t count;
    size_t elem_size;
    goblin_par_for_each_fn fn;
    atomic_size_t next;
    atomic_int error;
} goblin_par_for_each_ctx;

static int goblin_par_for_each_worker(void *arg)
{
    goblin_par_for_each_ctx *ctx = (goblin_par_for_each_ctx *)arg;

    for (;;) {
        if (atomic_load(&ctx->error) != 0) {
            return 0;
        }

        size_t i = atomic_fetch_add(&ctx->next, 1);
        if (i >= ctx->count) {
            break;
        }

        int rc = ctx->fn(ctx->base + i * ctx->elem_size);
        if (rc != 0) {
            atomic_store(&ctx->error, rc);
            break;
        }
    }

    return 0;
}

int goblin_par_for_each(void *arr, size_t count, size_t elem_size, goblin_par_for_each_fn fn, size_t max_threads)
{
    if (!arr || !fn || count == 0 || elem_size == 0 || max_threads == 0) {
        return 0;
    }

    if (count > SIZE_MAX / elem_size) {
        return -1; // overflow
    }

    size_t nthreads = count < max_threads ? count : max_threads;
    thrd_t *threads = (thrd_t *)malloc(nthreads * sizeof(*threads));
    if (!threads) {
        return -1;
    }

    goblin_par_for_each_ctx ctx = {
        .base = (unsigned char *)arr,
        .count = count,
        .elem_size = elem_size,
        .fn = fn,
        .next = ATOMIC_VAR_INIT(0),
        .error = ATOMIC_VAR_INIT(0),
    };

    size_t started = 0;
    for (; started < nthreads; ++started) {
        if (thrd_create(&threads[started], goblin_par_for_each_worker, &ctx) != thrd_success) {
            atomic_store(&ctx.error, -1);
            break;
        }
    }

    for (size_t i = 0; i < started; ++i) {
        int thread_rc;
        thrd_join(threads[i], &thread_rc);
    }

    free(threads);
    return atomic_load(&ctx.error);
}

typedef struct {
    unsigned char *base;
    unsigned char *user_base;
    size_t count;
    size_t elem_size;
    size_t user_elem_size;
    goblin_par_for_each_zip_fn fn;
    atomic_size_t next;
    atomic_int error;
} goblin_par_for_each_zip_ctx;

static int goblin_par_for_each_zip_worker(void *arg)
{
    goblin_par_for_each_zip_ctx *ctx = (goblin_par_for_each_zip_ctx *)arg;

    for (;;) {
        if (atomic_load(&ctx->error) != 0) {
            return 0;
        }

        size_t i = atomic_fetch_add(&ctx->next, 1);
        if (i >= ctx->count) {
            break;
        }

        void *elem = ctx->base + i * ctx->elem_size;
        void *user_elem = ctx->user_base + i * ctx->user_elem_size;

        int rc = ctx->fn(elem, user_elem);
        if (rc != 0) {
            int expected = 0;
            atomic_compare_exchange_strong(&ctx->error, &expected, rc);
            break;
        }
    }

    return 0;
}

int goblin_par_for_each_zip(void *arr, void *user_arr, size_t count, size_t elem_size, size_t user_elem_size, goblin_par_for_each_zip_fn fn, size_t max_threads)
{
    if (!arr || !user_arr || !fn || count == 0 || elem_size == 0 || user_elem_size == 0 || max_threads == 0) {
        return 0;
    }

    if (count > SIZE_MAX / elem_size || count > SIZE_MAX / user_elem_size) {
        return -1;
    }

    size_t nthreads = count < max_threads ? count : max_threads;
    thrd_t *threads = (thrd_t *)malloc(nthreads * sizeof(*threads));
    if (!threads) {
        return -1;
    }

    goblin_par_for_each_zip_ctx ctx = {
        .base = (unsigned char *)arr,
        .user_base = (unsigned char *)user_arr,
        .count = count,
        .elem_size = elem_size,
        .user_elem_size = user_elem_size,
        .fn = fn,
        .next = ATOMIC_VAR_INIT(0),
        .error = ATOMIC_VAR_INIT(0),
    };

    size_t started = 0;
    for (; started < nthreads; ++started) {
        if (thrd_create(&threads[started], goblin_par_for_each_zip_worker, &ctx) != thrd_success) {
            atomic_store(&ctx.error, -1);
            break;
        }
    }

    for (size_t i = 0; i < started; ++i) {
        int thread_rc;
        thrd_join(threads[i], &thread_rc);
    }

    free(threads);
    return atomic_load(&ctx.error);
}

typedef struct {
    unsigned char *base;
    size_t count;
    size_t elem_size;
    void *user_elem;
    mtx_t *mutex;
    goblin_par_for_each_shared_fn fn;
    atomic_size_t next;
    atomic_int error;
} goblin_par_for_each_shared_ctx;

static int goblin_par_for_each_shared_worker(void *arg)
{
    goblin_par_for_each_shared_ctx *ctx = (goblin_par_for_each_shared_ctx *)arg;

    for (;;) {
        if (atomic_load(&ctx->error) != 0) {
            return 0;
        }

        size_t i = atomic_fetch_add(&ctx->next, 1);
        if (i >= ctx->count) {
            break;
        }

        void *elem = ctx->base + i * ctx->elem_size;
        int rc = ctx->fn(elem, ctx->user_elem, ctx->mutex);

        if (rc != 0) {
            int expected = 0;
            atomic_compare_exchange_strong(&ctx->error, &expected, rc);
            break;
        }
    }

    return 0;
}

int goblin_par_for_each_shared(void *arr, size_t count, size_t elem_size, void *user_elem, mtx_t *mutex, goblin_par_for_each_shared_fn fn, size_t max_threads)
{
    if (!arr || !user_elem || !mutex || !fn || count == 0 || elem_size == 0 || max_threads == 0) {
        return 0;
    }

    if (count > SIZE_MAX / elem_size) {
        return -1;
    }

    size_t nthreads = count < max_threads ? count : max_threads;
    thrd_t *threads = (thrd_t *)malloc(nthreads * sizeof(*threads));
    if (!threads) {
        return -1;
    }

    goblin_par_for_each_shared_ctx ctx = {
        .base = (unsigned char *)arr,
        .count = count,
        .elem_size = elem_size,
        .user_elem = user_elem,
        .mutex = mutex,
        .fn = fn,
        .next = ATOMIC_VAR_INIT(0),
        .error = ATOMIC_VAR_INIT(0),
    };

    size_t started = 0;
    for (; started < nthreads; ++started) {
        if (thrd_create(&threads[started], goblin_par_for_each_shared_worker, &ctx) != thrd_success) {
            atomic_store(&ctx.error, -1);
            break;
        }
    }

    for (size_t i = 0; i < started; ++i) {
        int thread_rc;
        thrd_join(threads[i], &thread_rc);
    }

    free(threads);
    return atomic_load(&ctx.error);
}

#endif // GOBLIN_PAR_FOR_EACH_IMPLEMENTATION

#endif // GOBLIN_PAR_FOR_EACH_H