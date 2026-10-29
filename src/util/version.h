#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t epoch;
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    char *prerelease;
} Version;

typedef enum {
    OP_ANY = 0,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE
} VersionOp;

typedef struct {
    VersionOp op;
    Version version;
} VersionConstraint;

bool version_parse(const char *str, Version *out_ver);
void version_free(Version *ver);
int version_cmp(const char *ver_a, const char *ver_b);
int version_struct_cmp(const Version *a, const Version *b);

bool version_parse_constraint(const char *str, VersionConstraint *out_con);
void version_constraint_free(VersionConstraint *con);
bool version_satisfies(const Version *ver, const VersionConstraint *con);
bool version_satisfies_str(const char *ver_str, const char *con_str);

#endif
