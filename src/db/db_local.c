#define _POSIX_C_SOURCE 200809L
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *LOCAL_SCHEMA =
    "PRAGMA foreign_keys = ON;"
    "PRAGMA journal_mode = WAL;"
    "CREATE TABLE IF NOT EXISTS installed ("
    "    id INTEGER PRIMARY KEY,"
    "    name TEXT NOT NULL UNIQUE,"
    "    version TEXT NOT NULL,"
    "    reason TEXT NOT NULL DEFAULT 'explicit',"
    "    install_date INTEGER"
    ");"
    "CREATE TABLE IF NOT EXISTS files ("
    "    id INTEGER PRIMARY KEY,"
    "    pkg_name TEXT NOT NULL REFERENCES installed(name) ON DELETE CASCADE,"
    "    path TEXT NOT NULL UNIQUE"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);";

bool db_local_open(const char *path, sqlite3 **out_db) {
    if (!path || !out_db) return false;
    sqlite3 *db = NULL;
    int rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    char *errmsg = NULL;
    rc = sqlite3_exec(db, LOCAL_SCHEMA, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return false;
    }

    *out_db = db;
    return true;
}

bool db_local_install(sqlite3 *db, const char *name, const char *version, const char *reason) {
    if (!db || !name || !version) return false;
    const char *sql =
        "INSERT INTO installed (name, version, reason, install_date) VALUES (?, ?, ?, ?) "
        "ON CONFLICT(name) DO UPDATE SET version = excluded.version, reason = excluded.reason, install_date = excluded.install_date;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    int64_t now = (int64_t)time(NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, version, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, reason ? reason : "explicit", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, now);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool db_local_remove(sqlite3 *db, const char *name) {
    if (!db || !name) return false;
    const char *sql = "DELETE FROM installed WHERE name = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool db_local_is_installed(sqlite3 *db, const char *name, char **out_version) {
    if (!db || !name) return false;
    if (out_version) *out_version = NULL;

    const char *sql = "SELECT version FROM installed WHERE name = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out_version) {
            *out_version = strdup((const char *)sqlite3_column_text(stmt, 0));
        }
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool db_local_add_file(sqlite3 *db, const char *pkg_name, const char *path) {
    if (!db || !pkg_name || !path) return false;
    const char *sql = "INSERT INTO files (pkg_name, path) VALUES (?, ?) ON CONFLICT(path) DO UPDATE SET pkg_name = excluded.pkg_name;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool db_local_get_files(sqlite3 *db, const char *pkg_name, char ***out_paths, size_t *out_count) {
    if (!db || !pkg_name || !out_paths || !out_count) return false;
    *out_paths = NULL;
    *out_count = 0;

    const char *sql = "SELECT path FROM files WHERE pkg_name = ? ORDER BY path DESC;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_TRANSIENT);

    size_t cap = 16;
    size_t count = 0;
    char **paths = (char **)malloc(cap * sizeof(char *));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= cap) {
            cap *= 2;
            paths = (char **)realloc(paths, cap * sizeof(char *));
        }
        paths[count++] = strdup((const char *)sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    *out_paths = paths;
    *out_count = count;
    return true;
}

bool db_local_find_file_owner(sqlite3 *db, const char *path, char **out_pkg_name) {
    if (!db || !path || !out_pkg_name) return false;
    *out_pkg_name = NULL;

    const char *sql = "SELECT pkg_name FROM files WHERE path = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out_pkg_name = strdup((const char *)sqlite3_column_text(stmt, 0));
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool db_local_list_installed(sqlite3 *db, DbInstalledRecord **out_records, size_t *out_count) {
    if (!db || !out_records || !out_count) return false;
    *out_records = NULL;
    *out_count = 0;

    const char *sql = "SELECT id, name, version, reason, install_date FROM installed ORDER BY name ASC;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    size_t cap = 16;
    size_t count = 0;
    DbInstalledRecord *records = (DbInstalledRecord *)malloc(cap * sizeof(DbInstalledRecord));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= cap) {
            cap *= 2;
            records = (DbInstalledRecord *)realloc(records, cap * sizeof(DbInstalledRecord));
        }
        records[count].id = sqlite3_column_int64(stmt, 0);
        records[count].name = strdup((const char *)sqlite3_column_text(stmt, 1));
        records[count].version = strdup((const char *)sqlite3_column_text(stmt, 2));
        records[count].reason = strdup((const char *)sqlite3_column_text(stmt, 3));
        records[count].install_date = sqlite3_column_int64(stmt, 4);
        count++;
    }

    sqlite3_finalize(stmt);
    *out_records = records;
    *out_count = count;
    return true;
}

void db_local_records_free(DbInstalledRecord *records, size_t count) {
    if (!records) return;
    for (size_t i = 0; i < count; ++i) {
        free(records[i].name);
        free(records[i].version);
        free(records[i].reason);
    }
    free(records);
}

void db_local_files_free(char **paths, size_t count) {
    if (!paths) return;
    for (size_t i = 0; i < count; ++i) {
        free(paths[i]);
    }
    free(paths);
}
