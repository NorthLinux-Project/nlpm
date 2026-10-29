#include "unity.h"
#include "pkg/pkg.h"
#include "repo/repo.h"
#include "db/db.h"
#include "config/config.h"
#include "cmd/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

void test_full_pipeline_cli(void) {
    const char *test_base = "/tmp/test_pipeline";
    char sysroot[2048], pkg_src[2048], repo_dir[2048], db_dir[2048], cache_dir[2048];

    snprintf(sysroot, sizeof(sysroot), "%.256s/sysroot", test_base);
    snprintf(pkg_src, sizeof(pkg_src), "%.256s/pkg_src", test_base);
    snprintf(repo_dir, sizeof(repo_dir), "%.256s/repo", test_base);
    snprintf(db_dir, sizeof(db_dir), "%.256s/db", test_base);
    snprintf(cache_dir, sizeof(cache_dir), "%.256s/cache", test_base);

    mkdir(test_base, 0755);
    mkdir(sysroot, 0755);
    mkdir(pkg_src, 0755);
    mkdir(repo_dir, 0755);
    mkdir(db_dir, 0755);
    mkdir(cache_dir, 0755);

    char bin_dir[2048];
    snprintf(bin_dir, sizeof(bin_dir), "%.256s/bin", pkg_src);
    mkdir(bin_dir, 0755);

    char bin_file[2048];
    snprintf(bin_file, sizeof(bin_file), "%.256s/northapp", bin_dir);
    FILE *bf = fopen(bin_file, "wb");
    TEST_ASSERT_NOT_NULL(bf);
    fputs("#!/bin/sh\necho North Linux App\n", bf);
    fclose(bf);
    chmod(bin_file, 0755);

    char manifest_file[2048];
    snprintf(manifest_file, sizeof(manifest_file), "%.256s/nlpkg.toml", pkg_src);
    FILE *mf = fopen(manifest_file, "wb");
    TEST_ASSERT_NOT_NULL(mf);
    fputs("name = \"northapp\"\nversion = \"1.0.0\"\ndesc = \"Official North App\"\narch = \"x86_64\"\nisize = 512\n", mf);
    fclose(mf);

    char pkg_dest[2048];
    snprintf(pkg_dest, sizeof(pkg_dest), "%.256s/northapp-1.0.0-x86_64.nlpkg", repo_dir);
    TEST_ASSERT_TRUE(pkg_build_archive(manifest_file, pkg_src, pkg_dest));

    char nldb_dest[2048];
    snprintf(nldb_dest, sizeof(nldb_dest), "%.256s/core.nldb", repo_dir);
    TEST_ASSERT_TRUE(repo_create_index(repo_dir, nldb_dest));

    NlpmConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.rootdir = strdup(sysroot);
    cfg.dbdir = strdup(db_dir);
    cfg.cachedir = strdup(cache_dir);
    cfg.parallel = 2;
    cfg.repo_count = 1;
    cfg.repos = (NlpmRepoConfig *)calloc(1, sizeof(NlpmRepoConfig));
    cfg.repos[0].name = strdup("core");
    char file_url[2048];
    snprintf(file_url, sizeof(file_url), "file://%.256s", repo_dir);
    cfg.repos[0].url = strdup(file_url);
    cfg.repos[0].enabled = true;

    char *sync_args[] = {"sync"};
    TEST_ASSERT_EQUAL_INT(0, cmd_sync(1, sync_args, &cfg));

    char *search_args[] = {"search", "North App"};
    TEST_ASSERT_EQUAL_INT(0, cmd_search(2, search_args, &cfg));

    char *info_args[] = {"info", "northapp"};
    TEST_ASSERT_EQUAL_INT(0, cmd_info(2, info_args, &cfg));

    char *install_args[] = {"install", "northapp"};
    TEST_ASSERT_EQUAL_INT(0, cmd_install(2, install_args, &cfg));

    char installed_app[2048];
    snprintf(installed_app, sizeof(installed_app), "%.256s/bin/northapp", sysroot);
    TEST_ASSERT_EQUAL_INT(0, access(installed_app, F_OK));

    char *list_args[] = {"list"};
    TEST_ASSERT_EQUAL_INT(0, cmd_list(1, list_args, &cfg));

    char *owns_args[] = {"owns", installed_app};
    TEST_ASSERT_EQUAL_INT(0, cmd_owns(2, owns_args, &cfg));

    char *remove_args[] = {"remove", "northapp"};
    TEST_ASSERT_EQUAL_INT(0, cmd_remove(2, remove_args, &cfg));
    TEST_ASSERT_EQUAL_INT(-1, access(installed_app, F_OK));

    config_free(&cfg);

    unlink(bin_file);
    rmdir(bin_dir);
    unlink(manifest_file);
    rmdir(pkg_src);
    unlink(pkg_dest);
    unlink(nldb_dest);
    rmdir(repo_dir);

    char local_db_file[2048];
    snprintf(local_db_file, sizeof(local_db_file), "%.256s/local.db", db_dir);
    unlink(local_db_file);
    snprintf(local_db_file, sizeof(local_db_file), "%.256s/sync.db", db_dir);
    unlink(local_db_file);
    rmdir(db_dir);
    rmdir(cache_dir);
    rmdir(sysroot);
    rmdir(test_base);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_pipeline_cli);
    return UNITY_END();
}
