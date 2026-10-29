#define _POSIX_C_SOURCE 200809L
#include "pkg.h"
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool add_file_to_archive(struct archive *a, const char *fs_path, const char *archive_path) {
    struct stat st;
    if (lstat(fs_path, &st) != 0) return false;

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, archive_path);
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, S_ISDIR(st.st_mode) ? AE_IFDIR : (S_ISLNK(st.st_mode) ? AE_IFLNK : AE_IFREG));
    archive_entry_set_perm(entry, st.st_mode & 0777);
    archive_entry_set_mtime(entry, st.st_mtime, 0);

    if (S_ISLNK(st.st_mode)) {
        char link_target[1024];
        ssize_t len = readlink(fs_path, link_target, sizeof(link_target) - 1);
        if (len > 0) {
            link_target[len] = '\0';
            archive_entry_set_symlink(entry, link_target);
        }
    }

    if (archive_write_header(a, entry) != ARCHIVE_OK) {
        archive_entry_free(entry);
        return false;
    }

    if (S_ISREG(st.st_mode) && st.st_size > 0) {
        FILE *fp = fopen(fs_path, "rb");
        if (fp) {
            char buf[16384];
            size_t bytes;
            while ((bytes = fread(buf, 1, sizeof(buf), fp)) > 0) {
                archive_write_data(a, buf, bytes);
            }
            fclose(fp);
        }
    }

    archive_entry_free(entry);
    return true;
}

static bool recurse_dir_and_add(struct archive *a, const char *dir_path, const char *archive_prefix) {
    DIR *d = opendir(dir_path);
    if (!d) return false;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        char fs_child[1024];
        snprintf(fs_child, sizeof(fs_child), "%s/%s", dir_path, de->d_name);

        char arc_child[1024];
        snprintf(arc_child, sizeof(arc_child), "%s/%s", archive_prefix, de->d_name);

        struct stat st;
        if (lstat(fs_child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            add_file_to_archive(a, fs_child, arc_child);
            recurse_dir_and_add(a, fs_child, arc_child);
        } else {
            add_file_to_archive(a, fs_child, arc_child);
        }
    }

    closedir(d);
    return true;
}

bool pkg_build_archive(const char *manifest_toml_path, const char *data_root_dir, const char *out_nlpkg_path) {
    if (!manifest_toml_path || !out_nlpkg_path) return false;

    struct archive *a = archive_write_new();
    archive_write_add_filter_zstd(a);
    archive_write_set_format_pax_restricted(a);

    if (archive_write_open_filename(a, out_nlpkg_path) != ARCHIVE_OK) {
        archive_write_free(a);
        return false;
    }

    if (!add_file_to_archive(a, manifest_toml_path, ".PKGINFO")) {
        archive_write_close(a);
        archive_write_free(a);
        return false;
    }

    if (data_root_dir && access(data_root_dir, R_OK) == 0) {
        recurse_dir_and_add(a, data_root_dir, "data");
    }

    archive_write_close(a);
    archive_write_free(a);
    return true;
}
