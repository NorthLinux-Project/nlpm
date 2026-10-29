#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} Sha256Context;

void sha256_init(Sha256Context *ctx);
void sha256_update(Sha256Context *ctx, const void *data, size_t len);
void sha256_final(Sha256Context *ctx, uint8_t hash[32]);

void sha256_to_hex(const uint8_t hash[32], char hex[65]);
void sha256_buffer(const void *data, size_t len, char hex[65]);
bool sha256_file(const char *path, char hex[65]);

#endif
