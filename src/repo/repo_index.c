#define _POSIX_C_SOURCE 200809L
#include "repo.h"
#include "pkg/pkg.h"
#include "db/db.h"
#include "toml.h"
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void format_desc_toml(const NlpkgMeta *meta, const char *filename, char *buf, size_t buf_size) {
    size_t off = 0;
    off += snprintf(buf + off, buf_size - off,
                    "name = \"%s\"\n"
                    "version = \"%s\"\n"
                    "desc = \"%s\"\n"
                    "arch = \"%s\"\n"
                    "isize = %lld\n"
                    "csize = %lld\n"
                    "sha256 = \"%s\"\n"
                    "filename = \"%s\"\n\n"
                    "[deps]\n"
                    "depend = [",
                    meta->name, meta->version, meta->desc ? meta->desc : "",
                    meta->arch ? meta->arch : "x86_64",
                    (long long)meta->isize, (long long)meta->csize,
                    meta->sha256 ? meta->sha256 : "", filename);

    for (size_t i = 0; i < meta->dep_count; ++i) {
        if (meta->deps[i].constraint) {
            off += snprintf(buf + off, buf_size - off, "\"%s%s\"%s",
                            meta->deps[i].name, meta->deps[i].constraint,
                            (i + 1 < meta->dep_count) ? ", " : "");
        } else {
            off += snprintf(buf + off, buf_size - off, "\"%s\"%s",
                            meta->deps[i].name,
                            (i + 1 < meta->dep_count) ? ", " : "");
        }
    }

    off += snprintf(buf + off, buf_size - off, "]\n");
}

bool repo_create_index(const char *pkg_dir, const char *out_nldb_path) {
    if (!pkg_dir || !out_nldb_path) return false;

    DIR *d = opendir(pkg_dir);
    if (!d) return false;

    struct archive *a = archive_write_new();
    archive_write_add_filter_zstd(a);
    archive_write_set_format_pax_restricted(a);

    if (archive_write_open_filename(a, out_nldb_path) != ARCHIVE_OK) {
        archive_write_free(a);
        closedir(d);
        return false;
    }

    struct dirent *de;
    char desc_buf[8192];

    while ((de = readdir(d)) != NULL) {
        size_t len = strlen(de->d_name);
        if (len < 6 || strcmp(de->d_name + len - 6, ".nlpkg") != 0) {
            continue;
        }

        char pkg_path[1024];
        snprintf(pkg_path, sizeof(pkg_path), "%s/%s", pkg_dir, de->d_name);

        NlpkgMeta meta = {0};
        if (!pkg_read_meta(pkg_path, &meta)) {
            continue;
        }

        format_desc_toml(&meta, de->d_name, desc_buf, sizeof(desc_buf));

        char entry_path[512];
        snprintf(entry_path, sizeof(entry_path), "%s-%s/desc.toml", meta.name, meta.version);

        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, entry_path);
        archive_entry_set_size(entry, (int64_t)strlen(desc_buf));
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);

        archive_write_header(a, entry);
        archive_write_data(a, desc_buf, strlen(desc_buf));
        archive_entry_free(entry);

        pkg_meta_free(&meta);
    }

    closedir(d);
    archive_write_close(a);
    archive_write_free(a);
    return true;
}

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

static bool import_desc_toml(sqlite3 *db, int64_t repo_id, const char *content) {
    char errbuf[256];
    TomlTable *root = toml_parse(content, errbuf, sizeof(errbuf));
    if (!root) return false;

    const char *name = toml_get_string(root, "name", "");
    const char *version = toml_get_string(root, "version", "");
    const char *desc = toml_get_string(root, "desc", "");
    const char *arch = toml_get_string(root, "arch", "x86_64");
    int64_t isize = toml_get_int(root, "isize", 0);
    int64_t csize = toml_get_int(root, "csize", 0);
    const char *sha256 = toml_get_string(root, "sha256", "");
    const char *filename = toml_get_string(root, "filename", "");

    int64_t pkg_id = 0;
    if (!db_sync_pkg_insert(db, repo_id, name, version, desc, arch, isize, csize, sha256, filename, &pkg_id)) {
        toml_free(root);
        return false;
    }

    const TomlTable *deps_tbl = toml_get_table(root, "deps");
    if (deps_tbl) {
        const TomlArray *arr = toml_get_array(deps_tbl, "depend");
        if (arr) {
            for (size_t i = 0; i < arr->count; ++i) {
                if (arr->items[i].type == TOML_STRING) {
                    char *dep_name = NULL;
                    char *dep_con = NULL;
                    parse_dep_string(arr->items[i].str, &dep_name, &dep_con);
                    db_sync_dep_insert(db, pkg_id, "depend", dep_name, dep_con);
                    free(dep_name);
                    free(dep_con);
                }
            }
        }
    }

    toml_free(root);
    return true;
}

bool repo_import_index(sqlite3 *db, int64_t repo_id, const char *nldb_path) {
    if (!db || !nldb_path) return false;

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, nldb_path, 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = NULL;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(entry);
        size_t len = strlen(name);
        if (len >= 9 && strcmp(name + len - 9, "desc.toml") == 0) {
            int64_t sz = archive_entry_size(entry);
            if (sz <= 0) sz = 16384;
            char *buf = (char *)malloc(sz + 1);
            int64_t read_bytes = archive_read_data(a, buf, sz);
            if (read_bytes >= 0) {
                buf[read_bytes] = '\0';
                import_desc_toml(db, repo_id, buf);
            }
            free(buf);
        } else {
            archive_read_data_skip(a);
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    return true;
}
