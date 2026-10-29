#define _POSIX_C_SOURCE 200809L
#include "version.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char *parse_u32(const char *p, uint32_t *out) {
    if (!p || !isdigit((unsigned char)*p)) return NULL;
    uint64_t val = 0;
    while (*p && isdigit((unsigned char)*p)) {
        val = val * 10 + (*p - '0');
        if (val > UINT32_MAX) return NULL;
        p++;
    }
    *out = (uint32_t)val;
    return p;
}

bool version_parse(const char *str, Version *out_ver) {
    if (!str || !out_ver) return false;
    memset(out_ver, 0, sizeof(*out_ver));

    const char *p = str;
    const char *colon = strchr(p, ':');
    if (colon) {
        p = parse_u32(p, &out_ver->epoch);
        if (!p || p != colon) {
            return false;
        }
        p = colon + 1;
    }

    p = parse_u32(p, &out_ver->major);
    if (!p) return false;

    if (*p == '.') {
        p++;
        p = parse_u32(p, &out_ver->minor);
        if (!p) return false;

        if (*p == '.') {
            p++;
            p = parse_u32(p, &out_ver->patch);
            if (!p) return false;
        }
    }

    if (*p == '-') {
        p++;
        out_ver->prerelease = strdup(p);
        if (!out_ver->prerelease) return false;
    } else if (*p != '\0') {
        return false;
    }

    return true;
}

void version_free(Version *ver) {
    if (ver) {
        free(ver->prerelease);
        ver->prerelease = NULL;
    }
}

int version_struct_cmp(const Version *a, const Version *b) {
    if (a->epoch != b->epoch) {
        return (a->epoch > b->epoch) ? 1 : -1;
    }
    if (a->major != b->major) {
        return (a->major > b->major) ? 1 : -1;
    }
    if (a->minor != b->minor) {
        return (a->minor > b->minor) ? 1 : -1;
    }
    if (a->patch != b->patch) {
        return (a->patch > b->patch) ? 1 : -1;
    }

    if (!a->prerelease && !b->prerelease) return 0;
    if (a->prerelease && !b->prerelease) return -1;
    if (!a->prerelease && b->prerelease) return 1;

    return strcmp(a->prerelease, b->prerelease);
}

int version_cmp(const char *ver_a, const char *ver_b) {
    Version va = {0};
    Version vb = {0};

    if (!version_parse(ver_a, &va)) return -1;
    if (!version_parse(ver_b, &vb)) {
        version_free(&va);
        return 1;
    }

    int res = version_struct_cmp(&va, &vb);
    version_free(&va);
    version_free(&vb);
    return res;
}

bool version_parse_constraint(const char *str, VersionConstraint *out_con) {
    if (!str || !out_con) return false;
    memset(out_con, 0, sizeof(*out_con));

    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0' || strcmp(str, "*") == 0) {
        out_con->op = OP_ANY;
        return true;
    }

    if (strncmp(str, ">=", 2) == 0) {
        out_con->op = OP_GE;
        str += 2;
    } else if (strncmp(str, "<=", 2) == 0) {
        out_con->op = OP_LE;
        str += 2;
    } else if (strncmp(str, "!=", 2) == 0) {
        out_con->op = OP_NE;
        str += 2;
    } else if (strncmp(str, "==", 2) == 0) {
        out_con->op = OP_EQ;
        str += 2;
    } else if (*str == '=') {
        out_con->op = OP_EQ;
        str++;
    } else if (*str == '>') {
        out_con->op = OP_GT;
        str++;
    } else if (*str == '<') {
        out_con->op = OP_LT;
        str++;
    } else {
        out_con->op = OP_EQ;
    }

    while (isspace((unsigned char)*str)) str++;
    return version_parse(str, &out_con->version);
}

void version_constraint_free(VersionConstraint *con) {
    if (con) {
        version_free(&con->version);
    }
}

bool version_satisfies(const Version *ver, const VersionConstraint *con) {
    if (!ver || !con) return false;
    if (con->op == OP_ANY) return true;

    int cmp = version_struct_cmp(ver, &con->version);
    switch (con->op) {
        case OP_EQ: return cmp == 0;
        case OP_NE: return cmp != 0;
        case OP_LT: return cmp < 0;
        case OP_LE: return cmp <= 0;
        case OP_GT: return cmp > 0;
        case OP_GE: return cmp >= 0;
        default: return false;
    }
}

bool version_satisfies_str(const char *ver_str, const char *con_str) {
    if (!ver_str) return false;
    if (!con_str || *con_str == '\0' || strcmp(con_str, "*") == 0) return true;

    Version ver = {0};
    if (!version_parse(ver_str, &ver)) return false;

    VersionConstraint con = {0};
    if (!version_parse_constraint(con_str, &con)) {
        version_free(&ver);
        return false;
    }

    bool result = version_satisfies(&ver, &con);
    version_free(&ver);
    version_constraint_free(&con);
    return result;
}
