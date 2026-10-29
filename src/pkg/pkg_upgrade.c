#define _POSIX_C_SOURCE 200809L
#include "pkg.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool pkg_upgrade(sqlite3 *local_db, const char *pkg_path, const char *root_dir, char *err_buf, size_t err_size) {
    if (!local_db || !pkg_path || !root_dir) return false;

    NlpkgMeta new_meta = {0};
    if (!pkg_read_meta(pkg_path, &new_meta)) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to read new package metadata from %s", pkg_path);
        return false;
    }

    char *installed_ver = NULL;
    if (!db_local_is_installed(local_db, new_meta.name, &installed_ver)) {
        pkg_meta_free(&new_meta);
        return pkg_install(local_db, pkg_path, root_dir, "explicit", err_buf, err_size);
    }
    free(installed_ver);

    NlpkgFileEntry *new_files = NULL;
    size_t new_file_count = 0;
    if (!pkg_read_file_list(pkg_path, &new_files, &new_file_count)) {
        pkg_meta_free(&new_meta);
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to read file list from new package");
        return false;
    }

    char **old_files = NULL;
    size_t old_file_count = 0;
    db_local_get_files(local_db, new_meta.name, &old_files, &old_file_count);

    for (size_t i = 0; i < old_file_count; ++i) {
        bool still_present = false;
        for (size_t j = 0; j < new_file_count; ++j) {
            char new_full_path[1024];
            if (strcmp(root_dir, "/") == 0) {
                snprintf(new_full_path, sizeof(new_full_path), "/%s", new_files[j].path);
            } else {
                snprintf(new_full_path, sizeof(new_full_path), "%s/%s", root_dir, new_files[j].path);
            }
            if (strcmp(old_files[i], new_full_path) == 0) {
                still_present = true;
                break;
            }
        }
        if (!still_present) {
            unlink(old_files[i]);
        }
    }
    db_local_files_free(old_files, old_file_count);
    pkg_file_entries_free(new_files, new_file_count);
    pkg_meta_free(&new_meta);

    return pkg_install(local_db, pkg_path, root_dir, "upgrade", err_buf, err_size);
}
