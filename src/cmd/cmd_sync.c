#include "cmd.h"
#include "db/db.h"
#include "repo/repo.h"
#include <stdio.h>
#include <sys/stat.h>

int cmd_sync(int argc, char **argv, const NlpmConfig *cfg) {
    (void)argc;
    (void)argv;

    mkdir(cfg->dbdir, 0755);

    char sync_db_path[1024];
    snprintf(sync_db_path, sizeof(sync_db_path), "%s/sync.db", cfg->dbdir);

    sqlite3 *sync_db = NULL;
    if (!db_sync_open(sync_db_path, &sync_db)) {
        fprintf(stderr, "Error: Failed to open repository sync database at %s\n", sync_db_path);
        return 1;
    }

    printf(":: Synchronizing package databases...\n");
    for (size_t i = 0; i < cfg->repo_count; ++i) {
        if (cfg->repos[i].enabled) {
            printf(" downloading %s...\n", cfg->repos[i].name);
        }
    }

    bool ok = repo_sync_all(sync_db, cfg);
    sqlite3_close(sync_db);

    if (ok) {
        printf(":: Synchronization complete.\n");
        return 0;
    } else {
        fprintf(stderr, "Error: One or more repositories failed to synchronize.\n");
        return 1;
    }
}
