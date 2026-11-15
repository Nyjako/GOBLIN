#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_ASCII_TRANSLITERATE_IMPLEMENTATION
#include "../include/goblin/ascii_transliterate.h"

static char g_buf[256];

static void test_pure_ascii_passes_through_unchanged(void)
{
    size_t len = goblin_transliterate_to_ascii("Hello, World! 123", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "Hello, World! 123") == 0);
    assert(len == strlen("Hello, World! 123"));
}

static void test_polish_diacritics(void)
{
    /* "turn a goose sour by playing a viol" -- the standard Polish
       pangram, hitting every Polish letter with a diacritic at least
       once: a->a c->c e->e l->l n->n o->o s->s z->z (and z->z again). */
    goblin_transliterate_to_ascii("zażółć gęślą jaźń", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "zazolc gesla jazn") == 0);
}

static void test_german_umlauts_and_sharp_s(void)
{
    goblin_transliterate_to_ascii("München", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "Munchen") == 0);

    goblin_transliterate_to_ascii("Straße", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "Strasse") == 0);
}

static void test_french_accents(void)
{
    goblin_transliterate_to_ascii("café", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "cafe") == 0);

    goblin_transliterate_to_ascii("Élysée", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "Elysee") == 0);
}

static void test_ligature_digraphs(void)
{
    goblin_transliterate_to_ascii("æ", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "ae") == 0);

    goblin_transliterate_to_ascii("Æ", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "AE") == 0);

    goblin_transliterate_to_ascii("œ", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "oe") == 0);

    goblin_transliterate_to_ascii("þ", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "th") == 0);
}

static void test_non_letter_codepoints_in_mapped_block_use_fallback(void)
{
    /* U+00D7 (multiplication sign) and U+00F7 (division sign) sit inside
       the Latin-1 Supplement range this header otherwise maps, but
       neither is a letter -- both must fall back rather than silently
       becoming some unrelated ASCII character. */
    /* "3\xD7""7" i.e. "3x7" -- the string literal concatenation keeps the
       '7' from being swallowed into the preceding \x escape, which in C
       consumes as many following hex digits as it can find. */
    goblin_transliterate_to_ascii("3\xC3\x97" "7", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "3_7") == 0);
}

static void test_unmapped_scripts_collapse_one_codepoint_to_one_fallback_byte(void)
{
    /* Cyrillic "Privet" (6 codepoints, 2 bytes each) and CJK (2
       codepoints, 3 bytes each) have no Latin transliteration -- each
       whole codepoint must cost exactly one fallback byte, never one byte
       per UTF-8 byte. */
    size_t len = goblin_transliterate_to_ascii("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "______") == 0);
    assert(len == 6);

    goblin_transliterate_to_ascii("\xE6\x97\xA5\xE6\x9C\xAC", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "__") == 0);
}

static void test_emoji_collapses_to_one_fallback_byte(void)
{
    /* U+1F600 GRINNING FACE, a 4-byte sequence. */
    goblin_transliterate_to_ascii("\xF0\x9F\x98\x80", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "_") == 0);
}

static void test_custom_fallback_byte(void)
{
    goblin_transliterate_to_ascii("\xD0\x9F", '?', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "?") == 0);
}

static void test_malformed_byte_consumes_exactly_one_byte(void)
{
    /* 0xFF is never a valid UTF-8 byte at all -- must become exactly one
       fallback byte and not swallow the "ok" that follows it. */
    goblin_transliterate_to_ascii("\xFFok", '_', g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "_ok") == 0);
}

static void test_truncation_never_splits_a_digraph_replacement(void)
{
    /* "a" + sharp-s (-> "ss"): a 3-byte buffer has room for "a" plus the
       NUL, but not for the 2-byte "ss" replacement too -- it must be
       dropped whole, not split into just one 's'. */
    char small_buf[3];
    size_t len = goblin_transliterate_to_ascii("a\xC3\x9F", '_', small_buf, sizeof(small_buf));
    assert(strcmp(small_buf, "a") == 0);
    assert(len == 1);
}

static void test_null_input_yields_empty_string(void)
{
    size_t len = goblin_transliterate_to_ascii(NULL, '_', g_buf, sizeof(g_buf));
    assert(len == 0);
    assert(strcmp(g_buf, "") == 0);
}

static void test_null_out_buf_or_zero_size_is_a_safe_no_op(void)
{
    assert(goblin_transliterate_to_ascii("abc", '_', NULL, sizeof(g_buf)) == 0);
    assert(goblin_transliterate_to_ascii("abc", '_', g_buf, 0) == 0);
}

int main(void)
{
    test_pure_ascii_passes_through_unchanged();
    test_polish_diacritics();
    test_german_umlauts_and_sharp_s();
    test_french_accents();
    test_ligature_digraphs();
    test_non_letter_codepoints_in_mapped_block_use_fallback();
    test_unmapped_scripts_collapse_one_codepoint_to_one_fallback_byte();
    test_emoji_collapses_to_one_fallback_byte();
    test_custom_fallback_byte();
    test_malformed_byte_consumes_exactly_one_byte();
    test_truncation_never_splits_a_digraph_replacement();
    test_null_input_yields_empty_string();
    test_null_out_buf_or_zero_size_is_a_safe_no_op();

    puts("ascii_transliterate tests passed");
    return 0;
}
