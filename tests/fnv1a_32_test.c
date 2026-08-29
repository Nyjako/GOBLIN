#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define GOBLIN_FNV1A_BITS 32
#define GOBLIN_FNV1A_IMPLEMENTATION
#include "../include/goblin/fnv1a.h"

int main(void)
{
    /* FNV-1a 32-bit test vectors */
    assert(goblin_fnv1a_cstr("") == UINT32_C(2166136261));
    assert(goblin_fnv1a_cstr("a") == UINT32_C(3826002220));
    assert(goblin_fnv1a_cstr("b") == UINT32_C(3876335077));
    assert(goblin_fnv1a_cstr("c") == UINT32_C(3859557458));
    assert(goblin_fnv1a_cstr("d") == UINT32_C(3775678463));
    assert(goblin_fnv1a_cstr("e") == UINT32_C(3758900844));
    assert(goblin_fnv1a_cstr("f") == UINT32_C(3809225553));
    assert(goblin_fnv1a_cstr("foobar") == UINT32_C(3214735720));
    assert(goblin_fnv1a_cstr("hello") == UINT32_C(1335831723));
    assert(goblin_fnv1a_cstr("hello world") == UINT32_C(3582672807));
    assert(goblin_fnv1a_cstr("The quick brown fox jumps over the lazy dog") == UINT32_C(76545936));

    puts("fnv1a 32-bit tests passed");
    return 0;
}
