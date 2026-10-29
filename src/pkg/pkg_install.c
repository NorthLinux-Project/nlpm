#define _POSIX_C_SOURCE 200809L
#include "pkg.h"
#include "db/db.h"
#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

static bool create_parent_dirs(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *dir = dirname(tmp);
    if (!dir || strcmp(dir, ".") == 0 || strcmp(dir, "/") == 0) return true;

    char cur[1024] = "";
    char *token = strtok(dir, "/");
    if (dir[0] == '/') strncat(cur, "/", sizeof(cur) - 1);

    while (token) {
        strncat(cur, token, sizeof(cur) - strlen(cur) - 1);
        strncat(cur, "/", sizeof(cur) - strlen(cur) - 1);
        mkdir(cur, 0755);
        token = strtok(NULL, "/");
    }
    return true;
}

bool pkg_install(sqlite3 *local_db, const char *pkg_path, const char *root_dir, const char *reason, char *err_buf, size_t err_size) {
    if (!local_db || !pkg_path || !root_dir) return false;

    NlpkgMeta meta = {0};
    if (!pkg_read_meta(pkg_path, &meta)) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to read package metadata from %s", pkg_path);
        return false;
    }

    NlpkgFileEntry *files = NULL;
    size_t file_count = 0;
    if (!pkg_read_file_list(pkg_path, &files, &file_count)) {
        pkg_meta_free(&meta);
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to read file list from %s", pkg_path);
        return false;
    }

    for (size_t i = 0; i < file_count; ++i) {
        char target_path[1024];
        if (strcmp(root_dir, "/") == 0) {
            snprintf(target_path, sizeof(target_path), "/%s", files[i].path);
        } else {
            snprintf(target_path, sizeof(target_path), "%s/%s", root_dir, files[i].path);
        }

        char *owner = NULL;
        if (db_local_find_file_owner(local_db, target_path, &owner)) {
            if (strcmp(owner, meta.name) != 0) {
                if (err_buf && err_size > 0) {
                    snprintf(err_buf, err_size, "File conflict: '%s' is already owned by package '%s'", target_path, owner);
                }
                free(owner);
                pkg_file_entries_free(files, file_count);
                pkg_meta_free(&meta);
                return false;
            }
            free(owner);
        }
    }

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, pkg_path, 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        pkg_file_entries_free(files, file_count);
        pkg_meta_free(&meta);
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to open archive %s", pkg_path);
        return false;
    }

    struct archive_entry *entry = NULL;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);
        if (strncmp(pathname, "data/", 5) == 0 && pathname[5] != '\0') {
            const char *rel_path = pathname + 5;
            char dest_path[1024];
            if (strcmp(root_dir, "/") == 0) {
                snprintf(dest_path, sizeof(dest_path), "/%s", rel_path);
            } else {
                snprintf(dest_path, sizeof(dest_path), "%s/%s", root_dir, rel_path);
            }

            create_parent_dirs(dest_path);

            mode_t type = archive_entry_filetype(entry);
            if (type == AE_IFDIR) {
                mkdir(dest_path, archive_entry_perm(entry));
            } else if (type == AE_IFLNK) {
                const char *symlink_target = archive_entry_symlink(entry);
                if (symlink_target) {
                    unlink(dest_path);
                    symlink(symlink_target, dest_path);
                }
            } else {
                unlink(dest_path);
                FILE *fp = fopen(dest_path, "wb");
                if (fp) {
                    const void *buff = NULL;
                    size_t size = 0;
                    int64_t offset = 0;
                    while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                        fwrite(buff, 1, size, fp);
                    }
                    fclose(fp);
                    chmod(dest_path, archive_entry_perm(entry));
                }
            }
        } else {
            archive_read_data_skip(a);
        }
    }

    archive_read_close(a);
    archive_read_free(a);

    db_local_install(local_db, meta.name, meta.version, reason ? reason : "explicit");

    for (size_t i = 0; i < file_count; ++i) {
        char full_path[1024];
        if (strcmp(root_dir, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", files[i].path);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", root_dir, files[i].path);
        }
        db_local_add_file(local_db, meta.name, full_path);
    }

    pkg_file_entries_free(files, file_count);
    pkg_meta_free(&meta);
    return true;
}
