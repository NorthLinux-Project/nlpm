#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include "pkg/pkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cmd_info(int argc, char **argv, const NlpmConfig *cfg) {
    if (argc < 2) {
        fprintf(stderr, "Error: No package specified.\n");
        fprintf(stderr, "Usage: nlpm info <package | file.nlpkg>\n");
        return 1;
    }

    const char *target = argv[1];

    if (strstr(target, ".nlpkg") != NULL && access(target, R_OK) == 0) {
        NlpkgMeta meta = {0};
        if (!pkg_read_meta(target, &meta)) {
            fprintf(stderr, "Error: Failed to read package metadata from %s\n", target);
            return 1;
        }

        printf("Name         : %s\n", meta.name);
        printf("Version      : %s\n", meta.version);
        printf("Description  : %s\n", meta.desc ? meta.desc : "");
        printf("Architecture : %s\n", meta.arch ? meta.arch : "");
        printf("URL          : %s\n", meta.url ? meta.url : "");
        printf("Installed Sz : %lld bytes\n", (long long)meta.isize);
        printf("Archive Sz   : %lld bytes\n", (long long)meta.csize);
        printf("SHA-256      : %s\n", meta.sha256 ? meta.sha256 : "");
        printf("Dependencies : ");
        for (size_t i = 0; i < meta.dep_count; ++i) {
            printf("%s%s%s", meta.deps[i].name,
                   meta.deps[i].constraint ? meta.deps[i].constraint : "",
                   (i + 1 < meta.dep_count) ? ", " : "");
        }
        printf("\n");

        pkg_meta_free(&meta);
        return 0;
    }

    char sync_db_path[1024];
    snprintf(sync_db_path, sizeof(sync_db_path), "%s/sync.db", cfg->dbdir);

    sqlite3 *sync_db = NULL;
    if (db_sync_open(sync_db_path, &sync_db)) {
        DbPkgRecord rec = {0};
        if (db_sync_find_pkg(sync_db, target, &rec)) {
            printf("Name         : %s\n", rec.name);
            printf("Version      : %s\n", rec.version);
            printf("Description  : %s\n", rec.desc ? rec.desc : "");
            printf("Architecture : %s\n", rec.arch ? rec.arch : "");
            printf("Installed Sz : %lld bytes\n", (long long)rec.isize);
            printf("Download Sz  : %lld bytes\n", (long long)rec.csize);
            printf("SHA-256      : %s\n", rec.sha256 ? rec.sha256 : "");

            DbDepRecord *deps = NULL;
            size_t dep_count = 0;
            if (db_sync_get_deps(sync_db, rec.id, &deps, &dep_count)) {
                printf("Dependencies : ");
                for (size_t i = 0; i < dep_count; ++i) {
                    printf("%s%s%s", deps[i].name,
                           deps[i].constraint ? deps[i].constraint : "",
                           (i + 1 < dep_count) ? ", " : "");
                }
                printf("\n");
                db_sync_deps_free(deps, dep_count);
            }

            db_sync_pkg_free(&rec);
            sqlite3_close(sync_db);
            return 0;
        }
        sqlite3_close(sync_db);
    }

    fprintf(stderr, "Error: Package '%s' not found.\n", target);
    return 1;
}
