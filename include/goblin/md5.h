/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    md5.h — RFC 1321 MD5 digest. Not cryptographically secure (collisions
    are practical to construct) -- use it for checksums/fingerprints, not
    anywhere an adversary gets to pick the input.
*/

#ifndef GOBLIN_MD5_H
#define GOBLIN_MD5_H

#include <stddef.h>
#include <stdint.h>

#define GOBLIN_MD5_DIGEST_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct goblin_md5_ctx {
    uint32_t state[4];
    uint64_t total_len;
    unsigned char buffer[64];
    size_t buffer_len;
} goblin_md5_ctx;

void goblin_md5_init(goblin_md5_ctx *ctx);
void goblin_md5_update(goblin_md5_ctx *ctx, const void *data, size_t len);

/*
    digest must be at least GOBLIN_MD5_DIGEST_LEN bytes. ctx is left
    finalized -- do not call update/final on it again without re-init.
*/
void goblin_md5_final(goblin_md5_ctx *ctx, unsigned char *digest);

/*
    One-shot helper equivalent to init+update+final.
*/
void goblin_md5(const void *data, size_t len, unsigned char *digest);

#ifdef __cplusplus
}
namespace goblin {
    typedef goblin_md5_ctx md5_ctx;
    static inline void md5_init(md5_ctx *ctx) { goblin_md5_init(ctx); }
    static inline void md5_update(md5_ctx *ctx, const void *data, size_t len) { goblin_md5_update(ctx, data, len); }
    static inline void md5_final(md5_ctx *ctx, unsigned char *digest) { goblin_md5_final(ctx, digest); }
    static inline void md5(const void *data, size_t len, unsigned char *digest) { goblin_md5(data, len, digest); }
}
#endif

#ifdef GOBLIN_MD5_IMPLEMENTATION

#include <string.h>

static void goblin_md5_process_block(uint32_t state[4], const unsigned char block[64])
{
    static const uint32_t k[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };
    static const unsigned s[64] = {
        7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
        5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
        4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
        6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
    };

    uint32_t m[16];
    for (int i = 0; i < 16; i++) {
        m[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) | ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];

    for (int i = 0; i < 64; i++) {
        uint32_t f;
        unsigned g;

        if (i < 16) {
            f = (b & c) | (~b & d);
            g = (unsigned)i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5u * (unsigned)i + 1u) % 16u;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3u * (unsigned)i + 5u) % 16u;
        } else {
            f = c ^ (b | ~d);
            g = (7u * (unsigned)i) % 16u;
        }

        f += a + k[i] + m[g];
        a = d;
        d = c;
        c = b;
        b += (f << s[i]) | (f >> (32 - s[i]));
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void goblin_md5_init(goblin_md5_ctx *ctx)
{
    if (!ctx) return;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->total_len = 0;
    ctx->buffer_len = 0;
}

void goblin_md5_update(goblin_md5_ctx *ctx, const void *data, size_t len)
{
    if (!ctx || !data) return;

    const unsigned char *p = (const unsigned char *)data;
    ctx->total_len += len;

    while (len > 0) {
        size_t take = 64 - ctx->buffer_len;
        if (take > len) take = len;

        memcpy(ctx->buffer + ctx->buffer_len, p, take);
        ctx->buffer_len += take;
        p += take;
        len -= take;

        if (ctx->buffer_len == 64) {
            goblin_md5_process_block(ctx->state, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }
}

void goblin_md5_final(goblin_md5_ctx *ctx, unsigned char *digest)
{
    if (!ctx || !digest) return;

    uint64_t bit_len = ctx->total_len * 8;
    size_t pad_index = ctx->buffer_len;

    ctx->buffer[pad_index++] = 0x80;

    if (pad_index > 56) {
        while (pad_index < 64) ctx->buffer[pad_index++] = 0;
        goblin_md5_process_block(ctx->state, ctx->buffer);
        pad_index = 0;
    }

    while (pad_index < 56) ctx->buffer[pad_index++] = 0;

    for (int i = 0; i < 8; i++) {
        ctx->buffer[56 + i] = (unsigned char)(bit_len >> (8 * i));
    }

    goblin_md5_process_block(ctx->state, ctx->buffer);

    for (int i = 0; i < 4; i++) {
        digest[i * 4] = (unsigned char)(ctx->state[i]);
        digest[i * 4 + 1] = (unsigned char)(ctx->state[i] >> 8);
        digest[i * 4 + 2] = (unsigned char)(ctx->state[i] >> 16);
        digest[i * 4 + 3] = (unsigned char)(ctx->state[i] >> 24);
    }
}

void goblin_md5(const void *data, size_t len, unsigned char *digest)
{
    goblin_md5_ctx ctx;
    goblin_md5_init(&ctx);
    goblin_md5_update(&ctx, data, len);
    goblin_md5_final(&ctx, digest);
}

#endif /* GOBLIN_MD5_IMPLEMENTATION */

#endif /* GOBLIN_MD5_H */
