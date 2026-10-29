#include "sha256.h"
#include <stdio.h>
#include <string.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)   (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)   (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256_init(Sha256Context *ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void sha256_transform(Sha256Context *ctx, const uint8_t *block) {
    uint32_t w[64];
    for (size_t i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)block[i * 4] << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] << 8)
             | ((uint32_t)block[i * 4 + 3]);
    }
    for (size_t i = 16; i < 64; ++i) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];

    for (size_t i = 0; i < 64; ++i) {
        uint32_t t1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = SIGMA0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_update(Sha256Context *ctx, const void *data, size_t len) {
    const uint8_t *input = (const uint8_t *)data;
    size_t index = (size_t)(ctx->count & 0x3f);
    ctx->count += len;

    size_t part_len = 64 - index;
    size_t i = 0;

    if (len >= part_len) {
        memcpy(&ctx->buffer[index], input, part_len);
        sha256_transform(ctx, ctx->buffer);
        for (i = part_len; i + 63 < len; i += 64) {
            sha256_transform(ctx, &input[i]);
        }
        index = 0;
    }

    memcpy(&ctx->buffer[index], &input[i], len - i);
}

void sha256_final(Sha256Context *ctx, uint8_t hash[32]) {
    uint8_t final_count[8];
    uint64_t total_bits = ctx->count * 8;
    for (int i = 0; i < 8; ++i) {
        final_count[i] = (uint8_t)((total_bits >> ((7 - i) * 8)) & 0xff);
    }

    size_t index = (size_t)(ctx->count & 0x3f);
    size_t pad_len = (index < 56) ? (56 - index) : (120 - index);

    static const uint8_t padding[64] = {0x80};
    sha256_update(ctx, padding, pad_len);
    sha256_update(ctx, final_count, 8);

    for (int i = 0; i < 8; ++i) {
        hash[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xff);
        hash[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xff);
        hash[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xff);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xff);
    }
}

void sha256_to_hex(const uint8_t hash[32], char hex[65]) {
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        hex[i * 2]     = hex_chars[(hash[i] >> 4) & 0x0f];
        hex[i * 2 + 1] = hex_chars[hash[i] & 0x0f];
    }
    hex[64] = '\0';
}

void sha256_buffer(const void *data, size_t len, char hex[65]) {
    Sha256Context ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
    sha256_to_hex(hash, hex);
}

bool sha256_file(const char *path, char hex[65]) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    Sha256Context ctx;
    sha256_init(&ctx);

    uint8_t buf[16384];
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0) {
        sha256_update(&ctx, buf, bytes_read);
    }

    bool success = !ferror(fp);
    fclose(fp);

    if (!success) return false;

    uint8_t hash[32];
    sha256_final(&ctx, hash);
    sha256_to_hex(hash, hex);
    return true;
}
