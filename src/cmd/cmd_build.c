#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "pkg/pkg.h"
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cmd_build(int argc, char **argv, const NlpmConfig *cfg) {
    (void)cfg;
    if (argc < 2) {
        fprintf(stderr, "Error: No packaging directory specified.\n");
        fprintf(stderr, "Usage: nlpm build <packaging_directory>\n");
        return 1;
    }

    const char *dir = argv[1];

    char manifest_path[1024];
    snprintf(manifest_path, sizeof(manifest_path), "%s/nlpkg.toml", dir);
    if (access(manifest_path, R_OK) != 0) {
        snprintf(manifest_path, sizeof(manifest_path), "%s/.PKGINFO", dir);
        if (access(manifest_path, R_OK) != 0) {
            fprintf(stderr, "Error: Neither nlpkg.toml nor .PKGINFO found in %s\n", dir);
            return 1;
        }
    }

    char err_buf[256];
    TomlTable *manifest = toml_parse_file(manifest_path, err_buf, sizeof(err_buf));
    if (!manifest) {
        fprintf(stderr, "Error parsing manifest: %s\n", err_buf);
        return 1;
    }

    const char *name = toml_get_string(manifest, "name", "package");
    const char *version = toml_get_string(manifest, "version", "1.0.0");
    const char *arch = toml_get_string(manifest, "arch", "x86_64");

    char data_dir[1024];
    snprintf(data_dir, sizeof(data_dir), "%s/pkg", dir);
    if (access(data_dir, R_OK) != 0) {
        snprintf(data_dir, sizeof(data_dir), "%s/data", dir);
        if (access(data_dir, R_OK) != 0) {
            snprintf(data_dir, sizeof(data_dir), "%s", dir);
        }
    }

    char out_pkg[1024];
    snprintf(out_pkg, sizeof(out_pkg), "%s-%s-%s.nlpkg", name, version, arch);

    printf(":: Building package %s...\n", out_pkg);
    bool ok = pkg_build_archive(manifest_path, (strcmp(data_dir, dir) == 0) ? NULL : data_dir, out_pkg);
    toml_free(manifest);

    if (ok) {
        printf("Package built successfully: %s\n", out_pkg);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to build package archive.\n");
        return 1;
    }
}
