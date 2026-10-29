#define _POSIX_C_SOURCE 200809L
#include "pkg.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

static void cleanup_empty_dirs(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *dir = dirname(tmp);
    while (dir && strcmp(dir, ".") != 0 && strcmp(dir, "/") != 0) {
        if (rmdir(dir) != 0) {
            break;
        }
        dir = dirname(dir);
    }
}

bool pkg_remove(sqlite3 *local_db, const char *pkg_name, const char *root_dir, char *err_buf, size_t err_size) {
    if (!local_db || !pkg_name) return false;

    char *ver = NULL;
    if (!db_local_is_installed(local_db, pkg_name, &ver)) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Package '%s' is not installed", pkg_name);
        return false;
    }
    free(ver);

    char **files = NULL;
    size_t file_count = 0;
    db_local_get_files(local_db, pkg_name, &files, &file_count);

    for (size_t i = 0; i < file_count; ++i) {
        unlink(files[i]);
        if (root_dir && strcmp(root_dir, "/") != 0) {
            cleanup_empty_dirs(files[i]);
        }
    }
    db_local_files_free(files, file_count);

    if (!db_local_remove(local_db, pkg_name)) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to remove package '%s' from local database", pkg_name);
        return false;
    }

    return true;
}
