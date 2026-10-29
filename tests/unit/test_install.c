#include "unity.h"
#include "pkg/pkg.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

void test_full_install_upgrade_remove_lifecycle(void) {
    const char *root = "/tmp/test_sysroot";
    mkdir(root, 0755);

    sqlite3 *local_db = NULL;
    TEST_ASSERT_TRUE(db_local_open(":memory:", &local_db));

    mkdir("/tmp/test_src_app_1", 0755);
    mkdir("/tmp/test_src_app_1/bin", 0755);
    FILE *f1 = fopen("/tmp/test_src_app_1/bin/app", "wb");
    TEST_ASSERT_NOT_NULL(f1);
    fputs("version 1.0", f1);
    fclose(f1);

    FILE *m1 = fopen("/tmp/test_manifest_1.toml", "wb");
    TEST_ASSERT_NOT_NULL(m1);
    fputs("name = \"app\"\nversion = \"1.0.0\"\n", m1);
    fclose(m1);

    const char *pkg_v1 = "/tmp/app-1.0.0-x86_64.nlpkg";
    TEST_ASSERT_TRUE(pkg_build_archive("/tmp/test_manifest_1.toml", "/tmp/test_src_app_1", pkg_v1));

    char err[256];
    TEST_ASSERT_TRUE(pkg_install(local_db, pkg_v1, root, "explicit", err, sizeof(err)));

    char *ver = NULL;
    TEST_ASSERT_TRUE(db_local_is_installed(local_db, "app", &ver));
    TEST_ASSERT_EQUAL_STRING("1.0.0", ver);
    free(ver);

    char installed_bin[1024];
    snprintf(installed_bin, sizeof(installed_bin), "%s/bin/app", root);
    TEST_ASSERT_EQUAL_INT(0, access(installed_bin, F_OK));

    char *owner = NULL;
    TEST_ASSERT_TRUE(db_local_find_file_owner(local_db, installed_bin, &owner));
    TEST_ASSERT_EQUAL_STRING("app", owner);
    free(owner);

    mkdir("/tmp/test_src_app_2", 0755);
    mkdir("/tmp/test_src_app_2/bin", 0755);
    FILE *f2 = fopen("/tmp/test_src_app_2/bin/app", "wb");
    TEST_ASSERT_NOT_NULL(f2);
    fputs("version 2.0", f2);
    fclose(f2);

    FILE *m2 = fopen("/tmp/test_manifest_2.toml", "wb");
    TEST_ASSERT_NOT_NULL(m2);
    fputs("name = \"app\"\nversion = \"2.0.0\"\n", m2);
    fclose(m2);

    const char *pkg_v2 = "/tmp/app-2.0.0-x86_64.nlpkg";
    TEST_ASSERT_TRUE(pkg_build_archive("/tmp/test_manifest_2.toml", "/tmp/test_src_app_2", pkg_v2));

    TEST_ASSERT_TRUE(pkg_upgrade(local_db, pkg_v2, root, err, sizeof(err)));
    TEST_ASSERT_TRUE(db_local_is_installed(local_db, "app", &ver));
    TEST_ASSERT_EQUAL_STRING("2.0.0", ver);
    free(ver);

    TEST_ASSERT_TRUE(pkg_remove(local_db, "app", root, err, sizeof(err)));
    TEST_ASSERT_FALSE(db_local_is_installed(local_db, "app", NULL));
    TEST_ASSERT_EQUAL_INT(-1, access(installed_bin, F_OK));

    sqlite3_close(local_db);

    unlink("/tmp/test_src_app_1/bin/app");
    rmdir("/tmp/test_src_app_1/bin");
    rmdir("/tmp/test_src_app_1");
    unlink("/tmp/test_src_app_2/bin/app");
    rmdir("/tmp/test_src_app_2/bin");
    rmdir("/tmp/test_src_app_2");
    unlink("/tmp/test_manifest_1.toml");
    unlink("/tmp/test_manifest_2.toml");
    unlink(pkg_v1);
    unlink(pkg_v2);
    rmdir(root);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_install_upgrade_remove_lifecycle);
    return UNITY_END();
}
