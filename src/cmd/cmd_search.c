#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include <stdio.h>

int cmd_search(int argc, char **argv, const NlpmConfig *cfg) {
    if (argc < 2) {
        fprintf(stderr, "Error: No search query specified.\n");
        fprintf(stderr, "Usage: nlpm search <query>\n");
        return 1;
    }

    const char *query = argv[1];

    char sync_db_path[1024];
    snprintf(sync_db_path, sizeof(sync_db_path), "%s/sync.db", cfg->dbdir);

    sqlite3 *sync_db = NULL;
    if (!db_sync_open(sync_db_path, &sync_db)) {
        fprintf(stderr, "Error: Repository database not found at %s. Run 'nlpm sync' first.\n", sync_db_path);
        return 1;
    }

    DbPkgRecord *results = NULL;
    size_t count = 0;
    if (!db_sync_search(sync_db, query, &results, &count)) {
        sqlite3_close(sync_db);
        return 1;
    }

    if (count == 0) {
        printf("No packages found matching '%s'.\n", query);
    } else {
        for (size_t i = 0; i < count; ++i) {
            printf("%s %s\n    %s\n", results[i].name, results[i].version, results[i].desc ? results[i].desc : "(no description)");
        }
    }

    db_sync_pkgs_free(results, count);
    sqlite3_close(sync_db);
    return 0;
}
