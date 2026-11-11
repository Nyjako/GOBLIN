#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GOBLIN_BASE64_IMPLEMENTATION
#include "../include/goblin/base64.h"

static void test_encode_known_value_no_padding(void)
{
    const char *text = "any carnal pleas";
    char *out = goblin_base64_encode((const unsigned char *)text, strlen(text));
    assert(out != NULL);
    assert(strcmp(out, "YW55IGNhcm5hbCBwbGVhcw==") == 0);
    free(out);
}

static void test_encode_padding_one_equals(void)
{
    const char *text = "any carnal pleasu";
    char *out = goblin_base64_encode((const unsigned char *)text, strlen(text));
    assert(out != NULL);
    assert(strcmp(out, "YW55IGNhcm5hbCBwbGVhc3U=") == 0);
    free(out);
}

static void test_encode_padding_two_equals(void)
{
    const char *text = "any carnal pleasur";
    char *out = goblin_base64_encode((const unsigned char *)text, strlen(text));
    assert(out != NULL);
    assert(strcmp(out, "YW55IGNhcm5hbCBwbGVhc3Vy") == 0);
    free(out);
}

static void test_encode_empty_input(void)
{
    char *out = goblin_base64_encode((const unsigned char *)"", 0);
    assert(out != NULL);
    assert(strcmp(out, "") == 0);
    free(out);
}

static void test_decode_known_value(void)
{
    unsigned char out[32];
    size_t out_len = 0;

    assert(goblin_base64_decode("YW55IGNhcm5hbCBwbGVhcw==", out, &out_len) == true);
    assert(out_len == 16);
    assert(memcmp(out, "any carnal pleas", 16) == 0);
}

static void test_decode_with_one_equals_padding(void)
{
    unsigned char out[32];
    size_t out_len = 0;

    assert(goblin_base64_decode("YW55IGNhcm5hbCBwbGVhc3U=", out, &out_len) == true);
    assert(out_len == 17);
    assert(memcmp(out, "any carnal pleasu", 17) == 0);
}

static void test_decode_rejects_length_not_a_multiple_of_four(void)
{
    unsigned char out[8];
    size_t out_len = 0;
    assert(goblin_base64_decode("abc", out, &out_len) == false);
}

static void test_decode_rejects_padding_in_a_non_final_group(void)
{
    unsigned char out[32];
    size_t out_len = 0;
    assert(goblin_base64_decode("YW==YW55", out, &out_len) == false);
}

static void test_decode_rejects_invalid_character(void)
{
    unsigned char out[8];
    size_t out_len = 0;
    assert(goblin_base64_decode("!!!!", out, &out_len) == false);
}

static void test_decode_null_args_return_false(void)
{
    unsigned char out[8];
    size_t out_len = 0;
    assert(goblin_base64_decode(NULL, out, &out_len) == false);
    assert(goblin_base64_decode("YWJj", NULL, &out_len) == false);
    assert(goblin_base64_decode("YWJj", out, NULL) == false);
}

static void test_round_trip_every_padding_case(void)
{
    const char *cases[] = {"", "a", "ab", "abc", "abcd", "hello world!"};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t len = strlen(cases[i]);
        char *encoded = goblin_base64_encode((const unsigned char *)cases[i], len);
        assert(encoded != NULL);

        unsigned char decoded[64];
        size_t decoded_len = 0;
        if (len == 0) {
            assert(strcmp(encoded, "") == 0);
        } else {
            assert(goblin_base64_decode(encoded, decoded, &decoded_len) == true);
            assert(decoded_len == len);
            assert(memcmp(decoded, cases[i], len) == 0);
        }
        free(encoded);
    }
}

int main(void)
{
    test_encode_known_value_no_padding();
    test_encode_padding_one_equals();
    test_encode_padding_two_equals();
    test_encode_empty_input();
    test_decode_known_value();
    test_decode_with_one_equals_padding();
    test_decode_rejects_length_not_a_multiple_of_four();
    test_decode_rejects_padding_in_a_non_final_group();
    test_decode_rejects_invalid_character();
    test_decode_null_args_return_false();
    test_round_trip_every_padding_case();

    puts("base64 tests passed");
    return 0;
}
