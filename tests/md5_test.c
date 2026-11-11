#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GOBLIN_MD5_IMPLEMENTATION
#include "../include/goblin/md5.h"

static void test_one_shot_empty_input(void)
{
    static const unsigned char expected[GOBLIN_MD5_DIGEST_LEN] = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e,
    };
    unsigned char digest[GOBLIN_MD5_DIGEST_LEN];

    goblin_md5("", 0, digest);
    assert(memcmp(digest, expected, sizeof(expected)) == 0);
}

static void test_one_shot_known_value_abc(void)
{
    static const unsigned char expected[GOBLIN_MD5_DIGEST_LEN] = {
        0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
    };
    unsigned char digest[GOBLIN_MD5_DIGEST_LEN];

    goblin_md5("abc", 3, digest);
    assert(memcmp(digest, expected, sizeof(expected)) == 0);
}

static void test_one_shot_known_value_sentence(void)
{
    static const unsigned char expected[GOBLIN_MD5_DIGEST_LEN] = {
        0x9e, 0x10, 0x7d, 0x9d, 0x37, 0x2b, 0xb6, 0x82, 0x6b, 0xd8, 0x1d, 0x35, 0x42, 0xa4, 0x19, 0xd6,
    };
    const char *text = "The quick brown fox jumps over the lazy dog";
    unsigned char digest[GOBLIN_MD5_DIGEST_LEN];

    goblin_md5(text, strlen(text), digest);
    assert(memcmp(digest, expected, sizeof(expected)) == 0);
}

static void test_ctx_matches_one_shot(void)
{
    const char *text = "The quick brown fox jumps over the lazy dog";
    unsigned char one_shot[GOBLIN_MD5_DIGEST_LEN];
    unsigned char via_ctx[GOBLIN_MD5_DIGEST_LEN];
    goblin_md5_ctx ctx;

    goblin_md5(text, strlen(text), one_shot);

    goblin_md5_init(&ctx);
    goblin_md5_update(&ctx, text, strlen(text));
    goblin_md5_final(&ctx, via_ctx);

    assert(memcmp(one_shot, via_ctx, sizeof(one_shot)) == 0);
}

static void test_streaming_in_small_chunks_matches_one_shot(void)
{
    unsigned char data[200];
    for (size_t i = 0; i < sizeof(data); i++) data[i] = (unsigned char)(i * 7 + 1);

    unsigned char expected[GOBLIN_MD5_DIGEST_LEN];
    goblin_md5(data, sizeof(data), expected);

    unsigned char actual[GOBLIN_MD5_DIGEST_LEN];
    goblin_md5_ctx ctx;
    goblin_md5_init(&ctx);
    for (size_t i = 0; i < sizeof(data); i += 3) {
        size_t take = sizeof(data) - i < 3 ? sizeof(data) - i : 3;
        goblin_md5_update(&ctx, data + i, take);
    }
    goblin_md5_final(&ctx, actual);

    assert(memcmp(expected, actual, sizeof(expected)) == 0);
}

static void test_message_length_exactly_one_block(void)
{
    /* 64 bytes: padding spills into a second block. */
    unsigned char data[64];
    for (size_t i = 0; i < sizeof(data); i++) data[i] = (unsigned char)i;

    unsigned char one_shot[GOBLIN_MD5_DIGEST_LEN];
    unsigned char via_ctx[GOBLIN_MD5_DIGEST_LEN];
    goblin_md5_ctx ctx;

    goblin_md5(data, sizeof(data), one_shot);

    goblin_md5_init(&ctx);
    goblin_md5_update(&ctx, data, 40);
    goblin_md5_update(&ctx, data + 40, 24);
    goblin_md5_final(&ctx, via_ctx);

    assert(memcmp(one_shot, via_ctx, sizeof(one_shot)) == 0);
}

static void test_message_length_fifty_six_bytes(void)
{
    /* 56 bytes: the 0x80 pad byte alone already fills the last block. */
    unsigned char data[56];
    for (size_t i = 0; i < sizeof(data); i++) data[i] = (unsigned char)(i * 3);

    unsigned char digest_a[GOBLIN_MD5_DIGEST_LEN];
    unsigned char digest_b[GOBLIN_MD5_DIGEST_LEN];

    goblin_md5(data, sizeof(data), digest_a);
    goblin_md5(data, sizeof(data), digest_b);

    assert(memcmp(digest_a, digest_b, sizeof(digest_a)) == 0);
}

static void test_update_null_args_do_nothing(void)
{
    goblin_md5_ctx ctx;
    unsigned char before[GOBLIN_MD5_DIGEST_LEN];
    unsigned char after[GOBLIN_MD5_DIGEST_LEN];

    goblin_md5_init(&ctx);
    goblin_md5_update(&ctx, "abc", 3);
    goblin_md5_final(&ctx, before);

    goblin_md5_init(&ctx);
    goblin_md5_update(&ctx, "abc", 3);
    goblin_md5_update(NULL, "abc", 3); /* must not crash */
    goblin_md5_update(&ctx, NULL, 3);  /* must not crash, no-op */
    goblin_md5_final(&ctx, after);

    assert(memcmp(before, after, sizeof(before)) == 0);
}

static void test_final_null_args_do_nothing(void)
{
    goblin_md5_ctx ctx;
    goblin_md5_init(&ctx);
    goblin_md5_final(NULL, NULL); /* must not crash */
    goblin_md5_final(&ctx, NULL); /* must not crash */
}

int main(void)
{
    test_one_shot_empty_input();
    test_one_shot_known_value_abc();
    test_one_shot_known_value_sentence();
    test_ctx_matches_one_shot();
    test_streaming_in_small_chunks_matches_one_shot();
    test_message_length_exactly_one_block();
    test_message_length_fifty_six_bytes();
    test_update_null_args_do_nothing();
    test_final_null_args_do_nothing();

    puts("md5 tests passed");
    return 0;
}
