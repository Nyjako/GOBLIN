/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    sanitize_filename.h — turn arbitrary (possibly untrusted) text into a
    single safe filename/directory-name path component, portable across
    Windows/Linux/macOS/Android.
*/

#ifndef GOBLIN_SANITIZE_FILENAME_H
#define GOBLIN_SANITIZE_FILENAME_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Allowed bytes: every printable ASCII byte (0x20-0x7E) except the nine
    Windows reserves for path syntax (< > : " / \ | ? *) -- Linux/macOS
    only additionally forbid '/' (macOS also ':'), already covered by that
    same set, so satisfying Windows' stricter rule satisfies all three.
    Every ASCII control byte (0x00-0x1F, 0x7F) is disallowed too. When
    ascii_only is true, every byte >= 0x80 is disallowed as well.

    Every disallowed byte is replaced with '_'. A run of UTF-8 continuation
    bytes belonging to the same codepoint (ascii_only case, or a malformed
    sequence either way) collapses to exactly one '_', never one per byte.
    A trailing run of '.'/' ' is trimmed afterward (Windows can't create a
    name ending in either); an empty result (nothing left after trimming,
    or empty/NULL input) falls back to "_" rather than ever producing an
    unusable empty name. The result is also checked against Windows'
    reserved device names (CON, PRN, AUX, NUL, COM1-9, LPT1-9, matched
    case-insensitively against the part of the name before the first '.')
    and nudged with one extra '_' if it collides -- "con.txt" is just as
    blocked on Windows as "CON" itself.

    Writes at most out_buf_size - 1 bytes plus a NUL terminator into
    out_buf, truncating the input if it doesn't fit -- safe at any byte
    boundary when ascii_only is true (the output is then pure
    single-byte-per-character ASCII), and never splits a multi-byte UTF-8
    sequence when ascii_only is false (stops before a sequence that
    wouldn't fit whole, rather than cutting into it). Most filesystems in
    practice (ext4, NTFS, APFS) cap a single path component around 255
    bytes/UTF-16 code units; size out_buf with headroom below that for
    whatever your own call sites need on top (a mandatory prefix built
    separately, Android SAF provider overhead, ...). Returns the output
    length (excluding the NUL), same convention snprintf uses.

    If you want known Latin diacritics transliterated (e.g. "e" instead of
    "_") rather than collapsed to '_', run input through
    goblin_transliterate_to_ascii() first and pass its output here with
    ascii_only set to true -- the two headers compose, deliberately, rather
    than one including the other.
*/
size_t goblin_sanitize_filename_segment(const char *input, bool ascii_only, char *out_buf, size_t out_buf_size);

#ifdef __cplusplus
}
namespace goblin {
    static inline size_t sanitize_filename_segment(const char *input, bool ascii_only, char *out_buf, size_t out_buf_size) {
        return goblin_sanitize_filename_segment(input, ascii_only, out_buf, out_buf_size);
    }
}
#endif

#ifdef GOBLIN_SANITIZE_FILENAME_IMPLEMENTATION

#include <string.h>

static bool goblin_sanitize_is_windows_reserved_char(unsigned char c)
{
    switch (c) {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            return true;
        default:
            return false;
    }
}

static bool goblin_sanitize_is_allowed_ascii_byte(unsigned char c)
{
    if (c < 0x20 || c == 0x7F) return false;
    return !goblin_sanitize_is_windows_reserved_char(c);
}

static bool goblin_sanitize_is_utf8_continuation_byte(unsigned char c)
{
    return (c & 0xC0) == 0x80;
}

/*
    Returns the validated byte length (2, 3 or 4) of the complete UTF-8
    sequence starting at input[i], or 0 if it is not one -- a bare
    continuation byte, a lead byte Unicode never assigns (0xC0, 0xC1,
    0xF5-0xFF), a sequence truncated by the end of the string, a
    continuation byte count that does not match what the lead byte
    declares, an overlong encoding, or an encoded UTF-16 surrogate half.
    Callers fall back to treating anything this rejects as exactly one bad
    byte, rather than guessing how much of the input it was meant to cover
    -- the previous version of this function instead greedily swallowed
    every following continuation byte unconditionally, so a single
    malformed run could absorb an unbounded number of bytes into one
    "sequence" that then failed to fit the output buffer and truncated the
    whole rest of an otherwise-valid string.
*/
static size_t goblin_sanitize_utf8_seq_len(const unsigned char *input, size_t input_len, size_t i)
{
    unsigned char c = input[i];
    size_t expected;
    unsigned char min_second = 0x80, max_second = 0xBF;

    if ((c & 0xE0) == 0xC0) {
        if (c < 0xC2) return 0; /* 0xC0/0xC1: always an overlong 2-byte form */
        expected = 2;
    } else if ((c & 0xF0) == 0xE0) {
        expected = 3;
        if (c == 0xE0) min_second = 0xA0;      /* reject overlong 3-byte forms */
        else if (c == 0xED) max_second = 0x9F; /* reject encoded surrogate halves */
    } else if ((c & 0xF8) == 0xF0) {
        if (c > 0xF4) return 0; /* would decode past U+10FFFF */
        expected = 4;
        if (c == 0xF0) min_second = 0x90; /* reject overlong 4-byte forms */
        else if (c == 0xF4) max_second = 0x8F; /* reject > U+10FFFF */
    } else {
        return 0; /* ASCII byte or a lead byte that is never valid */
    }

    if (i + expected > input_len) return 0;

    unsigned char second = input[i + 1];
    if (second < min_second || second > max_second) return 0;

    for (size_t k = 2; k < expected; k++) {
        if (!goblin_sanitize_is_utf8_continuation_byte(input[i + k])) return 0;
    }
    return expected;
}

static bool goblin_sanitize_is_reserved_device_name(const char *base, size_t base_len)
{
    static const char *const reserved[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    };

    for (size_t r = 0; r < sizeof(reserved) / sizeof(reserved[0]); r++) {
        size_t len = strlen(reserved[r]);
        if (len != base_len) continue;

        bool match = true;
        for (size_t k = 0; k < len; k++) {
            unsigned char c = (unsigned char)base[k];
            unsigned char upper = (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : c;
            if (upper != (unsigned char)reserved[r][k]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

size_t goblin_sanitize_filename_segment(const char *input, bool ascii_only, char *out_buf, size_t out_buf_size)
{
    if (!out_buf || out_buf_size == 0) return 0;
    if (!input) input = "";

    const unsigned char *bytes = (const unsigned char *)input;
    size_t input_len = strlen(input);
    size_t out_len = 0;
    size_t i = 0;

    while (i < input_len) {
        unsigned char c = bytes[i];

        if (c < 0x80) {
            if (out_len + 1 >= out_buf_size) break;
            out_buf[out_len++] = goblin_sanitize_is_allowed_ascii_byte(c) ? (char)c : '_';
            i++;
            continue;
        }

        size_t seq_len = goblin_sanitize_utf8_seq_len(bytes, input_len, i);
        if (seq_len == 0) {
            /* Not a valid, complete sequence here -- one bad byte, not an
               unbounded run of them. */
            if (out_len + 1 >= out_buf_size) break;
            out_buf[out_len++] = '_';
            i++;
            continue;
        }

        if (ascii_only) {
            if (out_len + 1 >= out_buf_size) break;
            out_buf[out_len++] = '_';
            i += seq_len;
            continue;
        }

        if (out_len + seq_len + 1 > out_buf_size) break; /* whole sequence must fit, never split it */
        memcpy(out_buf + out_len, input + i, seq_len);
        out_len += seq_len;
        i += seq_len;
    }

    /* Windows can't create a name ending in '.' or ' ' -- both are
       single-byte ASCII, safe to trim regardless of ascii_only. */
    while (out_len > 0 && (out_buf[out_len - 1] == '.' || out_buf[out_len - 1] == ' ')) {
        out_len--;
    }

    if (out_len == 0 && out_buf_size >= 2) {
        out_buf[0] = '_';
        out_len = 1;
    }

    size_t base_len = 0;
    while (base_len < out_len && out_buf[base_len] != '.') base_len++;

    if (goblin_sanitize_is_reserved_device_name(out_buf, base_len)) {
        if (out_len + 1 < out_buf_size) {
            memmove(out_buf + base_len + 1, out_buf + base_len, out_len - base_len);
            out_buf[base_len] = '_';
            out_len++;
        } else {
            /* No room to grow -- every reserved name is at least 3 bytes
               long, so base_len - 1 is always a valid index. Mutating the
               base name's own last byte still guarantees it no longer
               matches. */
            out_buf[base_len - 1] = '_';
        }
    }

    out_buf[out_len] = '\0';
    return out_len;
}

#endif /* GOBLIN_SANITIZE_FILENAME_IMPLEMENTATION */

#endif /* GOBLIN_SANITIZE_FILENAME_H */
