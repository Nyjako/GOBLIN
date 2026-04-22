#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GOBLIN_CSTR_IMPLEMENTATION
#include "../include/goblin/cstr.h"

static void test_copy(void)
{
    char *s = goblin_copy_cstr("hello");
    assert(s != NULL);
    assert(strcmp(s, "hello") == 0);
    free(s);

    assert(goblin_copy_cstr(NULL) == NULL);
}

static void test_concat(void)
{
    char *s = goblin_concat_cstr("hello", "world");
    assert(s != NULL);
    assert(strcmp(s, "helloworld") == 0);
    free(s);

    assert(goblin_concat_cstr(NULL, "world") == NULL);
    assert(goblin_concat_cstr("hello", NULL) == NULL);
    assert(goblin_concat_cstr(NULL, NULL) == NULL);
}

static void test_variadic_concat(void)
{
    char *s;

    s = goblin_variadic_concat_cstr(2, "hello", "world");
    assert(s != NULL);
    assert(strcmp(s, "helloworld") == 0);
    free(s);

    s = goblin_variadic_concat_cstr(4, "a", "b", "c", "d");
    assert(s != NULL);
    assert(strcmp(s, "abcd") == 0);
    free(s);

    s = goblin_variadic_concat_cstr(3, "hello", "", "world");
    assert(s != NULL);
    assert(strcmp(s, "helloworld") == 0);
    free(s);

    s = goblin_variadic_concat_cstr(0);
    assert(s != NULL);
    assert(strcmp(s, "") == 0);
    free(s);

    assert(goblin_variadic_concat_cstr(2, "hello", NULL) == NULL);
    assert(goblin_variadic_concat_cstr(1, NULL) == NULL);
}

static void test_trim_start(void)
{
    char buf[] = " \t\r\nhello";
    goblin_trim_start_cstr(buf);
    assert(strcmp(buf, "hello") == 0);

    char buf2[] = "hello";
    goblin_trim_start_cstr(buf2);
    assert(strcmp(buf2, "hello") == 0);
}

static void test_trim_end(void)
{
    char buf[] = "hello \t\r\n";
    goblin_trim_end_cstr(buf);
    assert(strcmp(buf, "hello") == 0);

    char buf2[] = "hello";
    goblin_trim_end_cstr(buf2);
    assert(strcmp(buf2, "hello") == 0);
}

static void test_trim(void)
{
    char buf[] = " \t\r\nhello world \t\r\n";
    goblin_trim_cstr(buf);
    assert(strcmp(buf, "hello world") == 0);

    char buf2[] = "hello";
    goblin_trim_cstr(buf2);
    assert(strcmp(buf2, "hello") == 0);

    char buf3[] = " \t\r\n";
    goblin_trim_cstr(buf3);
    assert(strcmp(buf3, "") == 0);
}

static void test_starts_with(void)
{
    assert(goblin_starts_with_cstr("Hello, World!", "Hello"));
    assert(goblin_starts_with_cstr("foo bar baz", "foo bar baz"));
    assert(!goblin_starts_with_cstr("foo", "foo bar baz"));
}

static void test_ends_with(void)
{
    assert(goblin_ends_with_cstr("Hello, World!", "World!"));
    assert(goblin_ends_with_cstr("foo bar baz", "foo bar baz"));
    assert(!goblin_ends_with_cstr("foo", "foo bar baz"));
}

static void test_contains(void)
{
    assert(goblin_contains_cstr("Hello, World!", "Hello") == 0);
    assert(goblin_contains_cstr("foo bar baz", "foo bar baz") == 0);
    assert(goblin_contains_cstr("foo bar baz", "bar") == 4);
    assert(goblin_contains_cstr("foo", "foo bar baz") == -1);
    assert(goblin_contains_cstr("abc", "c") == 2);
    assert(goblin_contains_cstr("abc", "") == 0);
    assert(goblin_contains_cstr("", "") == 0);
    assert(goblin_contains_cstr("", "a") == -1);
    assert(goblin_contains_cstr("aaaaa", "aaa") == 0);
    assert(goblin_contains_cstr("banana", "ana") == 1);
    assert(goblin_contains_cstr("mississippi", "issip") == 4);
    assert(goblin_contains_cstr("abc", "abcd") == -1);
    assert(goblin_contains_cstr(NULL, "a") == -1);
    assert(goblin_contains_cstr("a", NULL) == -1);
}

static void test_slice(void)
{
    char *s;
    s = goblin_slice_cstr("hello", 0, 5);
    assert(s != NULL);
    assert(strcmp(s, "hello") == 0);
    free(s);

    s = goblin_slice_cstr("hello", 1, 4);
    assert(s != NULL);
    assert(strcmp(s, "ell") == 0);
    free(s);

    s = goblin_slice_cstr("hello", 2, 3);
    assert(s != NULL);
    assert(strcmp(s, "l") == 0);
    free(s);

    s = goblin_slice_cstr("hello", 3, 5);
    assert(s != NULL);
    assert(strcmp(s, "lo") == 0);
    free(s);

    assert(goblin_slice_cstr("hello", 2, 2) == NULL);
    assert(goblin_slice_cstr("hello", 5, 2) == NULL);
    assert(goblin_slice_cstr("hello", 0, 6) == NULL);
    assert(goblin_slice_cstr(NULL, 0, 1) == NULL);

    size_t len = strlen("hello");
    s = goblin_slice_cstr("hello", 0, len);
    assert(s != NULL);
    assert(strcmp(s, "hello") == 0);
    free(s);
}

int main(void)
{
    test_copy();
    test_concat();
    test_variadic_concat();
    test_trim_start();
    test_trim_end();
    test_trim();
    test_starts_with();
    test_ends_with();
    test_contains();
    test_slice();

    puts("cstr tests passed");
    return 0;
}
