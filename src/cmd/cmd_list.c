#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include <stdio.h>

int cmd_list(int argc, char **argv, const NlpmConfig *cfg) {
    (void)argc;
    (void)argv;

    char local_db_path[1024];
    snprintf(local_db_path, sizeof(local_db_path), "%s/local.db", cfg->dbdir);

    sqlite3 *local_db = NULL;
    if (!db_local_open(local_db_path, &local_db)) {
        fprintf(stderr, "Error: Failed to open local database at %s\n", local_db_path);
        return 1;
    }

    DbInstalledRecord *records = NULL;
    size_t count = 0;
    if (!db_local_list_installed(local_db, &records, &count)) {
        sqlite3_close(local_db);
        return 1;
    }

    if (count == 0) {
        printf("No packages currently installed.\n");
    } else {
        printf("Installed packages (%zu):\n", count);
        for (size_t i = 0; i < count; ++i) {
            printf("  %s %s [%s]\n", records[i].name, records[i].version, records[i].reason);
        }
    }

    db_local_records_free(records, count);
    sqlite3_close(local_db);
    return 0;
}
