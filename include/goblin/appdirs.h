/*
 G OBLIN by Kacper Tuchols*ki
 https://github.com/Nyjako/GOBLIN

 appdirs.h - Tiny cross-platform helper for locating per-user directories.
 */

#ifndef GOBLIN_APPDIR_H
#define GOBLIN_APPDIR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
    #endif

    typedef enum goblin_appdir_kind {
        GOBLIN_APPDIR_DATA,
        GOBLIN_APPDIR_CONFIG,
        GOBLIN_APPDIR_CACHE,
        GOBLIN_APPDIR_STATE,
    } goblin_appdir_kind;

    char *goblin_appdir_join(const char *base, const char *child);
    char *goblin_appdir_data_dir(const char *app_name);
    char *goblin_appdir_config_dir(const char *app_name);
    char *goblin_appdir_cache_dir(const char *app_name);
    char *goblin_appdir_state_dir(const char *app_name);

    /* Returns 0 on success, non-zero on failure. */
    int goblin_appdir_mkdir_p(const char *path);

    /* Returns true if path exists and is a directory. */
    bool goblin_appdir_is_dir(const char *path);

    #ifdef __cplusplus
} namespace goblin {
    static inline char *appdir_join(const char *base, const char *child) { return goblin_appdir_join(base, child); }
    static inline char *appdir_data_dir(const char *app_name) { return goblin_appdir_data_dir(app_name); }
    static inline char *appdir_config_dir(const char *app_name) { return goblin_appdir_config_dir(app_name); }
    static inline char *appdir_cache_dir(const char *app_name) { return goblin_appdir_cache_dir(app_name); }
    static inline char *appdir_state_dir(const char *app_name) { return goblin_appdir_state_dir(app_name); }
    static inline int mkdir_p_appdir(const char *path) { return goblin_appdir_mkdir_p(path); }
    static inline bool is_dir_appdir(const char *path) { return goblin_appdir_is_dir(path); }
}
#endif

#ifdef GOBLIN_APPDIR_IMPLEMENTATION

#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static inline char *goblin_appdir_copy_cstr(const char *s)
{
    size_t n;
    char *p;

    if (!s) return NULL;
    n = strlen(s) + 1;
    p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

#ifdef _WIN32
static inline char goblin_appdir_sep(void) { return '\\'; }
#else
static inline char goblin_appdir_sep(void) { return '/'; }
#endif

static inline bool goblin_appdir_is_sep(char c)
{
    return c == '/' || c == '\\';
}

static inline bool goblin_appdir_starts_with_drive(const char *path)
{
    #ifdef _WIN32
    return path && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':';
    #else
    (void)path;
    return false;
    #endif
}

static inline const char *goblin_appdir_getenv_first(const char *a, const char *b, const char *c, const char *d)
{
    const char *v;

    if (a) { v = getenv(a); if (v && *v) return v; }
    if (b) { v = getenv(b); if (v && *v) return v; }
    if (c) { v = getenv(c); if (v && *v) return v; }
    if (d) { v = getenv(d); if (v && *v) return v; }
    return NULL;
}

static inline char *goblin_appdir_join2(const char *a, const char *b)
{
    size_t alen;
    size_t blen;
    size_t need_sep;
    const char *bptr;
    char *out;

    if (!a || !b) return NULL;

    alen = strlen(a);
    blen = strlen(b);

    if (alen == 0) return goblin_appdir_copy_cstr(b);
    if (blen == 0) return goblin_appdir_copy_cstr(a);

    bptr = b;
    need_sep = 0;

    if (goblin_appdir_is_sep(a[alen - 1]) && goblin_appdir_is_sep(bptr[0])) {
        ++bptr;
        --blen;
    } else if (!goblin_appdir_is_sep(a[alen - 1]) && !goblin_appdir_is_sep(bptr[0])) {
        need_sep = 1u;
    }

    out = (char *)malloc(alen + blen + need_sep + 1u);
    if (!out) return NULL;

    memcpy(out, a, alen);
    if (need_sep) out[alen++] = goblin_appdir_sep();
    memcpy(out + alen, bptr, blen);
    out[alen + blen] = '\0';
    return out;
}

char *goblin_appdir_join(const char *base, const char *child)
{
    return goblin_appdir_join2(base, child);
}

static inline char *goblin_appdir_home_dir(void)
{
    #ifdef _WIN32
    const char *v;
    const char *drive;
    const char *path;

    v = getenv("USERPROFILE");
    if (v && *v) return goblin_appdir_copy_cstr(v);

    drive = getenv("HOMEDRIVE");
    path = getenv("HOMEPATH");
    if (drive && *drive && path && *path) return goblin_appdir_join2(drive, path);

    v = getenv("HOME");
    if (v && *v) return goblin_appdir_copy_cstr(v);

    return NULL;
    #else
    const char *v = getenv("HOME");
    if (v && *v) return goblin_appdir_copy_cstr(v);
    return NULL;
    #endif
}

static inline char *goblin_appdir_join_home_subdir(const char *subdir)
{
    char *home;
    char *out;

    home = goblin_appdir_home_dir();
    if (!home) return goblin_appdir_copy_cstr(".");
    out = goblin_appdir_join2(home, subdir);
    free(home);
    return out;
}

static inline char *goblin_appdir_base_dir(goblin_appdir_kind kind)
{
    #ifdef _WIN32
    const char *env;

    switch (kind) {
        case GOBLIN_APPDIR_CONFIG:
            env = goblin_appdir_getenv_first("APPDATA", "LOCALAPPDATA", "USERPROFILE", "HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            return goblin_appdir_join_home_subdir("AppData\\Roaming");

        case GOBLIN_APPDIR_CACHE:
        case GOBLIN_APPDIR_STATE:
        case GOBLIN_APPDIR_DATA:
        default:
            env = goblin_appdir_getenv_first("LOCALAPPDATA", "APPDATA", "USERPROFILE", "HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            return goblin_appdir_join_home_subdir("AppData\\Local");
    }
    #else
    const char *env;
    const char *home;

    home = getenv("HOME");
    switch (kind) {
        case GOBLIN_APPDIR_CONFIG:
            env = getenv("XDG_CONFIG_HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            if (home && *home) return goblin_appdir_join2(home, ".config");
            break;

        case GOBLIN_APPDIR_CACHE:
            env = getenv("XDG_CACHE_HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            if (home && *home) return goblin_appdir_join2(home, ".cache");
            break;

        case GOBLIN_APPDIR_STATE:
            env = getenv("XDG_STATE_HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            if (home && *home) return goblin_appdir_join2(home, ".local/state");
            break;

        case GOBLIN_APPDIR_DATA:
        default:
            env = getenv("XDG_DATA_HOME");
            if (env && *env) return goblin_appdir_copy_cstr(env);
            if (home && *home) return goblin_appdir_join2(home, ".local/share");
            break;
    }

    return goblin_appdir_copy_cstr(".");
    #endif
}

static inline char *goblin_appdir_kind_dir(goblin_appdir_kind kind, const char *app_name)
{
    char *base;
    char *out;

    base = goblin_appdir_base_dir(kind);
    if (!base) return NULL;
    if (!app_name || !*app_name) return base;

    #ifdef _WIN32
    if (goblin_appdir_starts_with_drive(app_name) || goblin_appdir_is_sep(app_name[0])) {
        free(base);
        return goblin_appdir_copy_cstr(app_name);
    }
    #else
    if (app_name[0] == '/') {
        free(base);
        return goblin_appdir_copy_cstr(app_name);
    }
    #endif

    out = goblin_appdir_join2(base, app_name);
    free(base);
    return out;
}

char *goblin_appdir_data_dir(const char *app_name)
{
    return goblin_appdir_kind_dir(GOBLIN_APPDIR_DATA, app_name);
}

char *goblin_appdir_config_dir(const char *app_name)
{
    return goblin_appdir_kind_dir(GOBLIN_APPDIR_CONFIG, app_name);
}

char *goblin_appdir_cache_dir(const char *app_name)
{
    return goblin_appdir_kind_dir(GOBLIN_APPDIR_CACHE, app_name);
}

char *goblin_appdir_state_dir(const char *app_name)
{
    return goblin_appdir_kind_dir(GOBLIN_APPDIR_STATE, app_name);
}

bool goblin_appdir_is_dir(const char *path)
{
    if (!path || !*path) return false;
    #ifdef _WIN32
    {
        struct _stat st;
        if (_stat(path, &st) != 0) return false;
        return (st.st_mode & _S_IFDIR) != 0;
    }
    #else
    {
        struct stat st;
        if (stat(path, &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }
    #endif
}

static inline int goblin_appdir_mkdir_one(const char *path)
{
    if (!path || !*path) return -1;
    if (goblin_appdir_is_dir(path)) return 0;
    #ifdef _WIN32
    if (_mkdir(path) == 0) return 0;
    #else
    if (mkdir(path, 0755) == 0) return 0;
    #endif
    if (errno == EEXIST && goblin_appdir_is_dir(path)) return 0;
    return -1;
}

#ifdef _WIN32
static inline size_t goblin_appdir_windows_prefix_len(const char *path)
{
    size_t i;

    if (!path || !*path) return 0;

    if (goblin_appdir_starts_with_drive(path)) {
        i = 2;
        if (goblin_appdir_is_sep(path[i])) ++i;
        return i;
    }

    if (goblin_appdir_is_sep(path[0]) && goblin_appdir_is_sep(path[1])) {
        i = 2;
        while (path[i] && !goblin_appdir_is_sep(path[i])) ++i; /* server */
            while (path[i] && goblin_appdir_is_sep(path[i])) ++i;
            while (path[i] && !goblin_appdir_is_sep(path[i])) ++i; /* share */
                while (path[i] && goblin_appdir_is_sep(path[i])) ++i;
                return i;
    }

    if (goblin_appdir_is_sep(path[0])) return 1;
    return 0;
}
#endif

int goblin_appdir_mkdir_p(const char *path)
{
    size_t len;
    size_t i;
    size_t prefix_len;
    size_t end;
    char *tmp;
    int rc;

    if (!path || !*path) return -1;
    if (goblin_appdir_is_dir(path)) return 0;

    len = strlen(path);
    tmp = (char *)malloc(len + 1u);
    if (!tmp) return -1;
    memcpy(tmp, path, len + 1u);
    rc = 0;

    #ifdef _WIN32
    prefix_len = goblin_appdir_windows_prefix_len(tmp);
    #else
    prefix_len = goblin_appdir_is_sep(tmp[0]) ? 1u : 0u;
    #endif

    for (i = prefix_len; i < len; ++i) {
        if (!goblin_appdir_is_sep(tmp[i])) continue;
        if (i > 0 && goblin_appdir_is_sep(tmp[i - 1])) continue;

        tmp[i] = '\0';
        if (tmp[0] && !goblin_appdir_is_dir(tmp)) {
            if (goblin_appdir_mkdir_one(tmp) != 0) {
                rc = -1;
                goto done;
            }
        }
        tmp[i] = goblin_appdir_sep();
    }

    end = len;
    while (end > prefix_len && goblin_appdir_is_sep(tmp[end - 1])) {
        --end;
    }
    tmp[end] = '\0';

    if (tmp[0] && !goblin_appdir_is_dir(tmp)) {
        if (goblin_appdir_mkdir_one(tmp) != 0) rc = -1;
    }

    done:
    free(tmp);
    return rc;
}

#endif /* GOBLIN_APPDIR_IMPLEMENTATION */

#endif /* GOBLIN_APPDIR_H */
