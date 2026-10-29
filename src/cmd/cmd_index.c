#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "repo/repo.h"
#include <stdio.h>
#include <string.h>

int cmd_index(int argc, char **argv, const NlpmConfig *cfg) {
    (void)cfg;
    if (argc < 2) {
        fprintf(stderr, "Error: No package directory specified.\n");
        fprintf(stderr, "Usage: nlpm index <package_directory> [output_nldb_path]\n");
        return 1;
    }

    const char *pkg_dir = argv[1];
    char out_nldb[1024];

    if (argc >= 3) {
        snprintf(out_nldb, sizeof(out_nldb), "%s", argv[2]);
    } else {
        snprintf(out_nldb, sizeof(out_nldb), "%s/core.nldb", pkg_dir);
    }

    printf(":: Indexing packages in %s -> %s...\n", pkg_dir, out_nldb);
    if (!repo_create_index(pkg_dir, out_nldb)) {
        fprintf(stderr, "Error: Failed to create repository database %s\n", out_nldb);
        return 1;
    }

    printf("Repository database created successfully: %s\n", out_nldb);
    return 0;
}
