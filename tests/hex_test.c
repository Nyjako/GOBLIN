#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_HEX_IMPLEMENTATION
#include "../include/goblin/hex.h"

static void test_bytes_to_hex_known_value(void)
{
    const unsigned char data[] = {0x00, 0x01, 0xAB, 0xFF};
    char out[9];

    goblin_bytes_to_hex(data, sizeof(data), out);
    assert(strcmp(out, "0001abff") == 0);
}

static void test_bytes_to_hex_zero_length_writes_empty_string(void)
{
    char out[1];
    goblin_bytes_to_hex((const unsigned char *)"ignored", 0, out);
    assert(out[0] == '\0');
}

static void test_bytes_to_hex_null_args_do_nothing(void)
{
    char out[3] = {'x', 'y', '\0'};
    goblin_bytes_to_hex(NULL, 1, out);
    assert(strcmp(out, "xy") == 0); /* untouched */

    const unsigned char data[] = {0x01};
    goblin_bytes_to_hex(data, 1, NULL); /* must not crash */
}

static void test_hex_to_bytes_known_value(void)
{
    unsigned char out[4];
    assert(goblin_hex_to_bytes("0001abff", out) == true);
    assert(out[0] == 0x00 && out[1] == 0x01 && out[2] == 0xAB && out[3] == 0xFF);
}

static void test_hex_to_bytes_is_case_insensitive(void)
{
    unsigned char out[2];
    assert(goblin_hex_to_bytes("AbCd", out) == true);
    assert(out[0] == 0xAB && out[1] == 0xCD);
}

static void test_hex_to_bytes_odd_length_fails(void)
{
    unsigned char out[4];
    assert(goblin_hex_to_bytes("abc", out) == false);
}

static void test_hex_to_bytes_invalid_character_fails(void)
{
    unsigned char out[2];
    assert(goblin_hex_to_bytes("zz", out) == false);
}

static void test_hex_to_bytes_null_args_return_false(void)
{
    unsigned char out[1];
    assert(goblin_hex_to_bytes(NULL, out) == false);
    assert(goblin_hex_to_bytes("ab", NULL) == false);
}

static void test_round_trip(void)
{
    const unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x42};
    char hex[13];
    unsigned char back[6];

    goblin_bytes_to_hex(data, sizeof(data), hex);
    assert(goblin_hex_to_bytes(hex, back) == true);
    assert(memcmp(data, back, sizeof(data)) == 0);
}

int main(void)
{
    test_bytes_to_hex_known_value();
    test_bytes_to_hex_zero_length_writes_empty_string();
    test_bytes_to_hex_null_args_do_nothing();
    test_hex_to_bytes_known_value();
    test_hex_to_bytes_is_case_insensitive();
    test_hex_to_bytes_odd_length_fails();
    test_hex_to_bytes_invalid_character_fails();
    test_hex_to_bytes_null_args_return_false();
    test_round_trip();

    puts("hex tests passed");
    return 0;
}
