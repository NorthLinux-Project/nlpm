#ifndef REPO_H
#define REPO_H

#include <stddef.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "config/config.h"

bool repo_create_index(const char *pkg_dir, const char *out_nldb_path);
bool repo_import_index(sqlite3 *db, int64_t repo_id, const char *nldb_path);
bool repo_sync_all(sqlite3 *db, const NlpmConfig *cfg);

#endif
