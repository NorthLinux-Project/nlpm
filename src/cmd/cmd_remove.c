#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include "pkg/pkg.h"
#include <stdio.h>

int cmd_remove(int argc, char **argv, const NlpmConfig *cfg) {
    if (argc < 2) {
        fprintf(stderr, "Error: No packages specified for removal.\n");
        fprintf(stderr, "Usage: nlpm remove <package...> [options]\n");
        return 1;
    }

    char local_db_path[1024];
    snprintf(local_db_path, sizeof(local_db_path), "%s/local.db", cfg->dbdir);

    sqlite3 *local_db = NULL;
    if (!db_local_open(local_db_path, &local_db)) {
        fprintf(stderr, "Error: Failed to open local package database at %s\n", local_db_path);
        return 1;
    }

    char err_buf[512];
    int exit_code = 0;

    for (int i = 1; i < argc; ++i) {
        const char *pkg_name = argv[i];
        if (pkg_name[0] == '-') continue;

        printf(":: Removing package %s...\n", pkg_name);
        if (!pkg_remove(local_db, pkg_name, cfg->rootdir, err_buf, sizeof(err_buf))) {
            fprintf(stderr, "Error removing %s: %s\n", pkg_name, err_buf);
            exit_code = 1;
        } else {
            printf("Removed %s successfully.\n", pkg_name);
        }
    }

    sqlite3_close(local_db);
    return exit_code;
}
