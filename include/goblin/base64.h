/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    base64.h — standard (RFC 4648) base64 with '=' padding.
*/

#ifndef GOBLIN_BASE64_H
#define GOBLIN_BASE64_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Returns a NUL-terminated base64 string, or NULL on allocation failure.
*/
char *goblin_base64_encode(const unsigned char *data, size_t len);

/*
    out must be at least 3*strlen(encoded)/4 bytes. *out_len receives the
    decoded byte count. Returns false (out and out_len left untouched) if
    encoded is NULL, its length isn't a multiple of 4, or it contains a
    character outside the base64 alphabet (padding aside).
*/
bool goblin_base64_decode(const char *encoded, unsigned char *out, size_t *out_len);

#ifdef __cplusplus
}
namespace goblin {
    static inline char *base64_encode(const unsigned char *data, size_t len) { return goblin_base64_encode(data, len); }
    static inline bool base64_decode(const char *encoded, unsigned char *out, size_t *out_len) { return goblin_base64_decode(encoded, out, out_len); }
}
#endif

#ifdef GOBLIN_BASE64_IMPLEMENTATION

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char goblin_base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *goblin_base64_encode(const unsigned char *data, size_t len)
{
    if (!data && len > 0) return NULL;

    size_t out_len = 4 * ((len + 2) / 3);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        uint32_t a = i < len ? data[i++] : 0;
        uint32_t b = i < len ? data[i++] : 0;
        uint32_t c = i < len ? data[i++] : 0;
        uint32_t triple = (a << 16) + (b << 8) + c;

        out[j++] = goblin_base64_table[(triple >> 18) & 0x3F];
        out[j++] = goblin_base64_table[(triple >> 12) & 0x3F];
        out[j++] = goblin_base64_table[(triple >> 6) & 0x3F];
        out[j++] = goblin_base64_table[triple & 0x3F];
    }

    size_t mod = len % 3;
    if (mod == 1) {
        out[out_len - 1] = '=';
        out[out_len - 2] = '=';
    } else if (mod == 2) {
        out[out_len - 1] = '=';
    }
    out[out_len] = '\0';
    return out;
}

static inline int goblin_base64_value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return 26 + (c - 'a');
    if (c >= '0' && c <= '9') return 52 + (c - '0');
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool goblin_base64_decode(const char *encoded, unsigned char *out, size_t *out_len)
{
    if (!encoded || !out || !out_len) return false;

    size_t len = strlen(encoded);
    if (len == 0 || len % 4 != 0) return false;

    size_t written = 0;
    for (size_t i = 0; i < len; i += 4) {
        int pad = (encoded[i + 2] == '=') + (encoded[i + 3] == '=');
        if (pad > 0 && i + 4 != len) return false; /* padding only in the final group */

        int v0 = goblin_base64_value(encoded[i]);
        int v1 = goblin_base64_value(encoded[i + 1]);
        int v2 = pad >= 2 ? 0 : goblin_base64_value(encoded[i + 2]);
        int v3 = pad >= 1 ? 0 : goblin_base64_value(encoded[i + 3]);
        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;

        uint32_t triple = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;

        out[written++] = (unsigned char)((triple >> 16) & 0xFF);
        if (pad < 2) out[written++] = (unsigned char)((triple >> 8) & 0xFF);
        if (pad < 1) out[written++] = (unsigned char)(triple & 0xFF);
    }

    *out_len = written;
    return true;
}

#endif /* GOBLIN_BASE64_IMPLEMENTATION */

#endif /* GOBLIN_BASE64_H */
