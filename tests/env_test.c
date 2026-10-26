#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GOBLIN_ENV_IMPLEMENTATION
#include "../include/goblin/env.h"

static void test_setenv_sets_a_new_variable(void)
{
    assert(getenv("GOBLIN_ENV_TEST_VAR_1") == NULL);
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_1", "hello", 1) == true);

    const char *v = getenv("GOBLIN_ENV_TEST_VAR_1");
    assert(v != NULL);
    assert(strcmp(v, "hello") == 0);
}

static void test_setenv_with_overwrite_replaces_existing_value(void)
{
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_2", "first", 1) == true);
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_2", "second", 1) == true);

    const char *v = getenv("GOBLIN_ENV_TEST_VAR_2");
    assert(v != NULL);
    assert(strcmp(v, "second") == 0);
}

static void test_setenv_without_overwrite_keeps_existing_value(void)
{
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_3", "first", 1) == true);
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_3", "second", 0) == true);

    const char *v = getenv("GOBLIN_ENV_TEST_VAR_3");
    assert(v != NULL);
    assert(strcmp(v, "first") == 0);
}

static void test_setenv_without_overwrite_sets_an_unset_variable(void)
{
    assert(getenv("GOBLIN_ENV_TEST_VAR_4") == NULL);
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_4", "value", 0) == true);

    const char *v = getenv("GOBLIN_ENV_TEST_VAR_4");
    assert(v != NULL);
    assert(strcmp(v, "value") == 0);
}

static void test_setenv_null_name_or_value_returns_false(void)
{
    assert(goblin_setenv(NULL, "value", 1) == false);
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_5", NULL, 1) == false);
}

int main(void)
{
    test_setenv_sets_a_new_variable();
    test_setenv_with_overwrite_replaces_existing_value();
    test_setenv_without_overwrite_keeps_existing_value();
    test_setenv_without_overwrite_sets_an_unset_variable();
    test_setenv_null_name_or_value_returns_false();

    puts("env tests passed");
    return 0;
}
