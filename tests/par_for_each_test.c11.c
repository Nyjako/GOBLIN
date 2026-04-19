#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define GOBLIN_PAR_FOR_EACH_IMPLEMENTATION
#include "../include/GOBLIN/par_for_each.h"

/* -----------------------------
   Test data types
------------------------------*/

typedef struct {
    int x;
    float y;
} sample_t;

typedef struct {
    int value;
} zip_left_t;

typedef struct {
    int add;
} zip_right_t;

typedef struct {
    int addend;
    int updates;
} shared_state_t;

/* -----------------------------
   goblin_par_for_each tests
------------------------------*/

static int increment_int_cb(void *elem)
{
    int *x = (int *)elem;
    *x += 10;
    return 0;
}

static void test_par_for_each_basic_ints(void)
{
    int arr[32];
    for (int i = 0; i < 32; ++i) {
        arr[i] = i;
    }

    int rc = goblin_par_for_each(arr, 32, sizeof(arr[0]), increment_int_cb, 8);
    assert(rc == 0);

    for (int i = 0; i < 32; ++i) {
        assert(arr[i] == i + 10);
    }
}

static int increment_sample_cb(void *elem)
{
    sample_t *s = (sample_t *)elem;
    s->x += 1;
    s->y += 1.0f;
    return 0;
}

static void test_par_for_each_basic_structs(void)
{
    sample_t arr[16];
    for (int i = 0; i < 16; ++i) {
        arr[i].x = i;
        arr[i].y = (float)i * 0.5f;
    }

    int rc = goblin_par_for_each(arr, 16, sizeof(arr[0]), increment_sample_cb, 4);
    assert(rc == 0);

    for (int i = 0; i < 16; ++i) {
        assert(arr[i].x == i + 1);
        assert(arr[i].y == (float)i * 0.5f + 1.0f);
    }
}

static atomic_int g_stop_first_seen = 1;
static atomic_int g_stop_first_calls = 0;

static int stop_on_first_cb(void *elem)
{
    (void)elem;
    atomic_fetch_add(&g_stop_first_calls, 1);

    int expected = 1;
    if (atomic_compare_exchange_strong(&g_stop_first_seen, &expected, 0)) {
        return 7;
    }

    return 0;
}

static void test_par_for_each_early_stop(void)
{
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }

    atomic_store(&g_stop_first_seen, 1);
    atomic_store(&g_stop_first_calls, 0);

    int rc = goblin_par_for_each(arr, 128, sizeof(arr[0]), stop_on_first_cb, 8);

    assert(rc == 7);

    int calls = atomic_load(&g_stop_first_calls);
    assert(calls >= 1);
    assert(calls <= 128);
}

/* -----------------------------
   goblin_par_for_each_zip tests
------------------------------*/

static int zip_add_cb(void *elem, void *user_elem)
{
    zip_left_t *lhs = (zip_left_t *)elem;
    zip_right_t *rhs = (zip_right_t *)user_elem;

    lhs->value += rhs->add;
    return 0;
}

static void test_par_for_each_zip_basic(void)
{
    zip_left_t lhs[24];
    zip_right_t rhs[24];

    for (int i = 0; i < 24; ++i) {
        lhs[i].value = i;
        rhs[i].add = i * 2;
    }

    int rc = goblin_par_for_each_zip(lhs, rhs, 24, sizeof(lhs[0]), sizeof(rhs[0]), zip_add_cb, 6);
    assert(rc == 0);

    for (int i = 0; i < 24; ++i) {
        assert(lhs[i].value == i + (i * 2));
        assert(rhs[i].add == i * 2);
    }
}

static atomic_int g_zip_stop_seen = 1;
static atomic_int g_zip_stop_calls = 0;

static int zip_stop_on_first_cb(void *elem, void *user_elem)
{
    (void)elem;
    (void)user_elem;

    atomic_fetch_add(&g_zip_stop_calls, 1);

    int expected = 1;
    if (atomic_compare_exchange_strong(&g_zip_stop_seen, &expected, 0)) {
        return 11;
    }

    return 0;
}

static void test_par_for_each_zip_early_stop(void)
{
    int lhs[64];
    int rhs[64];

    for (int i = 0; i < 64; ++i) {
        lhs[i] = i;
        rhs[i] = i * 3;
    }

    atomic_store(&g_zip_stop_seen, 1);
    atomic_store(&g_zip_stop_calls, 0);

    int rc = goblin_par_for_each_zip(lhs, rhs, 64, sizeof(lhs[0]), sizeof(rhs[0]), zip_stop_on_first_cb, 8);

    assert(rc == 11);

    int calls = atomic_load(&g_zip_stop_calls);
    assert(calls >= 1);
    assert(calls <= 64);
}

/* -----------------------------
   goblin_par_for_each_shared tests
------------------------------*/

static int shared_update_cb(void *elem, void *user_elem, mtx_t *mutex)
{
    int *x = (int *)elem;
    shared_state_t *state = (shared_state_t *)user_elem;

    /*
        "Heavy" work would normally happen outside the lock.
        Here we keep the critical section small and only protect the update.
    */
    int add = state->addend;

    assert(mtx_lock(mutex) == thrd_success);
    *x += add;
    state->updates += 1;
    assert(mtx_unlock(mutex) == thrd_success);

    return 0;
}

static void test_par_for_each_shared_basic(void)
{
    int arr[48];
    for (int i = 0; i < 48; ++i) {
        arr[i] = i;
    }

    shared_state_t state = {
        .addend = 5,
        .updates = 0
    };

    mtx_t mutex;
    assert(mtx_init(&mutex, mtx_plain) == thrd_success);

    int rc = goblin_par_for_each_shared(arr, 48, sizeof(arr[0]), &state, &mutex, shared_update_cb, 8);
    assert(rc == 0);

    for (int i = 0; i < 48; ++i) {
        assert(arr[i] == i + 5);
    }

    assert(state.updates == 48);

    mtx_destroy(&mutex);
}

int main(void)
{
    test_par_for_each_basic_ints();
    test_par_for_each_basic_structs();
    test_par_for_each_early_stop();

    test_par_for_each_zip_basic();
    test_par_for_each_zip_early_stop();

    test_par_for_each_shared_basic();

    puts("par_for_each tests passed");
    return 0;
}