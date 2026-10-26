#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define GOBLIN_FILEIO_IMPLEMENTATION
#include "../include/goblin/fileio.h"

static FILE *make_temp_file_with_text(const char *text)
{
    FILE *f = tmpfile();
    assert(f != NULL);

    if (text != NULL) {
        fputs(text, f);
    }

    rewind(f);
    return f;
}

static void test_read_line_empty_file_returns_null(void)
{
    FILE *f = make_temp_file_with_text("");
    size_t len = 12345;

    char *line = goblin_read_line(f, 16, &len);
    assert(line == NULL);
    assert(len == 12345); /* untouched on EOF before data */

    fclose(f);
}

static void test_read_line_single_line_with_newline(void)
{
    FILE *f = make_temp_file_with_text("hello\n");
    size_t len = 0;

    char *line = goblin_read_line(f, 8, &len);
    assert(line != NULL);
    assert(strcmp(line, "hello\n") == 0);
    assert(len == 6);
    free(line);

    assert(goblin_read_line(f, 8, &len) == NULL);
    fclose(f);
}

static void test_read_line_single_line_without_newline(void)
{
    FILE *f = make_temp_file_with_text("hello");
    size_t len = 0;

    char *line = goblin_read_line(f, 8, &len);
    assert(line != NULL);
    assert(strcmp(line, "hello") == 0);
    assert(len == 5);
    free(line);

    assert(goblin_read_line(f, 8, NULL) == NULL);
    fclose(f);
}

static void test_read_line_multiple_lines_in_loop(void)
{
    FILE *f = make_temp_file_with_text("one\ntwo\nthree\n");
    size_t len = 0;
    char *line;

    line = goblin_read_line(f, 8, &len);
    assert(line != NULL);
    assert(strcmp(line, "one\n") == 0);
    assert(len == 4);
    free(line);

    line = goblin_read_line(f, 8, &len);
    assert(line != NULL);
    assert(strcmp(line, "two\n") == 0);
    assert(len == 4);
    free(line);

    line = goblin_read_line(f, 8, &len);
    assert(line != NULL);
    assert(strcmp(line, "three\n") == 0);
    assert(len == 6);
    free(line);

    assert(goblin_read_line(f, 8, &len) == NULL);
    fclose(f);
}

static void test_read_line_long_line_grows_buffer(void)
{
    char text[600];
    memset(text, 'a', sizeof(text) - 2);
    text[sizeof(text) - 2] = '\n';
    text[sizeof(text) - 1] = '\0';

    FILE *f = make_temp_file_with_text(text);
    size_t len = 0;

    char *line = goblin_read_line(f, 4, &len);
    assert(line != NULL);

    size_t expected_len = strlen(text);
    assert(strlen(line) == expected_len);
    assert(strcmp(line, text) == 0);
    assert(len == expected_len);

    free(line);

    assert(goblin_read_line(f, 4, &len) == NULL);
    fclose(f);
}

static void test_read_line_null_fp_returns_null(void)
{
    assert(goblin_read_line(NULL, 16, NULL) == NULL);
}

static void test_read_line_zero_initial_capacity_returns_null(void)
{
    FILE *f = make_temp_file_with_text("hello\n");
    assert(goblin_read_line(f, 0, NULL) == NULL);
    fclose(f);
}

static void test_fseek64_and_ftell64(void)
{
    FILE *f = tmpfile();
    assert(f != NULL);

    assert(goblin_fseek64(f, 123456789, SEEK_SET) == 0);

    int64_t pos = goblin_ftell64(f);
    assert(pos == 123456789);

    fclose(f);
}

static void test_mkdir_creates_new_directory(void)
{
    const char *path = "goblin_test_dir_1";

    /* start from a clean state in case a previous failed run left it behind */
    remove(path);

    assert(goblin_path_exists(path) == false);
    assert(goblin_mkdir(path) == true);
    assert(goblin_path_exists(path) == true);

#if defined(_WIN32)
    _rmdir(path);
#else
    rmdir(path);
#endif
}

static void test_mkdir_on_existing_directory_is_success(void)
{
    const char *path = "goblin_test_dir_2";

    remove(path);
    assert(goblin_mkdir(path) == true);

    /* creating it again must still report success, not fail because it
     * already exists */
    assert(goblin_mkdir(path) == true);

#if defined(_WIN32)
    _rmdir(path);
#else
    rmdir(path);
#endif
}

static void test_mkdir_null_path_returns_false(void)
{
    assert(goblin_mkdir(NULL) == false);
}

static void test_path_exists_on_file(void)
{
    const char *path = "goblin_test_file_1.tmp";
    remove(path);

    assert(goblin_path_exists(path) == false);

    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fclose(f);

    assert(goblin_path_exists(path) == true);

    remove(path);
    assert(goblin_path_exists(path) == false);
}

static void test_path_exists_null_path_returns_false(void)
{
    assert(goblin_path_exists(NULL) == false);
}

static void test_path_writable_on_a_writable_directory(void)
{
    assert(goblin_path_writable(".") == true);
}

static void test_path_writable_on_a_nonexistent_path_is_false(void)
{
    const char *path = "goblin_test_dir_does_not_exist";
    remove(path);
    assert(goblin_path_writable(path) == false);
}

static void test_path_writable_null_path_returns_false(void)
{
    assert(goblin_path_writable(NULL) == false);
}

int main(void) {
    test_fseek64_and_ftell64();
    test_mkdir_creates_new_directory();
    test_mkdir_on_existing_directory_is_success();
    test_mkdir_null_path_returns_false();
    test_path_exists_on_file();
    test_path_exists_null_path_returns_false();
    test_path_writable_on_a_writable_directory();
    test_path_writable_on_a_nonexistent_path_is_false();
    test_path_writable_null_path_returns_false();
    test_read_line_empty_file_returns_null();
    test_read_line_single_line_with_newline();
    test_read_line_single_line_without_newline();
    test_read_line_multiple_lines_in_loop();
    test_read_line_long_line_grows_buffer();
    test_read_line_null_fp_returns_null();
    test_read_line_zero_initial_capacity_returns_null();

    puts("fileio tests passed");
    return 0;
}
