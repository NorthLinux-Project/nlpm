#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>

int cmd_owns(int argc, char **argv, const NlpmConfig *cfg) {
    if (argc < 2) {
        fprintf(stderr, "Error: No file path specified.\n");
        fprintf(stderr, "Usage: nlpm owns <file_path>\n");
        return 1;
    }

    const char *path = argv[1];

    char local_db_path[1024];
    snprintf(local_db_path, sizeof(local_db_path), "%s/local.db", cfg->dbdir);

    sqlite3 *local_db = NULL;
    if (!db_local_open(local_db_path, &local_db)) {
        fprintf(stderr, "Error: Failed to open local database at %s\n", local_db_path);
        return 1;
    }

    char *owner = NULL;
    if (db_local_find_file_owner(local_db, path, &owner)) {
        printf("%s is owned by %s\n", path, owner);
        free(owner);
        sqlite3_close(local_db);
        return 0;
    }

    fprintf(stderr, "Error: No installed package owns '%s'.\n", path);
    sqlite3_close(local_db);
    return 1;
}
