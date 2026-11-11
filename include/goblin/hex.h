/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    hex.h — binary <-> lowercase hex string conversion.
*/

#ifndef GOBLIN_HEX_H
#define GOBLIN_HEX_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    out must be at least len*2+1 bytes. Writes a NUL-terminated lowercase
    hex string.
*/
void goblin_bytes_to_hex(const unsigned char *data, size_t len, char *out);

/*
    hex must have an even length and contain only [0-9a-fA-F]. out must be
    at least strlen(hex)/2 bytes. Returns false (out left untouched) on a
    malformed input.
*/
bool goblin_hex_to_bytes(const char *hex, unsigned char *out);

#ifdef __cplusplus
}
namespace goblin {
    static inline void bytes_to_hex(const unsigned char *data, size_t len, char *out) { goblin_bytes_to_hex(data, len, out); }
    static inline bool hex_to_bytes(const char *hex, unsigned char *out) { return goblin_hex_to_bytes(hex, out); }
}
#endif

#ifdef GOBLIN_HEX_IMPLEMENTATION

void goblin_bytes_to_hex(const unsigned char *data, size_t len, char *out)
{
    static const char digits[] = "0123456789abcdef";
    if (!data || !out) return;

    for (size_t i = 0; i < len; i++) {
        out[i * 2] = digits[(data[i] >> 4) & 0xF];
        out[i * 2 + 1] = digits[data[i] & 0xF];
    }
    out[len * 2] = '\0';
}

static inline int goblin_hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

bool goblin_hex_to_bytes(const char *hex, unsigned char *out)
{
    if (!hex || !out) return false;

    size_t len = 0;
    while (hex[len]) len++;
    if (len % 2 != 0) return false;

    for (size_t i = 0; i < len / 2; i++) {
        int hi = goblin_hex_nibble(hex[i * 2]);
        int lo = goblin_hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return true;
}

#endif /* GOBLIN_HEX_IMPLEMENTATION */

#endif /* GOBLIN_HEX_H */
