#ifndef SQLITE3_H
#define SQLITE3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SQLITE_OK           0
#define SQLITE_ERROR        1
#define SQLITE_INTERNAL     2
#define SQLITE_PERM         3
#define SQLITE_ABORT        4
#define SQLITE_BUSY         5
#define SQLITE_LOCKED       6
#define SQLITE_NOMEM        7
#define SQLITE_READONLY     8
#define SQLITE_INTERRUPT    9
#define SQLITE_IOERR       10
#define SQLITE_CORRUPT     11
#define SQLITE_NOTFOUND    12
#define SQLITE_FULL        13
#define SQLITE_CANTOPEN    14
#define SQLITE_PROTOCOL    15
#define SQLITE_EMPTY       16
#define SQLITE_SCHEMA      17
#define SQLITE_TOOBIG      18
#define SQLITE_CONSTRAINT  19
#define SQLITE_MISMATCH    20
#define SQLITE_MISUSE      21
#define SQLITE_NOLFS       22
#define SQLITE_AUTH        23
#define SQLITE_FORMAT      24
#define SQLITE_RANGE       25
#define SQLITE_NOTADB      26
#define SQLITE_ROW         100
#define SQLITE_DONE        101

#define SQLITE_OPEN_READONLY         0x00000001
#define SQLITE_OPEN_READWRITE        0x00000002
#define SQLITE_OPEN_CREATE           0x00000004
#define SQLITE_OPEN_URI              0x00000040
#define SQLITE_OPEN_MEMORY           0x00000080
#define SQLITE_OPEN_NOMUTEX          0x00008000
#define SQLITE_OPEN_FULLMUTEX        0x00010000
#define SQLITE_OPEN_SHAREDCACHE      0x00020000
#define SQLITE_OPEN_PRIVATECACHE     0x00040000

#define SQLITE_STATIC      ((void(*)(void *))0)
#define SQLITE_TRANSIENT   ((void(*)(void *))-1)

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef int64_t sqlite3_int64;
typedef uint64_t sqlite3_uint64;

const char *sqlite3_libversion(void);
int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs);
int sqlite3_close(sqlite3 *db);
int sqlite3_exec(sqlite3 *db, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg);
const char *sqlite3_errmsg(sqlite3 *db);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3 *db);
int sqlite3_changes(sqlite3 *db);

int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
int sqlite3_step(sqlite3_stmt *stmt);
int sqlite3_finalize(sqlite3_stmt *pStmt);
int sqlite3_reset(sqlite3_stmt *pStmt);

int sqlite3_bind_int(sqlite3_stmt *stmt, int index, int val);
int sqlite3_bind_int64(sqlite3_stmt *stmt, int index, sqlite3_int64 val);
int sqlite3_bind_text(sqlite3_stmt *stmt, int index, const char *val, int len, void(*destructor)(void*));
int sqlite3_bind_null(sqlite3_stmt *stmt, int index);

int sqlite3_column_count(sqlite3_stmt *pStmt);
int sqlite3_column_type(sqlite3_stmt *pStmt, int iCol);
int sqlite3_column_int(sqlite3_stmt *pStmt, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
int sqlite3_column_bytes(sqlite3_stmt *pStmt, int iCol);

void sqlite3_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
