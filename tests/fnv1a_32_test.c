#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define GOBLIN_FNV1A_BITS 32
#define GOBLIN_FNV1A_IMPLEMENTATION
#include "../include/goblin/fnv1a.h"

int main(void) {
    assert(goblin_fnv1a_cstr("") == 2166136261u);
    assert(goblin_fnv1a_cstr("a") == 3382895412u);
    assert(goblin_fnv1a_cstr("hello") == 2375435031u);

    puts("fnv1a tests passed");
    return 0;
}