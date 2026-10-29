#ifndef PKG_H
#define PKG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sqlite3.h>

typedef struct {
    char *name;
    char *constraint;
} NlpkgDep;

typedef struct {
    char *name;
    char *version;
    char *base;
    char *desc;
    char *url;
    char *arch;
    char **licenses;
    size_t license_count;
    int64_t isize;
    int64_t csize;
    char *sha256;
    int64_t build_date;
    char *packager;

    NlpkgDep *deps;
    size_t dep_count;

    NlpkgDep *conflicts;
    size_t conflict_count;

    char **provides;
    size_t provide_count;
} NlpkgMeta;

typedef struct {
    char *path;
    int64_t size;
    mode_t mode;
} NlpkgFileEntry;

bool pkg_read_meta(const char *pkg_path, NlpkgMeta *out_meta);
bool pkg_read_file_list(const char *pkg_path, NlpkgFileEntry **out_files, size_t *out_count);
bool pkg_build_archive(const char *manifest_toml_path, const char *data_root_dir, const char *out_nlpkg_path);

bool pkg_install(sqlite3 *local_db, const char *pkg_path, const char *root_dir, const char *reason, char *err_buf, size_t err_size);
bool pkg_remove(sqlite3 *local_db, const char *pkg_name, const char *root_dir, char *err_buf, size_t err_size);
bool pkg_upgrade(sqlite3 *local_db, const char *pkg_path, const char *root_dir, char *err_buf, size_t err_size);

void pkg_meta_free(NlpkgMeta *meta);
void pkg_file_entries_free(NlpkgFileEntry *entries, size_t count);

#endif
