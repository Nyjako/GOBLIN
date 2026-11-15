#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_SANITIZE_FILENAME_IMPLEMENTATION
#include "../include/goblin/sanitize_filename.h"

static char g_buf[256];

static void test_leaves_already_safe_ascii_untouched(void)
{
    size_t len = goblin_sanitize_filename_segment("My Comic (Part 2)", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "My Comic (Part 2)") == 0);
    assert(len == strlen("My Comic (Part 2)"));
}

static void test_replaces_windows_reserved_characters_with_underscore(void)
{
    const char *input = "Some/Bad:Comic*Name?\"<>|\\";
    size_t len = goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(len == strlen(input));
    for (size_t i = 0; i < len; i++) {
        bool is_reserved = strchr("/:*?\"<>|\\", input[i]) != NULL;
        assert(g_buf[i] == (is_reserved ? '_' : input[i]));
    }
}

static void test_replaces_control_bytes(void)
{
    char input[] = {'a', '\t', '\n', 'b', '\0'};
    goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "a__b") == 0);
}

static void test_null_input_falls_back_to_underscore(void)
{
    goblin_sanitize_filename_segment(NULL, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "_") == 0);
}

static void test_empty_input_falls_back_to_underscore(void)
{
    goblin_sanitize_filename_segment("", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "_") == 0);
}

static void test_input_that_trims_to_nothing_falls_back_to_underscore(void)
{
    goblin_sanitize_filename_segment(" . . ", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "_") == 0);
}

static void test_trims_trailing_dots_and_spaces(void)
{
    goblin_sanitize_filename_segment("My Title. ", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "My Title") == 0);
}

static void test_ascii_only_true_collapses_a_non_ascii_codepoint_to_one_underscore(void)
{
    /* U+00E9 ("e"), 2 bytes in UTF-8 (0xC3 0xA9) -- must become exactly one
       '_', not two. */
    goblin_sanitize_filename_segment("caf\xC3\xA9 au lait", true, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "caf_ au lait") == 0);
}

static void test_ascii_only_false_preserves_valid_non_ascii_bytes_verbatim(void)
{
    goblin_sanitize_filename_segment("caf\xC3\xA9", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "caf\xC3\xA9") == 0);
}

static void test_truncates_to_fit_out_buf_size(void)
{
    char small_buf[6];
    size_t len = goblin_sanitize_filename_segment("abcdefghij", false, small_buf, sizeof(small_buf));
    assert(strcmp(small_buf, "abcde") == 0);
    assert(len == 5);
}

static void test_truncation_never_splits_a_multibyte_sequence_when_not_ascii_only(void)
{
    /* "ab" + U+00E9 (2 bytes) -- a 4-byte buffer only has room for "ab" plus
       the NUL, so the 2-byte sequence must be dropped whole, not split. */
    char small_buf[4];
    goblin_sanitize_filename_segment("ab\xC3\xA9", false, small_buf, sizeof(small_buf));
    assert(strcmp(small_buf, "ab") == 0);
}

static void test_out_buf_size_one_writes_only_the_nul_terminator(void)
{
    char tiny_buf[1];
    size_t len = goblin_sanitize_filename_segment("anything", false, tiny_buf, sizeof(tiny_buf));
    assert(len == 0);
    assert(strcmp(tiny_buf, "") == 0);
}

/* --- Hardening: malformed UTF-8 is rejected one bad byte at a time ------
 * The previous version of this function (before it was proposed for
 * GOBLIN) treated ANY byte >= 0x80 as "the first byte of a sequence" and
 * then greedily counted every following continuation-pattern byte
 * (0x80-0xBF) as part of it, with no check that the lead byte actually
 * declared that many of them. A long run of bytes that merely looked like
 * continuation bytes -- stray 0x80s, or an invalid lead byte followed by
 * some -- collapsed into one oversized "sequence" that then either got
 * copied verbatim as if it were valid UTF-8, or (if it didn't fit the
 * output buffer) aborted the whole copy immediately via the "doesn't fit,
 * stop" branch, discarding any valid content that followed it even though
 * most of the buffer was still empty. These tests pin the fix: each
 * invalid byte costs exactly one output byte, so the buffer still fills
 * up gracefully instead of collapsing to nothing. */

static void test_long_malformed_run_fills_the_buffer_instead_of_collapsing_to_empty(void)
{
    /* 0xF5 is not a valid UTF-8 lead byte at all (> 0xF4), followed by 30
       bytes that merely look like continuation bytes. None of this is a
       real sequence of any length -- every one of the 31 bytes must be
       rejected on its own. */
    char input[1 + 30 + 1];
    input[0] = (char)0xF5;
    for (int i = 0; i < 30; i++) input[1 + i] = (char)0x80;
    input[31] = '\0';

    char buf[10];
    size_t len = goblin_sanitize_filename_segment(input, false, buf, sizeof(buf));

    /* Every byte of buf (minus the NUL) is '_' -- the buffer was used to
       capacity one bad byte at a time, not abandoned after a single
       oversized (and wrong) length computation. */
    assert(len == sizeof(buf) - 1);
    for (size_t i = 0; i < len; i++) assert(buf[i] == '_');
}

static void test_short_malformed_run_does_not_swallow_trailing_valid_text(void)
{
    const char *input = "\xF5\x80\x80hello";
    size_t len = goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "___hello") == 0);
    assert(len == 8);
}

static void test_overlong_two_byte_encoding_rejected_byte_by_byte(void)
{
    /* 0xC0 0x80 -- the classic overlong encoding of NUL. Never a valid
       sequence; both bytes must be rejected individually. */
    const char *input = "\xC0\x80";
    goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "__") == 0);
}

static void test_encoded_surrogate_half_rejected_byte_by_byte(void)
{
    /* 0xED 0xA0 0x80 would decode to U+D800, a UTF-16 surrogate half --
       never a valid Unicode scalar value, so never valid UTF-8 either. */
    const char *input = "\xED\xA0\x80";
    goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "___") == 0);
}

static void test_out_of_range_four_byte_lead_rejected_byte_by_byte(void)
{
    /* 0xF5 and above would decode past U+10FFFF, the highest assigned
       Unicode scalar value -- not a valid lead byte at all. */
    const char *input = "\xF5\x80\x80\x80";
    goblin_sanitize_filename_segment(input, false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "____") == 0);
}

/* --- Hardening: Windows reserved device names --------------------------
 * CON, PRN, AUX, NUL, COM1-9 and LPT1-9 can't be created on Windows no
 * matter the extension, matched case-insensitively against the part of
 * the name before the first '.'. The previous version only stripped bad
 * characters, so "con.txt" sailed straight through untouched. */

static void test_reserved_name_exact_match_gets_nudged(void)
{
    goblin_sanitize_filename_segment("CON", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "CON_") == 0);
}

static void test_reserved_name_match_is_case_insensitive(void)
{
    goblin_sanitize_filename_segment("con", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "con_") == 0);
}

static void test_reserved_name_with_extension_is_nudged_before_the_dot(void)
{
    goblin_sanitize_filename_segment("con.txt", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "con_.txt") == 0);
}

static void test_reserved_name_with_multiple_dots_checks_only_up_to_the_first(void)
{
    goblin_sanitize_filename_segment("NUL.tar.gz", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "NUL_.tar.gz") == 0);
}

static void test_com_and_lpt_variants_are_recognized(void)
{
    goblin_sanitize_filename_segment("COM3", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "COM3_") == 0);

    goblin_sanitize_filename_segment("lpt9", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "lpt9_") == 0);
}

static void test_name_that_merely_starts_with_a_reserved_prefix_is_untouched(void)
{
    /* "console" is 7 bytes before any dot -- no reserved name is that
       long, so this must not be mistaken for "CON". */
    goblin_sanitize_filename_segment("console", false, g_buf, sizeof(g_buf));
    assert(strcmp(g_buf, "console") == 0);
}

static void test_reserved_name_nudge_falls_back_to_mutation_when_buffer_is_full(void)
{
    /* Exactly 4 bytes: "CON" + NUL, no room to grow by inserting a byte.
       The fix must still guarantee the result no longer collides, by
       mutating the base name's own last byte instead. */
    char small_buf[4];
    size_t len = goblin_sanitize_filename_segment("CON", false, small_buf, sizeof(small_buf));
    assert(strcmp(small_buf, "CO_") == 0);
    assert(len == 3);
}

int main(void)
{
    test_leaves_already_safe_ascii_untouched();
    test_replaces_windows_reserved_characters_with_underscore();
    test_replaces_control_bytes();
    test_null_input_falls_back_to_underscore();
    test_empty_input_falls_back_to_underscore();
    test_input_that_trims_to_nothing_falls_back_to_underscore();
    test_trims_trailing_dots_and_spaces();
    test_ascii_only_true_collapses_a_non_ascii_codepoint_to_one_underscore();
    test_ascii_only_false_preserves_valid_non_ascii_bytes_verbatim();
    test_truncates_to_fit_out_buf_size();
    test_truncation_never_splits_a_multibyte_sequence_when_not_ascii_only();
    test_out_buf_size_one_writes_only_the_nul_terminator();

    test_long_malformed_run_fills_the_buffer_instead_of_collapsing_to_empty();
    test_short_malformed_run_does_not_swallow_trailing_valid_text();
    test_overlong_two_byte_encoding_rejected_byte_by_byte();
    test_encoded_surrogate_half_rejected_byte_by_byte();
    test_out_of_range_four_byte_lead_rejected_byte_by_byte();

    test_reserved_name_exact_match_gets_nudged();
    test_reserved_name_match_is_case_insensitive();
    test_reserved_name_with_extension_is_nudged_before_the_dot();
    test_reserved_name_with_multiple_dots_checks_only_up_to_the_first();
    test_com_and_lpt_variants_are_recognized();
    test_name_that_merely_starts_with_a_reserved_prefix_is_untouched();
    test_reserved_name_nudge_falls_back_to_mutation_when_buffer_is_full();

    puts("sanitize_filename tests passed");
    return 0;
}
