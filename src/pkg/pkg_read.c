#define _POSIX_C_SOURCE 200809L
#include "pkg.h"
#include "crypto/sha256.h"
#include "toml.h"
#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void parse_dep_string(const char *raw, char **out_name, char **out_con) {
    const char *p = raw;
    while (*p && *p != '>' && *p != '<' && *p != '=' && *p != '!') {
        p++;
    }

    size_t name_len = (size_t)(p - raw);
    char *name = (char *)malloc(name_len + 1);
    memcpy(name, raw, name_len);
    name[name_len] = '\0';
    *out_name = name;

    if (*p != '\0') {
        *out_con = strdup(p);
    } else {
        *out_con = NULL;
    }
}

static bool parse_pkginfo_content(const char *content, NlpkgMeta *out_meta) {
    char errbuf[256];
    TomlTable *root = toml_parse(content, errbuf, sizeof(errbuf));
    if (!root) return false;

    memset(out_meta, 0, sizeof(*out_meta));
    out_meta->name = strdup(toml_get_string(root, "name", ""));
    out_meta->version = strdup(toml_get_string(root, "version", ""));
    out_meta->base = strdup(toml_get_string(root, "base", out_meta->name));
    out_meta->desc = strdup(toml_get_string(root, "desc", ""));
    out_meta->url = strdup(toml_get_string(root, "url", ""));
    out_meta->arch = strdup(toml_get_string(root, "arch", "x86_64"));
    out_meta->isize = toml_get_int(root, "isize", 0);

    const TomlTable *build = toml_get_table(root, "build");
    if (build) {
        out_meta->build_date = toml_get_int(build, "date", 0);
        out_meta->packager = strdup(toml_get_string(build, "packager", ""));
    }

    const TomlTable *deps_tbl = toml_get_table(root, "deps");
    if (deps_tbl) {
        const TomlArray *arr = toml_get_array(deps_tbl, "depend");
        if (arr && arr->count > 0) {
            out_meta->deps = (NlpkgDep *)calloc(arr->count, sizeof(NlpkgDep));
            out_meta->dep_count = arr->count;
            for (size_t i = 0; i < arr->count; ++i) {
                if (arr->items[i].type == TOML_STRING) {
                    parse_dep_string(arr->items[i].str, &out_meta->deps[i].name, &out_meta->deps[i].constraint);
                }
            }
        }

        const TomlArray *conflicts = toml_get_array(deps_tbl, "conflict");
        if (conflicts && conflicts->count > 0) {
            out_meta->conflicts = (NlpkgDep *)calloc(conflicts->count, sizeof(NlpkgDep));
            out_meta->conflict_count = conflicts->count;
            for (size_t i = 0; i < conflicts->count; ++i) {
                if (conflicts->items[i].type == TOML_STRING) {
                    parse_dep_string(conflicts->items[i].str, &out_meta->conflicts[i].name, &out_meta->conflicts[i].constraint);
                }
            }
        }

        const TomlArray *provides = toml_get_array(deps_tbl, "provides");
        if (provides && provides->count > 0) {
            out_meta->provides = (char **)calloc(provides->count, sizeof(char *));
            out_meta->provide_count = provides->count;
            for (size_t i = 0; i < provides->count; ++i) {
                if (provides->items[i].type == TOML_STRING) {
                    out_meta->provides[i] = strdup(provides->items[i].str);
                }
            }
        }
    }

    toml_free(root);
    return true;
}

bool pkg_read_meta(const char *pkg_path, NlpkgMeta *out_meta) {
    if (!pkg_path || !out_meta) return false;

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, pkg_path, 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = NULL;
    char *pkginfo_buf = NULL;
    bool found_pkginfo = false;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(entry);
        if (strcmp(name, ".PKGINFO") == 0) {
            int64_t sz = archive_entry_size(entry);
            if (sz <= 0) sz = 65536;
            pkginfo_buf = (char *)malloc(sz + 1);
            int64_t bytes = archive_read_data(a, pkginfo_buf, sz);
            if (bytes >= 0) {
                pkginfo_buf[bytes] = '\0';
                found_pkginfo = true;
            }
            break;
        } else {
            archive_read_data_skip(a);
        }
    }

    archive_read_close(a);
    archive_read_free(a);

    if (!found_pkginfo || !pkginfo_buf) {
        free(pkginfo_buf);
        return false;
    }

    bool parsed = parse_pkginfo_content(pkginfo_buf, out_meta);
    free(pkginfo_buf);

    if (!parsed) return false;

    struct stat st;
    if (stat(pkg_path, &st) == 0) {
        out_meta->csize = (int64_t)st.st_size;
    }

    char hex[65];
    if (sha256_file(pkg_path, hex)) {
        out_meta->sha256 = strdup(hex);
    }

    return true;
}

bool pkg_read_file_list(const char *pkg_path, NlpkgFileEntry **out_files, size_t *out_count) {
    if (!pkg_path || !out_files || !out_count) return false;
    *out_files = NULL;
    *out_count = 0;

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, pkg_path, 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        return false;
    }

    size_t cap = 32;
    size_t count = 0;
    NlpkgFileEntry *entries = (NlpkgFileEntry *)malloc(cap * sizeof(NlpkgFileEntry));

    struct archive_entry *entry = NULL;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);
        if (strncmp(pathname, "data/", 5) == 0 && pathname[5] != '\0') {
            const char *rel_path = pathname + 5;
            if (count >= cap) {
                cap *= 2;
                entries = (NlpkgFileEntry *)realloc(entries, cap * sizeof(NlpkgFileEntry));
            }
            entries[count].path = strdup(rel_path);
            entries[count].size = archive_entry_size(entry);
            entries[count].mode = archive_entry_mode(entry);
            count++;
        }
        archive_read_data_skip(a);
    }

    archive_read_close(a);
    archive_read_free(a);

    *out_files = entries;
    *out_count = count;
    return true;
}

void pkg_meta_free(NlpkgMeta *meta) {
    if (!meta) return;
    free(meta->name);
    free(meta->version);
    free(meta->base);
    free(meta->desc);
    free(meta->url);
    free(meta->arch);
    free(meta->sha256);
    free(meta->packager);

    if (meta->licenses) {
        for (size_t i = 0; i < meta->license_count; ++i) {
            free(meta->licenses[i]);
        }
        free(meta->licenses);
    }

    if (meta->deps) {
        for (size_t i = 0; i < meta->dep_count; ++i) {
            free(meta->deps[i].name);
            free(meta->deps[i].constraint);
        }
        free(meta->deps);
    }

    if (meta->conflicts) {
        for (size_t i = 0; i < meta->conflict_count; ++i) {
            free(meta->conflicts[i].name);
            free(meta->conflicts[i].constraint);
        }
        free(meta->conflicts);
    }

    if (meta->provides) {
        for (size_t i = 0; i < meta->provide_count; ++i) {
            free(meta->provides[i]);
        }
        free(meta->provides);
    }

    memset(meta, 0, sizeof(*meta));
}

void pkg_file_entries_free(NlpkgFileEntry *entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; ++i) {
        free(entries[i].path);
    }
    free(entries);
}
