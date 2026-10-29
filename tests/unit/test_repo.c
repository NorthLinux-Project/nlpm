#include "unity.h"
#include "pkg/pkg.h"
#include "repo/repo.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

void test_repo_index_and_import(void) {
    mkdir("/tmp/test_repo_dir", 0755);

    FILE *mf = fopen("/tmp/test_repo_manifest.toml", "wb");
    TEST_ASSERT_NOT_NULL(mf);
    fputs("name = \"zsh\"\nversion = \"5.9.0\"\ndesc = \"Z shell\"\narch = \"x86_64\"\nisize = 2048\n[deps]\ndepend = [\"glibc>=2.30\"]\n", mf);
    fclose(mf);

    const char *pkg1 = "/tmp/test_repo_dir/zsh-5.9.0-x86_64.nlpkg";
    TEST_ASSERT_TRUE(pkg_build_archive("/tmp/test_repo_manifest.toml", NULL, pkg1));

    FILE *mf2 = fopen("/tmp/test_repo_manifest.toml", "wb");
    TEST_ASSERT_NOT_NULL(mf2);
    fputs("name = \"curl\"\nversion = \"8.5.0\"\ndesc = \"command line url tool\"\narch = \"x86_64\"\nisize = 1048576\n[deps]\ndepend = [\"glibc\", \"openssl>=3.0\"]\n", mf2);
    fclose(mf2);

    const char *pkg2 = "/tmp/test_repo_dir/curl-8.5.0-x86_64.nlpkg";
    TEST_ASSERT_TRUE(pkg_build_archive("/tmp/test_repo_manifest.toml", NULL, pkg2));

    const char *nldb_path = "/tmp/test_repo_dir/core.nldb";
    TEST_ASSERT_TRUE(repo_create_index("/tmp/test_repo_dir", nldb_path));

    sqlite3 *db = NULL;
    TEST_ASSERT_TRUE(db_sync_open(":memory:", &db));

    int64_t repo_id = 0;
    TEST_ASSERT_TRUE(db_sync_repo_upsert(db, "core", "file:///tmp/test_repo_dir", &repo_id));
    TEST_ASSERT_TRUE(repo_import_index(db, repo_id, nldb_path));

    DbPkgRecord rec = {0};
    TEST_ASSERT_TRUE(db_sync_find_pkg(db, "zsh", &rec));
    TEST_ASSERT_EQUAL_STRING("zsh", rec.name);
    TEST_ASSERT_EQUAL_STRING("5.9.0", rec.version);
    TEST_ASSERT_EQUAL_STRING("Z shell", rec.desc);

    DbDepRecord *deps = NULL;
    size_t dep_count = 0;
    TEST_ASSERT_TRUE(db_sync_get_deps(db, rec.id, &deps, &dep_count));
    TEST_ASSERT_EQUAL_UINT(1, dep_count);
    TEST_ASSERT_EQUAL_STRING("glibc", deps[0].name);
    TEST_ASSERT_EQUAL_STRING(">=2.30", deps[0].constraint);
    db_sync_deps_free(deps, dep_count);
    db_sync_pkg_free(&rec);

    TEST_ASSERT_TRUE(db_sync_find_pkg(db, "curl", &rec));
    TEST_ASSERT_EQUAL_STRING("curl", rec.name);
    TEST_ASSERT_EQUAL_STRING("8.5.0", rec.version);
    db_sync_pkg_free(&rec);

    sqlite3_close(db);

    unlink(pkg1);
    unlink(pkg2);
    unlink(nldb_path);
    unlink("/tmp/test_repo_manifest.toml");
    rmdir("/tmp/test_repo_dir");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_repo_index_and_import);
    return UNITY_END();
}
