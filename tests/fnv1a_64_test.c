#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define GOBLIN_FNV1A_BITS 64
#define GOBLIN_FNV1A_IMPLEMENTATION
#include "../include/goblin/fnv1a.h"

int main(void)
{
    /* FNV-1a 64-bit test vectors */
    assert(goblin_fnv1a_cstr("") == UINT64_C(14695981039346656037));
    assert(goblin_fnv1a_cstr("a") == UINT64_C(12638187200555641996));
    assert(goblin_fnv1a_cstr("b") == UINT64_C(12638190499090526629));
    assert(goblin_fnv1a_cstr("c") == UINT64_C(12638189399578898418));
    assert(goblin_fnv1a_cstr("d") == UINT64_C(12638183902020757363));
    assert(goblin_fnv1a_cstr("e") == UINT64_C(12638182802509129152));
    assert(goblin_fnv1a_cstr("f") == UINT64_C(12638186101044013785));
    assert(goblin_fnv1a_cstr("foobar") == UINT64_C(9625390261332436968));
    assert(goblin_fnv1a_cstr("hello") == UINT64_C(11831194018420276491));
    assert(goblin_fnv1a_cstr("hello world") == UINT64_C(8618312879776256743));
    assert(goblin_fnv1a_cstr("The quick brown fox jumps over the lazy dog") == UINT64_C(17580284887202820368));

    puts("fnv1a 64-bit tests passed");
    return 0;
}
