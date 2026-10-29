#define _POSIX_C_SOURCE 200809L
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *SYNC_SCHEMA =
    "PRAGMA foreign_keys = ON;"
    "PRAGMA journal_mode = WAL;"
    "CREATE TABLE IF NOT EXISTS repos ("
    "    id INTEGER PRIMARY KEY,"
    "    name TEXT NOT NULL UNIQUE,"
    "    url TEXT NOT NULL,"
    "    enabled INTEGER NOT NULL DEFAULT 1,"
    "    last_sync INTEGER"
    ");"
    "CREATE TABLE IF NOT EXISTS packages ("
    "    id INTEGER PRIMARY KEY,"
    "    repo_id INTEGER NOT NULL REFERENCES repos(id) ON DELETE CASCADE,"
    "    name TEXT NOT NULL,"
    "    version TEXT NOT NULL,"
    "    desc TEXT,"
    "    arch TEXT NOT NULL,"
    "    isize INTEGER,"
    "    csize INTEGER,"
    "    sha256 TEXT,"
    "    filename TEXT,"
    "    UNIQUE(repo_id, name)"
    ");"
    "CREATE TABLE IF NOT EXISTS deps ("
    "    id INTEGER PRIMARY KEY,"
    "    pkg_id INTEGER NOT NULL REFERENCES packages(id) ON DELETE CASCADE,"
    "    kind TEXT NOT NULL,"
    "    name TEXT NOT NULL,"
    "    constraint_str TEXT"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_packages_name ON packages(name);"
    "CREATE INDEX IF NOT EXISTS idx_deps_pkg ON deps(pkg_id);"
    "CREATE INDEX IF NOT EXISTS idx_deps_name ON deps(name);";

bool db_sync_open(const char *path, sqlite3 **out_db) {
    if (!path || !out_db) return false;
    sqlite3 *db = NULL;
    int rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    char *errmsg = NULL;
    rc = sqlite3_exec(db, SYNC_SCHEMA, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return false;
    }

    *out_db = db;
    return true;
}

bool db_sync_repo_upsert(sqlite3 *db, const char *name, const char *url, int64_t *out_repo_id) {
    if (!db || !name || !url) return false;
    const char *sql =
        "INSERT INTO repos (name, url, last_sync) VALUES (?, ?, ?)"
        " ON CONFLICT(name) DO UPDATE SET url = excluded.url, last_sync = excluded.last_sync"
        " RETURNING id;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    int64_t now = (int64_t)time(NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, url, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out_repo_id) {
            *out_repo_id = sqlite3_column_int64(stmt, 0);
        }
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool db_sync_pkg_insert(sqlite3 *db, int64_t repo_id, const char *name, const char *version,
                        const char *desc, const char *arch, int64_t isize, int64_t csize,
                        const char *sha256, const char *filename, int64_t *out_pkg_id) {
    if (!db || !name || !version) return false;
    const char *sql =
        "INSERT INTO packages (repo_id, name, version, desc, arch, isize, csize, sha256, filename) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(repo_id, name) DO UPDATE SET "
        "version = excluded.version, desc = excluded.desc, arch = excluded.arch, "
        "isize = excluded.isize, csize = excluded.csize, sha256 = excluded.sha256, filename = excluded.filename "
        "RETURNING id;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, repo_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, version, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, desc ? desc : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, arch ? arch : "x86_64", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, isize);
    sqlite3_bind_int64(stmt, 7, csize);
    sqlite3_bind_text(stmt, 8, sha256 ? sha256 : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, filename ? filename : "", -1, SQLITE_TRANSIENT);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t pkg_id = sqlite3_column_int64(stmt, 0);
        if (out_pkg_id) *out_pkg_id = pkg_id;
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool db_sync_dep_insert(sqlite3 *db, int64_t pkg_id, const char *kind, const char *name, const char *constraint) {
    if (!db || !kind || !name) return false;
    const char *sql = "INSERT INTO deps (pkg_id, kind, name, constraint_str) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, pkg_id);
    sqlite3_bind_text(stmt, 2, kind, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name, -1, SQLITE_TRANSIENT);
    if (constraint) {
        sqlite3_bind_text(stmt, 4, constraint, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

static void fill_pkg_record(sqlite3_stmt *stmt, DbPkgRecord *rec) {
    rec->id = sqlite3_column_int64(stmt, 0);
    rec->repo_id = sqlite3_column_int64(stmt, 1);
    rec->name = strdup((const char *)sqlite3_column_text(stmt, 2));
    rec->version = strdup((const char *)sqlite3_column_text(stmt, 3));
    rec->desc = strdup((const char *)sqlite3_column_text(stmt, 4));
    rec->arch = strdup((const char *)sqlite3_column_text(stmt, 5));
    rec->isize = sqlite3_column_int64(stmt, 6);
    rec->csize = sqlite3_column_int64(stmt, 7);
    rec->sha256 = strdup((const char *)sqlite3_column_text(stmt, 8));
    rec->filename = strdup((const char *)sqlite3_column_text(stmt, 9));
}

bool db_sync_find_pkg(sqlite3 *db, const char *name, DbPkgRecord *out_pkg) {
    if (!db || !name || !out_pkg) return false;
    memset(out_pkg, 0, sizeof(*out_pkg));

    const char *sql =
        "SELECT id, repo_id, name, version, desc, arch, isize, csize, sha256, filename "
        "FROM packages WHERE name = ? ORDER BY id DESC LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        fill_pkg_record(stmt, out_pkg);
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool db_sync_search(sqlite3 *db, const char *pattern, DbPkgRecord **out_pkgs, size_t *out_count) {
    if (!db || !pattern || !out_pkgs || !out_count) return false;
    *out_pkgs = NULL;
    *out_count = 0;

    const char *sql =
        "SELECT id, repo_id, name, version, desc, arch, isize, csize, sha256, filename "
        "FROM packages WHERE name LIKE ? OR desc LIKE ? ORDER BY name ASC;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    char query[256];
    snprintf(query, sizeof(query), "%%%s%%", pattern);
    sqlite3_bind_text(stmt, 1, query, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, query, -1, SQLITE_TRANSIENT);

    size_t cap = 8;
    size_t count = 0;
    DbPkgRecord *records = (DbPkgRecord *)malloc(cap * sizeof(DbPkgRecord));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= cap) {
            cap *= 2;
            records = (DbPkgRecord *)realloc(records, cap * sizeof(DbPkgRecord));
        }
        fill_pkg_record(stmt, &records[count++]);
    }

    sqlite3_finalize(stmt);
    *out_pkgs = records;
    *out_count = count;
    return true;
}

bool db_sync_get_deps(sqlite3 *db, int64_t pkg_id, DbDepRecord **out_deps, size_t *out_count) {
    if (!db || !out_deps || !out_count) return false;
    *out_deps = NULL;
    *out_count = 0;

    const char *sql = "SELECT id, pkg_id, kind, name, constraint_str FROM deps WHERE pkg_id = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, pkg_id);

    size_t cap = 8;
    size_t count = 0;
    DbDepRecord *deps = (DbDepRecord *)malloc(cap * sizeof(DbDepRecord));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= cap) {
            cap *= 2;
            deps = (DbDepRecord *)realloc(deps, cap * sizeof(DbDepRecord));
        }
        deps[count].id = sqlite3_column_int64(stmt, 0);
        deps[count].pkg_id = sqlite3_column_int64(stmt, 1);
        deps[count].kind = strdup((const char *)sqlite3_column_text(stmt, 2));
        deps[count].name = strdup((const char *)sqlite3_column_text(stmt, 3));
        const unsigned char *c = sqlite3_column_text(stmt, 4);
        deps[count].constraint = c ? strdup((const char *)c) : NULL;
        count++;
    }

    sqlite3_finalize(stmt);
    *out_deps = deps;
    *out_count = count;
    return true;
}

void db_sync_pkg_free(DbPkgRecord *pkg) {
    if (!pkg) return;
    free(pkg->name);
    free(pkg->version);
    free(pkg->desc);
    free(pkg->arch);
    free(pkg->sha256);
    free(pkg->filename);
    memset(pkg, 0, sizeof(*pkg));
}

void db_sync_pkgs_free(DbPkgRecord *pkgs, size_t count) {
    if (!pkgs) return;
    for (size_t i = 0; i < count; ++i) {
        db_sync_pkg_free(&pkgs[i]);
    }
    free(pkgs);
}

void db_sync_deps_free(DbDepRecord *deps, size_t count) {
    if (!deps) return;
    for (size_t i = 0; i < count; ++i) {
        free(deps[i].kind);
        free(deps[i].name);
        free(deps[i].constraint);
    }
    free(deps);
}
