#ifndef DB_H
#define DB_H

#include <stdint.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "error.h"

typedef struct {
    int64_t id;
    int64_t repo_id;
    char *name;
    char *version;
    char *desc;
    char *arch;
    int64_t isize;
    int64_t csize;
    char *sha256;
    char *filename;
} DbPkgRecord;

typedef struct {
    int64_t id;
    int64_t pkg_id;
    char *kind;
    char *name;
    char *constraint;
} DbDepRecord;

typedef struct {
    int64_t id;
    char *name;
    char *version;
    char *reason;
    int64_t install_date;
} DbInstalledRecord;

bool db_sync_open(const char *path, sqlite3 **out_db);
bool db_sync_repo_upsert(sqlite3 *db, const char *name, const char *url, int64_t *out_repo_id);
bool db_sync_pkg_insert(sqlite3 *db, int64_t repo_id, const char *name, const char *version,
                        const char *desc, const char *arch, int64_t isize, int64_t csize,
                        const char *sha256, const char *filename, int64_t *out_pkg_id);
bool db_sync_dep_insert(sqlite3 *db, int64_t pkg_id, const char *kind, const char *name, const char *constraint);
bool db_sync_find_pkg(sqlite3 *db, const char *name, DbPkgRecord *out_pkg);
bool db_sync_search(sqlite3 *db, const char *pattern, DbPkgRecord **out_pkgs, size_t *out_count);
bool db_sync_get_deps(sqlite3 *db, int64_t pkg_id, DbDepRecord **out_deps, size_t *out_count);
void db_sync_pkg_free(DbPkgRecord *pkg);
void db_sync_pkgs_free(DbPkgRecord *pkgs, size_t count);
void db_sync_deps_free(DbDepRecord *deps, size_t count);

bool db_local_open(const char *path, sqlite3 **out_db);
bool db_local_install(sqlite3 *db, const char *name, const char *version, const char *reason);
bool db_local_remove(sqlite3 *db, const char *name);
bool db_local_is_installed(sqlite3 *db, const char *name, char **out_version);
bool db_local_add_file(sqlite3 *db, const char *pkg_name, const char *path);
bool db_local_get_files(sqlite3 *db, const char *pkg_name, char ***out_paths, size_t *out_count);
bool db_local_find_file_owner(sqlite3 *db, const char *path, char **out_pkg_name);
bool db_local_list_installed(sqlite3 *db, DbInstalledRecord **out_records, size_t *out_count);
void db_local_records_free(DbInstalledRecord *records, size_t count);
void db_local_files_free(char **paths, size_t count);

#endif
