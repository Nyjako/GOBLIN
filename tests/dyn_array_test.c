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

    intvec_array_free(&a);

    puts("dyn_array tests passed");
    return 0;
}