#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_ZIP_IMPLEMENTATION
#include "../include/goblin/zip.h"

#ifdef _WIN32
#include <direct.h>
#define GOBLIN_TEST_RMDIR _rmdir
#else
#include <unistd.h>
#define GOBLIN_TEST_RMDIR rmdir
#endif

static const char *g_tmp_path = "goblin_zip_test.zip";

static void write_basic_archive(void)
{
    goblin_zip_writer_t *w = goblin_zip_writer_open(g_tmp_path);
    assert(w != NULL);
    assert(goblin_zip_writer_add(w, "mimetype", (const unsigned char *)"application/epub+zip", 21));
    assert(goblin_zip_writer_add(w, "META-INF/container.xml", (const unsigned char *)"<x/>", 4));
    assert(goblin_zip_writer_add(w, "empty.txt", NULL, 0));
    assert(goblin_zip_writer_close(w));
}

static void test_round_trips_multiple_entries(void)
{
    write_basic_archive();

    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);

    size_t len = 0;
    const unsigned char *data = goblin_zip_reader_get(r, "mimetype", &len);
    assert(data != NULL);
    assert(len == 21);
    assert(memcmp(data, "application/epub+zip", 21) == 0);

    data = goblin_zip_reader_get(r, "META-INF/container.xml", &len);
    assert(data != NULL);
    assert(len == 4);
    assert(memcmp(data, "<x/>", 4) == 0);

    data = goblin_zip_reader_get(r, "empty.txt", &len);
    assert(data != NULL);
    assert(len == 0);

    goblin_zip_reader_close(r);
}

static void test_missing_entry_returns_null(void)
{
    write_basic_archive();
    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);
    size_t len = 0;
    assert(goblin_zip_reader_get(r, "nope.txt", &len) == NULL);
    goblin_zip_reader_close(r);
}

static void test_binary_data_with_embedded_zero_bytes_round_trips(void)
{
    goblin_zip_writer_t *w = goblin_zip_writer_open(g_tmp_path);
    assert(w != NULL);
    unsigned char payload[6] = {0x00, 0xFF, 0x00, 0x7F, 0x80, 0x00};
    assert(goblin_zip_writer_add(w, "bin", payload, sizeof(payload)));
    assert(goblin_zip_writer_close(w));

    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    size_t len = 0;
    const unsigned char *data = goblin_zip_reader_get(r, "bin", &len);
    assert(data != NULL);
    assert(len == sizeof(payload));
    assert(memcmp(data, payload, sizeof(payload)) == 0);
    goblin_zip_reader_close(r);
}

/* A general-purpose reader (unlike a self-archive-only one) has to tolerate
 * a trailing zip comment after the end-of-central-directory record's own
 * fixed 22 bytes -- appends one directly to a file this writer produced. */
static void test_eocd_is_found_past_a_trailing_comment(void)
{
    write_basic_archive();
    FILE *f = fopen(g_tmp_path, "ab");
    assert(f != NULL);
    /* The writer always emits a zero comment-length EOCD; patch it to claim
     * a comment, then actually append one, so this is a well-formed
     * "zip with a comment" rather than just garbage appended after EOCD. */
    fclose(f);

    FILE *rw = fopen(g_tmp_path, "r+b");
    assert(rw != NULL);
    fseek(rw, 0, SEEK_END);
    long size = ftell(rw);
    assert(size >= 22);
    /* comment length field is the last 2 bytes of the 22-byte EOCD record */
    fseek(rw, size - 2, SEEK_SET);
    unsigned char comment_len_bytes[2] = {5, 0}; /* little-endian 5 */
    fwrite(comment_len_bytes, 1, 2, rw);
    fseek(rw, 0, SEEK_END);
    fwrite("hello", 1, 5, rw);
    fclose(rw);

    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);
    size_t len = 0;
    const unsigned char *data = goblin_zip_reader_get(r, "mimetype", &len);
    assert(data != NULL);
    assert(len == 21);
    goblin_zip_reader_close(r);
}

static void test_corrupted_entry_bytes_are_rejected_via_crc(void)
{
    write_basic_archive();

    FILE *rw = fopen(g_tmp_path, "r+b");
    assert(rw != NULL);
    /* The first local file header is 30 bytes + "mimetype" (8 bytes) before
     * its data begins -- flip one byte of "application/epub+zip" itself. */
    fseek(rw, 30 + 8, SEEK_SET);
    unsigned char bad = 'X';
    fwrite(&bad, 1, 1, rw);
    fclose(rw);

    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);
    size_t len = 0;
    assert(goblin_zip_reader_get(r, "mimetype", &len) == NULL);
    goblin_zip_reader_close(r);
}

static void test_opening_a_nonexistent_file_returns_null(void)
{
    assert(goblin_zip_reader_open("this_file_does_not_exist_at_all.zip") == NULL);
}

static void test_opening_a_truncated_file_returns_null(void)
{
    FILE *f = fopen(g_tmp_path, "wb");
    assert(f != NULL);
    fwrite("short", 1, 5, f);
    fclose(f);
    assert(goblin_zip_reader_open(g_tmp_path) == NULL);
}

static void test_a_non_stored_method_entry_is_rejected(void)
{
    write_basic_archive();

    FILE *rw = fopen(g_tmp_path, "r+b");
    assert(rw != NULL);
    /* Patch the central directory's compression-method field for the
     * first entry (DEFLATE = 8) without touching the local header or data
     * -- goblin_zip_reader_get must refuse it purely from that metadata.
     * Re-derive the central directory offset the same way the reader does:
     * scan for the EOCD and read its cd_offset field, then the method
     * field sits 10 bytes into that first central directory record. */
    fseek(rw, 0, SEEK_END);
    long size = ftell(rw);
    unsigned char eocd[22];
    fseek(rw, size - 22, SEEK_SET);
    size_t n = fread(eocd, 1, 22, rw);
    assert(n == 22);
    uint32_t cd_offset = (uint32_t)(eocd[16] | (eocd[17] << 8) | (eocd[18] << 16) | ((uint32_t)eocd[19] << 24));

    fseek(rw, (long)cd_offset + 10, SEEK_SET);
    unsigned char method_bytes[2] = {8, 0};
    fwrite(method_bytes, 1, 2, rw);
    fclose(rw);

    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);
    size_t len = 0;
    assert(goblin_zip_reader_get(r, "mimetype", &len) == NULL);
    goblin_zip_reader_close(r);
}

static void test_entry_count_and_entry_name_enumerate_in_order(void)
{
    write_basic_archive();
    goblin_zip_reader_t *r = goblin_zip_reader_open(g_tmp_path);
    assert(r != NULL);
    assert(goblin_zip_reader_entry_count(r) == 3);
    assert(strcmp(goblin_zip_reader_entry_name(r, 0), "mimetype") == 0);
    assert(strcmp(goblin_zip_reader_entry_name(r, 1), "META-INF/container.xml") == 0);
    assert(strcmp(goblin_zip_reader_entry_name(r, 2), "empty.txt") == 0);
    assert(goblin_zip_reader_entry_name(r, 3) == NULL);
    goblin_zip_reader_close(r);
}

static bool file_contents_equal(const char *path, const char *expected, size_t expected_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    return n == expected_len && memcmp(buf, expected, expected_len) == 0;
}

static void test_extract_all_writes_files_and_nested_directories(void)
{
    const char *dest = "goblin_zip_test_extracted_basic";
    write_basic_archive();
    assert(goblin_zip_extract_all(g_tmp_path, dest));

    assert(file_contents_equal("goblin_zip_test_extracted_basic/mimetype", "application/epub+zip", 21));
    assert(file_contents_equal("goblin_zip_test_extracted_basic/META-INF/container.xml", "<x/>", 4));
    assert(file_contents_equal("goblin_zip_test_extracted_basic/empty.txt", "", 0));

    remove("goblin_zip_test_extracted_basic/mimetype");
    remove("goblin_zip_test_extracted_basic/META-INF/container.xml");
    remove("goblin_zip_test_extracted_basic/empty.txt");
    GOBLIN_TEST_RMDIR("goblin_zip_test_extracted_basic/META-INF");
    GOBLIN_TEST_RMDIR(dest);
}

static void test_extract_all_creates_directory_marker_entries(void)
{
    const char *dest = "goblin_zip_test_extracted_dirmarker";
    goblin_zip_writer_t *w = goblin_zip_writer_open(g_tmp_path);
    assert(w != NULL);
    assert(goblin_zip_writer_add(w, "assets/", NULL, 0));
    assert(goblin_zip_writer_add(w, "assets/logo.png", (const unsigned char *)"\x89PNG", 4));
    assert(goblin_zip_writer_close(w));

    assert(goblin_zip_extract_all(g_tmp_path, dest));
    assert(file_contents_equal("goblin_zip_test_extracted_dirmarker/assets/logo.png", "\x89PNG", 4));

    remove("goblin_zip_test_extracted_dirmarker/assets/logo.png");
    GOBLIN_TEST_RMDIR("goblin_zip_test_extracted_dirmarker/assets");
    GOBLIN_TEST_RMDIR(dest);
}

/* The "Zip Slip" case: an entry name that climbs out of dest_dir. This
 * writer has no name validation of its own (same as the original), so
 * building the malicious archive is just an ordinary goblin_zip_writer_add
 * call -- the refusal has to come entirely from goblin_zip_extract_all. */
static void test_extract_all_rejects_a_zip_slip_entry(void)
{
    goblin_zip_writer_t *w = goblin_zip_writer_open(g_tmp_path);
    assert(w != NULL);
    assert(goblin_zip_writer_add(w, "../goblin_zip_test_escaped.txt", (const unsigned char *)"evil", 4));
    assert(goblin_zip_writer_close(w));

    assert(!goblin_zip_extract_all(g_tmp_path, "goblin_zip_test_extracted_slip"));

    FILE *escaped = fopen("goblin_zip_test_escaped.txt", "rb");
    assert(escaped == NULL); /* must never have been written */
    GOBLIN_TEST_RMDIR("goblin_zip_test_extracted_slip");
}

static void test_extract_all_rejects_an_absolute_entry_name(void)
{
    goblin_zip_writer_t *w = goblin_zip_writer_open(g_tmp_path);
    assert(w != NULL);
#ifdef _WIN32
    assert(goblin_zip_writer_add(w, "C:/evil.txt", (const unsigned char *)"evil", 4));
#else
    assert(goblin_zip_writer_add(w, "/tmp/goblin_zip_test_evil.txt", (const unsigned char *)"evil", 4));
#endif
    assert(goblin_zip_writer_close(w));

    assert(!goblin_zip_extract_all(g_tmp_path, "goblin_zip_test_extracted_abs"));
    GOBLIN_TEST_RMDIR("goblin_zip_test_extracted_abs");
}

static void test_extract_all_fails_on_a_corrupted_entry(void)
{
    write_basic_archive();
    FILE *rw = fopen(g_tmp_path, "r+b");
    assert(rw != NULL);
    fseek(rw, 30 + 8, SEEK_SET); /* inside "mimetype"'s own data, same offset as the CRC test above */
    unsigned char bad = 'X';
    fwrite(&bad, 1, 1, rw);
    fclose(rw);

    assert(!goblin_zip_extract_all(g_tmp_path, "goblin_zip_test_extracted_corrupt"));

    remove("goblin_zip_test_extracted_corrupt/mimetype");
    GOBLIN_TEST_RMDIR("goblin_zip_test_extracted_corrupt");
}

static void test_extract_all_fails_for_a_nonexistent_zip(void)
{
    assert(!goblin_zip_extract_all("this_zip_does_not_exist.zip", "goblin_zip_test_extracted_missing"));
}

int main(void)
{
    test_round_trips_multiple_entries();
    test_missing_entry_returns_null();
    test_binary_data_with_embedded_zero_bytes_round_trips();
    test_eocd_is_found_past_a_trailing_comment();
    test_corrupted_entry_bytes_are_rejected_via_crc();
    test_opening_a_nonexistent_file_returns_null();
    test_opening_a_truncated_file_returns_null();
    test_a_non_stored_method_entry_is_rejected();
    test_entry_count_and_entry_name_enumerate_in_order();
    test_extract_all_writes_files_and_nested_directories();
    test_extract_all_creates_directory_marker_entries();
    test_extract_all_rejects_a_zip_slip_entry();
    test_extract_all_rejects_an_absolute_entry_name();
    test_extract_all_fails_on_a_corrupted_entry();
    test_extract_all_fails_for_a_nonexistent_zip();

    remove(g_tmp_path);
    puts("zip tests passed");
    return 0;
}
