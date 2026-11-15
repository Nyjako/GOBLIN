/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    ascii_transliterate.h — best-effort transliteration of UTF-8 text into
    plain ASCII, mapping known Latin letters-with-diacritics to their base
    letter (Polish "a" -> 'a', "s" -> 's', "c" -> 'c', German "u" -> 'u',
    French "e" -> 'e', ...) instead of just discarding them.
*/

#ifndef GOBLIN_ASCII_TRANSLITERATE_H
#define GOBLIN_ASCII_TRANSLITERATE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Writes a best-effort ASCII transliteration of input into out_buf
    (must be at least out_buf_size bytes). Each input codepoint becomes:

      - itself, unchanged, if it is already ASCII (< 0x80);
      - its mapped 1-2 ASCII bytes, if it falls in the Latin-1 Supplement
        or Latin Extended-A blocks (U+00C0-U+017F) and has one -- this
        covers most accented Latin letters used by European languages
        (Polish, Czech, German, French, Spanish, Nordic, ...), plus a
        handful of digraphs for ligature-like letters ("ss", "ae", "th",
        "oe", "ij", "ng");
      - fallback_byte otherwise -- anything with no Latin transliteration
        (Cyrillic, Greek, CJK, emoji, ...), a handful of non-letter
        codepoints in that same Latin-1 Supplement block (e.g. the
        multiplication/division signs), and any malformed UTF-8 byte.

    One input codepoint always maps to exactly one *decision* (zero extra
    bytes beyond what it is replaced by), so a 3-byte CJK character still
    only costs one fallback_byte in the output, never one byte per input
    byte; a malformed byte is consumed one byte at a time instead of being
    folded into whatever sequence happens to follow it. Truncates to fit
    out_buf_size (never splits a 2-byte replacement across the boundary)
    and always NUL-terminates. Returns the output length (excluding the
    NUL), same convention snprintf uses.

    This only transliterates -- it does not otherwise restrict which ASCII
    bytes come out the other end (control bytes and shell/path-special
    characters that were already ASCII in the input pass straight
    through). Feed its output into goblin_sanitize_filename_segment()
    (sanitize_filename.h) with ascii_only = true if you also need a safe
    filename out of it; the two headers compose rather than one including
    the other.
*/
size_t goblin_transliterate_to_ascii(const char *input, char fallback_byte, char *out_buf, size_t out_buf_size);

#ifdef __cplusplus
}
namespace goblin {
    static inline size_t transliterate_to_ascii(const char *input, char fallback_byte, char *out_buf, size_t out_buf_size) {
        return goblin_transliterate_to_ascii(input, fallback_byte, out_buf, out_buf_size);
    }
}
#endif

#ifdef GOBLIN_ASCII_TRANSLITERATE_IMPLEMENTATION

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define GOBLIN_TRANSLITERATE_INVALID 0xFFFFFFFFu

static bool goblin_transliterate_is_continuation_byte(unsigned char c)
{
    return (c & 0xC0) == 0x80;
}

/*
    Decodes the codepoint starting at input[i], writing its byte length
    (1-4) to *out_seq_len. On anything malformed -- a bare continuation
    byte, a lead byte Unicode never assigns, a sequence truncated by the
    end of the string, an overlong encoding, or an encoded UTF-16
    surrogate half -- returns GOBLIN_TRANSLITERATE_INVALID and sets
    *out_seq_len to 1, so the caller recovers one byte at a time rather
    than guessing how much of the input a malformed lead byte was meant to
    cover.
*/
static uint32_t goblin_transliterate_decode(const unsigned char *input, size_t input_len, size_t i, size_t *out_seq_len)
{
    unsigned char c = input[i];

    if (c < 0x80) {
        *out_seq_len = 1;
        return c;
    }

    size_t expected;
    uint32_t codepoint;
    unsigned char min_second = 0x80, max_second = 0xBF;

    if ((c & 0xE0) == 0xC0) {
        if (c < 0xC2) {
            *out_seq_len = 1;
            return GOBLIN_TRANSLITERATE_INVALID; /* 0xC0/0xC1: always an overlong 2-byte form */
        }
        expected = 2;
        codepoint = (uint32_t)(c & 0x1F);
    } else if ((c & 0xF0) == 0xE0) {
        expected = 3;
        codepoint = (uint32_t)(c & 0x0F);
        if (c == 0xE0) min_second = 0xA0;      /* reject overlong 3-byte forms */
        else if (c == 0xED) max_second = 0x9F; /* reject encoded surrogate halves */
    } else if ((c & 0xF8) == 0xF0) {
        if (c > 0xF4) {
            *out_seq_len = 1;
            return GOBLIN_TRANSLITERATE_INVALID; /* would decode past U+10FFFF */
        }
        expected = 4;
        codepoint = (uint32_t)(c & 0x07);
        if (c == 0xF0) min_second = 0x90;      /* reject overlong 4-byte forms */
        else if (c == 0xF4) max_second = 0x8F; /* reject > U+10FFFF */
    } else {
        *out_seq_len = 1;
        return GOBLIN_TRANSLITERATE_INVALID; /* ASCII handled above, so only a stray lead/continuation byte reaches here */
    }

    if (i + expected > input_len) {
        *out_seq_len = 1;
        return GOBLIN_TRANSLITERATE_INVALID;
    }

    unsigned char second = input[i + 1];
    if (second < min_second || second > max_second) {
        *out_seq_len = 1;
        return GOBLIN_TRANSLITERATE_INVALID;
    }
    codepoint = (codepoint << 6) | (uint32_t)(second & 0x3F);

    for (size_t k = 2; k < expected; k++) {
        unsigned char cont = input[i + k];
        if (!goblin_transliterate_is_continuation_byte(cont)) {
            *out_seq_len = 1;
            return GOBLIN_TRANSLITERATE_INVALID;
        }
        codepoint = (codepoint << 6) | (uint32_t)(cont & 0x3F);
    }

    *out_seq_len = expected;
    return codepoint;
}

/*
    Indexed by codepoint - 0xC0, covering U+00C0-U+017F (Latin-1
    Supplement + Latin Extended-A): 192 entries, NULL where the codepoint
    is not a letter (e.g. the multiplication/division signs) or has no
    sensible single-letter transliteration.
*/
static const char *const goblin_transliterate_table[192] = {
    /* 0xC0 */ "A", "A", "A", "A", "A", "A", "AE", "C",
    /* 0xC8 */ "E", "E", "E", "E", "I", "I", "I", "I",
    /* 0xD0 */ "D", "N", "O", "O", "O", "O", "O", NULL,
    /* 0xD8 */ "O", "U", "U", "U", "U", "Y", "TH", "ss",
    /* 0xE0 */ "a", "a", "a", "a", "a", "a", "ae", "c",
    /* 0xE8 */ "e", "e", "e", "e", "i", "i", "i", "i",
    /* 0xF0 */ "d", "n", "o", "o", "o", "o", "o", NULL,
    /* 0xF8 */ "o", "u", "u", "u", "u", "y", "th", "y",
    /* 0x100 */ "A", "a", "A", "a", "A", "a", "C", "c",
    /* 0x108 */ "C", "c", "C", "c", "C", "c", "D", "d",
    /* 0x110 */ "D", "d", "E", "e", "E", "e", "E", "e",
    /* 0x118 */ "E", "e", "E", "e", "G", "g", "G", "g",
    /* 0x120 */ "G", "g", "G", "g", "H", "h", "H", "h",
    /* 0x128 */ "I", "i", "I", "i", "I", "i", "I", "i",
    /* 0x130 */ "I", "i", "IJ", "ij", "J", "j", "K", "k",
    /* 0x138 */ "k", "L", "l", "L", "l", "L", "l", "L",
    /* 0x140 */ "l", "L", "l", "N", "n", "N", "n", "N",
    /* 0x148 */ "n", "n", "NG", "ng", "O", "o", "O", "o",
    /* 0x150 */ "O", "o", "OE", "oe", "R", "r", "R", "r",
    /* 0x158 */ "R", "r", "S", "s", "S", "s", "S", "s",
    /* 0x160 */ "S", "s", "T", "t", "T", "t", "T", "t",
    /* 0x168 */ "U", "u", "U", "u", "U", "u", "U", "u",
    /* 0x170 */ "U", "u", "U", "u", "W", "w", "Y", "y",
    /* 0x178 */ "Y", "Z", "z", "Z", "z", "Z", "z", "s",
};

size_t goblin_transliterate_to_ascii(const char *input, char fallback_byte, char *out_buf, size_t out_buf_size)
{
    if (!out_buf || out_buf_size == 0) return 0;
    if (!input) input = "";

    const unsigned char *bytes = (const unsigned char *)input;
    size_t input_len = strlen(input);
    size_t out_len = 0;
    size_t i = 0;

    while (i < input_len) {
        size_t seq_len;
        uint32_t codepoint = goblin_transliterate_decode(bytes, input_len, i, &seq_len);

        char single[2];
        single[1] = '\0';
        const char *replacement = NULL;

        if (codepoint < 0x80) {
            single[0] = (char)codepoint;
            replacement = single;
        } else if (codepoint != GOBLIN_TRANSLITERATE_INVALID && codepoint >= 0xC0 && codepoint <= 0x17F) {
            replacement = goblin_transliterate_table[codepoint - 0xC0];
        }

        if (!replacement) {
            single[0] = fallback_byte;
            replacement = single;
        }

        size_t replacement_len = strlen(replacement);
        if (out_len + replacement_len + 1 > out_buf_size) break; /* never split a replacement across the boundary */

        memcpy(out_buf + out_len, replacement, replacement_len);
        out_len += replacement_len;
        i += seq_len;
    }

    out_buf[out_len] = '\0';
    return out_len;
}

#endif /* GOBLIN_ASCII_TRANSLITERATE_IMPLEMENTATION */

#endif /* GOBLIN_ASCII_TRANSLITERATE_H */
