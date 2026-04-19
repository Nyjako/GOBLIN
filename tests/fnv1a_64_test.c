#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define GOBLIN_FNV1A_BITS 64
#define GOBLIN_FNV1A_IMPLEMENTATION
#include "../include/goblin/fnv1a.h"

int main(void) {
    assert(goblin_fnv1a_cstr("") == 14695981039346656037ull);
    assert(goblin_fnv1a_cstr("a") == 12638187200555641996ull);
    assert(goblin_fnv1a_cstr("hello") == 11831194018420276491ull);

    puts("fnv1a tests passed");
    return 0;
}