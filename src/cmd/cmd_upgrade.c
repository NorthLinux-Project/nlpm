#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include "pkg/pkg.h"
#include "util/version.h"
#include "net/net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cmd_upgrade(int argc, char **argv, const NlpmConfig *cfg) {
    (void)argc;
    (void)argv;

    char local_db_path[1024];
    snprintf(local_db_path, sizeof(local_db_path), "%s/local.db", cfg->dbdir);

    sqlite3 *local_db = NULL;
    if (!db_local_open(local_db_path, &local_db)) {
        fprintf(stderr, "Error: Failed to open local package database at %s\n", local_db_path);
        return 1;
    }

    char sync_db_path[1024];
    snprintf(sync_db_path, sizeof(sync_db_path), "%s/sync.db", cfg->dbdir);

    sqlite3 *sync_db = NULL;
    if (!db_sync_open(sync_db_path, &sync_db)) {
        sqlite3_close(local_db);
        fprintf(stderr, "Error: Repository database not found. Run 'nlpm sync' first.\n");
        return 1;
    }

    DbInstalledRecord *installed = NULL;
    size_t count = 0;
    if (!db_local_list_installed(local_db, &installed, &count)) {
        sqlite3_close(sync_db);
        sqlite3_close(local_db);
        return 1;
    }

    printf(":: Checking for package upgrades...\n");
    size_t upgrades_found = 0;
    char err_buf[512];

    for (size_t i = 0; i < count; ++i) {
        DbPkgRecord repo_pkg = {0};
        if (db_sync_find_pkg(sync_db, installed[i].name, &repo_pkg)) {
            if (version_cmp(repo_pkg.version, installed[i].version) > 0) {
                upgrades_found++;
                printf(":: Upgrading %s: %s -> %s\n", installed[i].name, installed[i].version, repo_pkg.version);

                char cached_pkg_path[1024];
                snprintf(cached_pkg_path, sizeof(cached_pkg_path), "%s/%s", cfg->cachedir, repo_pkg.filename);

                if (access(cached_pkg_path, R_OK) != 0) {
                    char repo_url[1024] = "";
                    for (size_t r = 0; r < cfg->repo_count; ++r) {
                        if (cfg->repos[r].enabled) {
                            snprintf(repo_url, sizeof(repo_url), "%s/%s", cfg->repos[r].url, repo_pkg.filename);
                            break;
                        }
                    }

                    if (repo_url[0] != '\0') {
                        printf("   downloading %s...\n", repo_pkg.filename);
                        net_download_file(repo_url, cached_pkg_path, repo_pkg.sha256, err_buf, sizeof(err_buf));
                    }
                }

                if (!pkg_upgrade(local_db, cached_pkg_path, cfg->rootdir, err_buf, sizeof(err_buf))) {
                    fprintf(stderr, "Error upgrading %s: %s\n", installed[i].name, err_buf);
                } else {
                    printf("   upgraded %s successfully.\n", installed[i].name);
                }
            }
            db_sync_pkg_free(&repo_pkg);
        }
    }

    if (upgrades_found == 0) {
        printf(":: System is already up to date.\n");
    }

    db_local_records_free(installed, count);
    sqlite3_close(sync_db);
    sqlite3_close(local_db);
    return 0;
}
