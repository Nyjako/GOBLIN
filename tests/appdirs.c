#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define goblin_test_mkdir(path) _mkdir(path)
#define goblin_test_rmdir(path) _rmdir(path)

static void goblin_test_setenv(const char *name, const char *value)
{
    assert(_putenv_s(name, value) == 0);
}

static void goblin_test_unsetenv(const char *name)
{
    assert(_putenv_s(name, "") == 0);
}

static char goblin_test_sep(void)
{
    return '\\';
}
#else
#include <sys/stat.h>
#include <unistd.h>
#define goblin_test_mkdir(path) mkdir(path, 0755)
#define goblin_test_rmdir(path) rmdir(path)

static void goblin_test_setenv(const char *name, const char *value)
{
    assert(setenv(name, value, 1) == 0);
}

static void goblin_test_unsetenv(const char *name)
{
    assert(unsetenv(name) == 0);
}

static char goblin_test_sep(void)
{
    return '/';
}
#endif

#define GOBLIN_APPDIR_IMPLEMENTATION
#include "../include/goblin/appdirs.h"

static void test_join(void)
{
    char *s;
    char sep = goblin_test_sep();
    char expected[64];

    snprintf(expected, sizeof expected, "foo%cbar", sep);

    s = goblin_appdir_join("foo", "bar");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    s = goblin_appdir_join("foo/", "bar");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    s = goblin_appdir_join("foo", "/bar");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    s = goblin_appdir_join("foo/", "/bar");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    s = goblin_appdir_join("", "bar");
    assert(s != NULL);
    assert(strcmp(s, "bar") == 0);
    free(s);

    s = goblin_appdir_join("foo", "");
    assert(s != NULL);
    assert(strcmp(s, "foo") == 0);
    free(s);

    assert(goblin_appdir_join(NULL, "bar") == NULL);
    assert(goblin_appdir_join("foo", NULL) == NULL);
}

static void test_dirs(void)
{
    char sep = goblin_test_sep();
    char base[256];
    char data_home[256];
    char config_home[256];
    char cache_home[256];
    char state_home[256];
    char *s;
    char expected[512];

    snprintf(base, sizeof base, "goblin_appdir_test_env");
    snprintf(data_home, sizeof data_home, "%s%cdata", base, sep);
    snprintf(config_home, sizeof config_home, "%s%cconfig", base, sep);
    snprintf(cache_home, sizeof cache_home, "%s%ccache", base, sep);
    snprintf(state_home, sizeof state_home, "%s%cstate", base, sep);

    #ifdef _WIN32
    goblin_test_setenv("APPDATA", config_home);
    goblin_test_setenv("LOCALAPPDATA", data_home);
    goblin_test_setenv("USERPROFILE", base);
    goblin_test_setenv("HOME", base);
    #else
    goblin_test_setenv("XDG_DATA_HOME", data_home);
    goblin_test_setenv("XDG_CONFIG_HOME", config_home);
    goblin_test_setenv("XDG_CACHE_HOME", cache_home);
    goblin_test_setenv("XDG_STATE_HOME", state_home);
    goblin_test_setenv("HOME", base);
    #endif

    snprintf(expected, sizeof expected, "%s%cGOBLIN", data_home, sep);
    s = goblin_appdir_data_dir("GOBLIN");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    snprintf(expected, sizeof expected, "%s%cGOBLIN", config_home, sep);
    s = goblin_appdir_config_dir("GOBLIN");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    snprintf(expected, sizeof expected, "%s%cGOBLIN", cache_home, sep);
    s = goblin_appdir_cache_dir("GOBLIN");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    snprintf(expected, sizeof expected, "%s%cGOBLIN", state_home, sep);
    s = goblin_appdir_state_dir("GOBLIN");
    assert(s != NULL);
    assert(strcmp(s, expected) == 0);
    free(s);

    s = goblin_appdir_data_dir("");
    assert(s != NULL);
    assert(strcmp(s, data_home) == 0);
    free(s);

    s = goblin_appdir_config_dir(NULL);
    assert(s != NULL);
    assert(strcmp(s, config_home) == 0);
    free(s);

    #ifdef _WIN32
    s = goblin_appdir_data_dir("C:\\absolute\\app");
    assert(s != NULL);
    assert(strcmp(s, "C:\\absolute\\app") == 0);
    free(s);
    #else
    s = goblin_appdir_data_dir("/absolute/app");
    assert(s != NULL);
    assert(strcmp(s, "/absolute/app") == 0);
    free(s);
    #endif
}

static void test_is_dir_and_mkdir_p(void)
{
    char sep = goblin_test_sep();
    char root[256];
    char a[512];
    char b[512];
    char c[512];
    char d[512];

    snprintf(root, sizeof root, "goblin_appdir_test_tree");
    snprintf(a, sizeof a, "%s%cone", root, sep);
    snprintf(b, sizeof b, "%s%ctwo", a, sep);
    snprintf(c, sizeof c, "%s%cthree", b, sep);
    snprintf(d, sizeof d, "%s%c", c, sep);

    assert(!goblin_appdir_is_dir(root));

    assert(goblin_appdir_mkdir_p(d) == 0);
    assert(goblin_appdir_is_dir(root));
    assert(goblin_appdir_is_dir(a));
    assert(goblin_appdir_is_dir(b));
    assert(goblin_appdir_is_dir(c));
    assert(goblin_appdir_is_dir(d));

    assert(goblin_appdir_mkdir_p(d) == 0);

    /* cleanup */
    goblin_test_rmdir(c);
    goblin_test_rmdir(b);
    goblin_test_rmdir(a);
    goblin_test_rmdir(root);
}

int main(void)
{
    #ifdef _WIN32
    goblin_test_unsetenv("APPDATA");
    goblin_test_unsetenv("LOCALAPPDATA");
    goblin_test_unsetenv("USERPROFILE");
    goblin_test_unsetenv("HOME");
    #else
    goblin_test_unsetenv("XDG_DATA_HOME");
    goblin_test_unsetenv("XDG_CONFIG_HOME");
    goblin_test_unsetenv("XDG_CACHE_HOME");
    goblin_test_unsetenv("XDG_STATE_HOME");
    goblin_test_unsetenv("HOME");
    #endif

    test_join();
    test_dirs();
    test_is_dir_and_mkdir_p();

    puts("appdirs tests passed");
    return 0;
}
