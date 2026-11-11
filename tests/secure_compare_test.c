#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_SECURE_COMPARE_IMPLEMENTATION
#include "../include/goblin/secure_compare.h"

static void test_streq_equal_strings(void)
{
    assert(goblin_secure_streq("secret-token", "secret-token") == true);
}

static void test_streq_different_strings_same_length(void)
{
    assert(goblin_secure_streq("secret-token", "secret-tokeN") == false);
}

static void test_streq_different_lengths(void)
{
    assert(goblin_secure_streq("short", "a-lot-longer") == false);
}

static void test_streq_empty_strings_are_equal(void)
{
    assert(goblin_secure_streq("", "") == true);
}

static void test_streq_null_args_return_false(void)
{
    assert(goblin_secure_streq(NULL, "x") == false);
    assert(goblin_secure_streq("x", NULL) == false);
    assert(goblin_secure_streq(NULL, NULL) == false);
}

static void test_memcmp_equal_buffers(void)
{
    const unsigned char a[] = {1, 2, 3, 4};
    const unsigned char b[] = {1, 2, 3, 4};
    assert(goblin_secure_memcmp(a, b, sizeof(a), sizeof(b)) == true);
}

static void test_memcmp_differing_byte(void)
{
    const unsigned char a[] = {1, 2, 3, 4};
    const unsigned char b[] = {1, 2, 9, 4};
    assert(goblin_secure_memcmp(a, b, sizeof(a), sizeof(b)) == false);
}

static void test_memcmp_different_lengths(void)
{
    const unsigned char a[] = {1, 2, 3};
    const unsigned char b[] = {1, 2, 3, 4};
    assert(goblin_secure_memcmp(a, b, sizeof(a), sizeof(b)) == false);
}

static void test_memcmp_null_args_return_false(void)
{
    const unsigned char a[] = {1};
    assert(goblin_secure_memcmp(NULL, a, 1, 1) == false);
    assert(goblin_secure_memcmp(a, NULL, 1, 1) == false);
}

int main(void)
{
    test_streq_equal_strings();
    test_streq_different_strings_same_length();
    test_streq_different_lengths();
    test_streq_empty_strings_are_equal();
    test_streq_null_args_return_false();
    test_memcmp_equal_buffers();
    test_memcmp_differing_byte();
    test_memcmp_different_lengths();
    test_memcmp_null_args_return_false();

    puts("secure_compare tests passed");
    return 0;
}
