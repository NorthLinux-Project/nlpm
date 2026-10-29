#define _POSIX_C_SOURCE 200809L
#include "cmd.h"
#include "db/db.h"
#include "pkg/pkg.h"
#include "solver/solver.h"
#include "net/net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int cmd_install(int argc, char **argv, const NlpmConfig *cfg) {
    if (argc < 2) {
        fprintf(stderr, "Error: No packages specified for installation.\n");
        fprintf(stderr, "Usage: nlpm install <package...> [options]\n");
        return 1;
    }

    mkdir(cfg->dbdir, 0755);
    mkdir(cfg->cachedir, 0755);

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
    db_sync_open(sync_db_path, &sync_db);

    char err_buf[512];
    int exit_code = 0;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (arg[0] == '-') continue;

        if (strstr(arg, ".nlpkg") != NULL && access(arg, R_OK) == 0) {
            printf(":: Installing local package %s...\n", arg);
            if (!pkg_install(local_db, arg, cfg->rootdir, "explicit", err_buf, sizeof(err_buf))) {
                fprintf(stderr, "Error installing %s: %s\n", arg, err_buf);
                exit_code = 1;
            } else {
                printf("Installed %s successfully.\n", arg);
            }
            continue;
        }

        if (!sync_db) {
            fprintf(stderr, "Error: Repository database not found. Run 'nlpm sync' first.\n");
            exit_code = 1;
            break;
        }

        printf(":: Resolving dependencies for %s...\n", arg);
        Solver s = solver_create(sync_db);
        const char *roots[] = {arg};
        const char *constraints[] = {"*"};
        SolvePlan plan = solver_solve(&s, roots, constraints, 1);

        if (!plan.success) {
            fprintf(stderr, "Error resolving dependencies: %s\n", plan.error_msg);
            solver_destroy(&s);
            exit_code = 1;
            continue;
        }

        printf(":: Package transaction plan (%zu packages):\n", plan.count);
        for (size_t p = 0; p < plan.count; ++p) {
            printf("   [%zu] %s %s\n", p + 1, plan.steps[p].pkg, plan.steps[p].version);
        }

        for (size_t p = 0; p < plan.count; ++p) {
            const char *pkg_name = plan.steps[p].pkg;
            DbPkgRecord rec = {0};
            if (!db_sync_find_pkg(sync_db, pkg_name, &rec)) {
                fprintf(stderr, "Error: Package metadata missing for %s\n", pkg_name);
                exit_code = 1;
                break;
            }

            char cached_pkg_path[1024];
            snprintf(cached_pkg_path, sizeof(cached_pkg_path), "%s/%s", cfg->cachedir, rec.filename);

            if (access(cached_pkg_path, R_OK) != 0) {
                char repo_url[1024] = "";
                for (size_t r = 0; r < cfg->repo_count; ++r) {
                    if (cfg->repos[r].enabled) {
                        snprintf(repo_url, sizeof(repo_url), "%s/%s", cfg->repos[r].url, rec.filename);
                        break;
                    }
                }

                if (repo_url[0] != '\0') {
                    printf(":: Downloading %s...\n", rec.filename);
                    if (!net_download_file(repo_url, cached_pkg_path, rec.sha256, err_buf, sizeof(err_buf))) {
                        fprintf(stderr, "Error downloading %s: %s\n", rec.filename, err_buf);
                        db_sync_pkg_free(&rec);
                        exit_code = 1;
                        break;
                    }
                }
            }

            const char *reason = (strcmp(pkg_name, arg) == 0) ? "explicit" : "dependency";
            printf(":: Extracting and installing %s (%s)...\n", pkg_name, rec.version);
            if (!pkg_install(local_db, cached_pkg_path, cfg->rootdir, reason, err_buf, sizeof(err_buf))) {
                fprintf(stderr, "Error installing %s: %s\n", pkg_name, err_buf);
                db_sync_pkg_free(&rec);
                exit_code = 1;
                break;
            }

            db_sync_pkg_free(&rec);
        }

        solve_plan_free(&plan);
        solver_destroy(&s);
    }

    if (sync_db) sqlite3_close(sync_db);
    sqlite3_close(local_db);
    return exit_code;
}
