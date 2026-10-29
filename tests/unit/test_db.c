#include "unity.h"
#include "db/db.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_db_sync_crud(void) {
    sqlite3 *db = NULL;
    TEST_ASSERT_TRUE(db_sync_open(":memory:", &db));

    int64_t repo_id = 0;
    TEST_ASSERT_TRUE(db_sync_repo_upsert(db, "core", "https://pkg.northlinux.org/core", &repo_id));
    TEST_ASSERT_TRUE(repo_id > 0);

    int64_t pkg_id = 0;
    TEST_ASSERT_TRUE(db_sync_pkg_insert(db, repo_id, "nano", "8.3.0", "Nano editor", "x86_64",
                                        3145728, 900000, "abc123sha256", "nano-8.3.0-x86_64.nlpkg", &pkg_id));
    TEST_ASSERT_TRUE(pkg_id > 0);

    TEST_ASSERT_TRUE(db_sync_dep_insert(db, pkg_id, "depend", "glibc", ">=2.33"));
    TEST_ASSERT_TRUE(db_sync_dep_insert(db, pkg_id, "depend", "ncurses", NULL));

    DbPkgRecord pkg = {0};
    TEST_ASSERT_TRUE(db_sync_find_pkg(db, "nano", &pkg));
    TEST_ASSERT_EQUAL_STRING("nano", pkg.name);
    TEST_ASSERT_EQUAL_STRING("8.3.0", pkg.version);
    TEST_ASSERT_EQUAL_STRING("Nano editor", pkg.desc);
    db_sync_pkg_free(&pkg);

    DbPkgRecord *search_results = NULL;
    size_t search_count = 0;
    TEST_ASSERT_TRUE(db_sync_search(db, "editor", &search_results, &search_count));
    TEST_ASSERT_EQUAL_UINT(1, search_count);
    TEST_ASSERT_EQUAL_STRING("nano", search_results[0].name);
    db_sync_pkgs_free(search_results, search_count);

    DbDepRecord *deps = NULL;
    size_t dep_count = 0;
    TEST_ASSERT_TRUE(db_sync_get_deps(db, pkg_id, &deps, &dep_count));
    TEST_ASSERT_EQUAL_UINT(2, dep_count);
    db_sync_deps_free(deps, dep_count);

    sqlite3_close(db);
}

void test_db_local_crud(void) {
    sqlite3 *db = NULL;
    TEST_ASSERT_TRUE(db_local_open(":memory:", &db));

    char *ver = NULL;
    TEST_ASSERT_FALSE(db_local_is_installed(db, "bash", &ver));

    TEST_ASSERT_TRUE(db_local_install(db, "bash", "5.2.0", "explicit"));
    TEST_ASSERT_TRUE(db_local_is_installed(db, "bash", &ver));
    TEST_ASSERT_EQUAL_STRING("5.2.0", ver);
    free(ver);

    TEST_ASSERT_TRUE(db_local_add_file(db, "bash", "/bin/bash"));
    TEST_ASSERT_TRUE(db_local_add_file(db, "bash", "/usr/share/man/man1/bash.1.gz"));

    char *owner = NULL;
    TEST_ASSERT_TRUE(db_local_find_file_owner(db, "/bin/bash", &owner));
    TEST_ASSERT_EQUAL_STRING("bash", owner);
    free(owner);

    char **files = NULL;
    size_t file_count = 0;
    TEST_ASSERT_TRUE(db_local_get_files(db, "bash", &files, &file_count));
    TEST_ASSERT_EQUAL_UINT(2, file_count);
    db_local_files_free(files, file_count);

    DbInstalledRecord *list = NULL;
    size_t list_count = 0;
    TEST_ASSERT_TRUE(db_local_list_installed(db, &list, &list_count));
    TEST_ASSERT_EQUAL_UINT(1, list_count);
    TEST_ASSERT_EQUAL_STRING("bash", list[0].name);
    db_local_records_free(list, list_count);

    TEST_ASSERT_TRUE(db_local_remove(db, "bash"));
    TEST_ASSERT_FALSE(db_local_is_installed(db, "bash", NULL));

    files = NULL;
    file_count = 0;
    TEST_ASSERT_TRUE(db_local_get_files(db, "bash", &files, &file_count));
    TEST_ASSERT_EQUAL_UINT(0, file_count);
    db_local_files_free(files, file_count);

    sqlite3_close(db);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_db_sync_crud);
    RUN_TEST(test_db_local_crud);
    return UNITY_END();
}
