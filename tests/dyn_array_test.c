#include <assert.h>
#include <stdio.h>

#include "../include/goblin/dyn_array.h"

GOBLIN_DYN_ARRAY_DECL(intvec, int)

int main(void) {
    intvec_array a;
    intvec_array_init(&a);

    assert(a.items == NULL);
    assert(a.count == 0);
    assert(a.capacity == 0);

    assert(intvec_array_push(&a, 10));
    assert(intvec_array_push(&a, 20));
    assert(intvec_array_push(&a, 30));

    assert(a.count == 3);
    assert(a.items[0] == 10);
    assert(a.items[1] == 20);
    assert(a.items[2] == 30);

    /* remove_swap: removing the middle element should move the last
     * element (30) into its place, not shift 20/30 down. */
    intvec_array_remove_swap(&a, 1);
    assert(a.count == 2);
    assert(a.items[0] == 10);
    assert(a.items[1] == 30);

    /* Out-of-bounds index is a documented no-op, not a crash. */
    intvec_array_remove_swap(&a, 99);
    assert(a.count == 2);

    /* Removing the last element just shrinks count - no swap needed
     * (there's nothing after it to move), but should still work. */
    intvec_array_remove_swap(&a, 1);
    assert(a.count == 1);
    assert(a.items[0] == 10);

    /* clear: count resets to 0, but capacity/allocation survives (a
     * push right after shouldn't need to grow if capacity already
     * covers it). */
    size_t capacity_before_clear = a.capacity;
    intvec_array_clear(&a);
    assert(a.count == 0);
    assert(a.capacity == capacity_before_clear);

    assert(intvec_array_push(&a, 42));
    assert(a.count == 1);
    assert(a.items[0] == 42);
    assert(a.capacity == capacity_before_clear); /* no reallocation needed */

    intvec_array_free(&a);

    puts("dyn_array tests passed");
    return 0;
}
