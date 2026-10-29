#define _POSIX_C_SOURCE 200809L
#include "repo.h"
#include "net/net.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool repo_sync_all(sqlite3 *db, const NlpmConfig *cfg) {
    if (!db || !cfg) return false;

    mkdir(cfg->cachedir, 0755);

    bool any_success = false;

    for (size_t i = 0; i < cfg->repo_count; ++i) {
        if (!cfg->repos[i].enabled) continue;

        const char *repo_name = cfg->repos[i].name;
        const char *repo_url = cfg->repos[i].url;

        int64_t repo_id = 0;
        if (!db_sync_repo_upsert(db, repo_name, repo_url, &repo_id)) {
            continue;
        }

        char index_dest[1024];
        snprintf(index_dest, sizeof(index_dest), "%s/%s.nldb", cfg->cachedir, repo_name);

        bool downloaded = false;
        if (strncmp(repo_url, "file://", 7) == 0) {
            const char *file_path = repo_url + 7;
            char nldb_src[1024];
            snprintf(nldb_src, sizeof(nldb_src), "%s/%s.nldb", file_path, repo_name);
            if (repo_import_index(db, repo_id, nldb_src)) {
                downloaded = true;
            }
        } else if (repo_url[0] == '/') {
            char nldb_src[1024];
            snprintf(nldb_src, sizeof(nldb_src), "%s/%s.nldb", repo_url, repo_name);
            if (repo_import_index(db, repo_id, nldb_src)) {
                downloaded = true;
            }
        } else {
            char url[1024];
            snprintf(url, sizeof(url), "%s/%s.nldb", repo_url, repo_name);

            char err_buf[256];
            if (net_download_file(url, index_dest, NULL, err_buf, sizeof(err_buf))) {
                if (repo_import_index(db, repo_id, index_dest)) {
                    downloaded = true;
                }
            }
        }

        if (downloaded) {
            any_success = true;
        }
    }

    return any_success || (cfg->repo_count == 0);
}
