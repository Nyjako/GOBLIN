#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_ARGV_IMPLEMENTATION
#include "../include/goblin/argv.h"

static void test_has_flag_finds_a_present_bare_flag(void)
{
    char *argv[] = {"prog", "--verbose", "--jobs", "4"};
    assert(goblin_argv_has_flag(4, argv, "--verbose") == true);
    assert(goblin_argv_has_flag(4, argv, "--missing") == false);
}

static void test_has_flag_never_matches_argv0(void)
{
    char *argv[] = {"--verbose", "x"};
    assert(goblin_argv_has_flag(2, argv, "--verbose") == false);
}

static void test_find_value_returns_the_following_token(void)
{
    char *argv[] = {"prog", "--jobs", "4", "--site", "e621"};
    assert(strcmp(goblin_argv_find_value(5, argv, "--jobs"), "4") == 0);
    assert(strcmp(goblin_argv_find_value(5, argv, "--site"), "e621") == 0);
}

static void test_find_value_is_null_when_flag_is_absent(void)
{
    char *argv[] = {"prog", "--jobs", "4"};
    assert(goblin_argv_find_value(3, argv, "--site") == NULL);
}

static void test_find_value_is_null_when_flag_is_the_last_token(void)
{
    char *argv[] = {"prog", "--jobs"};
    assert(goblin_argv_find_value(2, argv, "--jobs") == NULL);
}

static void test_find_value_uses_the_first_occurrence_on_a_repeated_flag(void)
{
    char *argv[] = {"prog", "--tag", "a", "--tag", "b"};
    assert(strcmp(goblin_argv_find_value(5, argv, "--tag"), "a") == 0);
}

static void test_find_index_returns_the_matching_position(void)
{
    char *argv[] = {"prog", "--download", "pool:1", "search:2"};
    assert(goblin_argv_find_index(4, argv, "--download") == 1);
}

static void test_find_index_is_minus_one_when_absent(void)
{
    char *argv[] = {"prog", "--download"};
    assert(goblin_argv_find_index(2, argv, "--search") == -1);
}

static void test_null_argv_or_flag_is_a_safe_no_match(void)
{
    char *argv[] = {"prog", "--x"};
    assert(goblin_argv_has_flag(2, argv, NULL) == false);
    assert(goblin_argv_has_flag(2, NULL, "--x") == false);
    assert(goblin_argv_find_value(2, argv, NULL) == NULL);
    assert(goblin_argv_find_index(2, argv, NULL) == -1);
}

int main(void)
{
    test_has_flag_finds_a_present_bare_flag();
    test_has_flag_never_matches_argv0();
    test_find_value_returns_the_following_token();
    test_find_value_is_null_when_flag_is_absent();
    test_find_value_is_null_when_flag_is_the_last_token();
    test_find_value_uses_the_first_occurrence_on_a_repeated_flag();
    test_find_index_returns_the_matching_position();
    test_find_index_is_minus_one_when_absent();
    test_null_argv_or_flag_is_a_safe_no_match();

    puts("argv tests passed");
    return 0;
}
