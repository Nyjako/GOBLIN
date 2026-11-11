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

static void test_getenv_or_returns_the_set_value(void)
{
    assert(goblin_setenv("GOBLIN_ENV_TEST_VAR_6", "set-value", 1) == true);

    char *v = goblin_getenv_or("GOBLIN_ENV_TEST_VAR_6", "fallback");
    assert(v != NULL);
    assert(strcmp(v, "set-value") == 0);
    free(v);
}

static void test_getenv_or_falls_back_when_unset(void)
{
    assert(getenv("GOBLIN_ENV_TEST_VAR_UNSET_1") == NULL);

    char *v = goblin_getenv_or("GOBLIN_ENV_TEST_VAR_UNSET_1", "fallback");
    assert(v != NULL);
    assert(strcmp(v, "fallback") == 0);
    free(v);
}

static void test_getenv_or_null_default_on_unset_returns_null(void)
{
    assert(getenv("GOBLIN_ENV_TEST_VAR_UNSET_2") == NULL);
    assert(goblin_getenv_or("GOBLIN_ENV_TEST_VAR_UNSET_2", NULL) == NULL);
}

static void test_getenv_int_or_parses_a_set_value(void)
{
    assert(goblin_setenv("GOBLIN_ENV_TEST_INT_1", "42", 1) == true);
    assert(goblin_getenv_int_or("GOBLIN_ENV_TEST_INT_1", -1) == 42);
}

static void test_getenv_int_or_falls_back_when_unset(void)
{
    assert(getenv("GOBLIN_ENV_TEST_INT_UNSET") == NULL);
    assert(goblin_getenv_int_or("GOBLIN_ENV_TEST_INT_UNSET", 7) == 7);
}

static void test_getenv_int_or_falls_back_on_garbage(void)
{
    assert(goblin_setenv("GOBLIN_ENV_TEST_INT_2", "not-a-number", 1) == true);
    assert(goblin_getenv_int_or("GOBLIN_ENV_TEST_INT_2", 7) == 7);

    assert(goblin_setenv("GOBLIN_ENV_TEST_INT_3", "42trailing", 1) == true);
    assert(goblin_getenv_int_or("GOBLIN_ENV_TEST_INT_3", 7) == 7);

    assert(goblin_setenv("GOBLIN_ENV_TEST_INT_4", "", 1) == true);
    assert(goblin_getenv_int_or("GOBLIN_ENV_TEST_INT_4", 7) == 7);
}

static void test_getenv_bool_or_recognizes_truthy_and_falsy_values(void)
{
    const char *truthy[] = {"1", "true", "TRUE", "yes", "On"};
    for (size_t i = 0; i < sizeof(truthy) / sizeof(truthy[0]); i++) {
        assert(goblin_setenv("GOBLIN_ENV_TEST_BOOL", truthy[i], 1) == true);
        assert(goblin_getenv_bool_or("GOBLIN_ENV_TEST_BOOL", false) == true);
    }

    const char *falsy[] = {"0", "false", "FALSE", "no", "Off"};
    for (size_t i = 0; i < sizeof(falsy) / sizeof(falsy[0]); i++) {
        assert(goblin_setenv("GOBLIN_ENV_TEST_BOOL", falsy[i], 1) == true);
        assert(goblin_getenv_bool_or("GOBLIN_ENV_TEST_BOOL", true) == false);
    }
}

static void test_getenv_bool_or_falls_back_when_unset_or_unrecognized(void)
{
    assert(getenv("GOBLIN_ENV_TEST_BOOL_UNSET") == NULL);
    assert(goblin_getenv_bool_or("GOBLIN_ENV_TEST_BOOL_UNSET", true) == true);

    assert(goblin_setenv("GOBLIN_ENV_TEST_BOOL_2", "maybe", 1) == true);
    assert(goblin_getenv_bool_or("GOBLIN_ENV_TEST_BOOL_2", true) == true);
    assert(goblin_getenv_bool_or("GOBLIN_ENV_TEST_BOOL_2", false) == false);
}

int main(void)
{
    test_setenv_sets_a_new_variable();
    test_setenv_with_overwrite_replaces_existing_value();
    test_setenv_without_overwrite_keeps_existing_value();
    test_setenv_without_overwrite_sets_an_unset_variable();
    test_setenv_null_name_or_value_returns_false();
    test_getenv_or_returns_the_set_value();
    test_getenv_or_falls_back_when_unset();
    test_getenv_or_null_default_on_unset_returns_null();
    test_getenv_int_or_parses_a_set_value();
    test_getenv_int_or_falls_back_when_unset();
    test_getenv_int_or_falls_back_on_garbage();
    test_getenv_bool_or_recognizes_truthy_and_falsy_values();
    test_getenv_bool_or_falls_back_when_unset_or_unrecognized();

    puts("env tests passed");
    return 0;
}
