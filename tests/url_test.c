#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GOBLIN_URL_IMPLEMENTATION
#include "../include/goblin/url.h"

static void test_encode_leaves_unreserved_characters_alone(void)
{
    char *out = goblin_url_encode("abcXYZ019-_.~");
    assert(out != NULL);
    assert(strcmp(out, "abcXYZ019-_.~") == 0);
    free(out);
}

static void test_encode_percent_escapes_everything_else(void)
{
    char *out = goblin_url_encode("a b/c+d");
    assert(out != NULL);
    assert(strcmp(out, "a%20b%2Fc%2Bd") == 0);
    free(out);
}

static void test_encode_empty_string(void)
{
    char *out = goblin_url_encode("");
    assert(out != NULL);
    assert(strcmp(out, "") == 0);
    free(out);
}

static void test_encode_null_returns_null(void)
{
    assert(goblin_url_encode(NULL) == NULL);
}

static void test_decode_reverses_percent_escapes(void)
{
    char *out = goblin_url_decode("a%20b%2Fc%2Bd");
    assert(out != NULL);
    assert(strcmp(out, "a b/c+d") == 0);
    free(out);
}

static void test_decode_plus_becomes_space(void)
{
    char *out = goblin_url_decode("a+b+c");
    assert(out != NULL);
    assert(strcmp(out, "a b c") == 0);
    free(out);
}

static void test_decode_is_case_insensitive_for_hex_digits(void)
{
    char *out = goblin_url_decode("%2f%2F");
    assert(out != NULL);
    assert(strcmp(out, "//") == 0);
    free(out);
}

static void test_decode_truncated_escape_fails(void)
{
    assert(goblin_url_decode("abc%2") == NULL);
    assert(goblin_url_decode("abc%") == NULL);
}

static void test_decode_invalid_hex_digit_fails(void)
{
    assert(goblin_url_decode("%zz") == NULL);
}

static void test_decode_null_returns_null(void)
{
    assert(goblin_url_decode(NULL) == NULL);
}

static void test_round_trip(void)
{
    const char *original = "hello world/x=1&y=2#frag";
    char *encoded = goblin_url_encode(original);
    assert(encoded != NULL);

    char *decoded = goblin_url_decode(encoded);
    assert(decoded != NULL);
    assert(strcmp(decoded, original) == 0);

    free(encoded);
    free(decoded);
}

int main(void)
{
    test_encode_leaves_unreserved_characters_alone();
    test_encode_percent_escapes_everything_else();
    test_encode_empty_string();
    test_encode_null_returns_null();
    test_decode_reverses_percent_escapes();
    test_decode_plus_becomes_space();
    test_decode_is_case_insensitive_for_hex_digits();
    test_decode_truncated_escape_fails();
    test_decode_invalid_hex_digit_fails();
    test_decode_null_returns_null();
    test_round_trip();

    puts("url tests passed");
    return 0;
}
