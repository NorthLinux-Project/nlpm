#include "unity.h"
#include "solver/solver.h"
#include "db/db.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_solver_diamond_dependency(void) {
    sqlite3 *db = NULL;
    TEST_ASSERT_TRUE(db_sync_open(":memory:", &db));

    int64_t repo_id = 0;
    TEST_ASSERT_TRUE(db_sync_repo_upsert(db, "core", "http://repo", &repo_id));

    int64_t glibc_id = 0;
    TEST_ASSERT_TRUE(db_sync_pkg_insert(db, repo_id, "glibc", "2.38.0", "GNU C Library", "x86_64", 100, 50, "hash", "glibc.nlpkg", &glibc_id));

    int64_t ncurses_id = 0;
    TEST_ASSERT_TRUE(db_sync_pkg_insert(db, repo_id, "ncurses", "6.4.0", "terminal handling", "x86_64", 200, 80, "hash", "ncurses.nlpkg", &ncurses_id));
    TEST_ASSERT_TRUE(db_sync_dep_insert(db, ncurses_id, "depend", "glibc", ">=2.30.0"));

    int64_t nano_id = 0;
    TEST_ASSERT_TRUE(db_sync_pkg_insert(db, repo_id, "nano", "8.0.0", "nano editor", "x86_64", 300, 100, "hash", "nano.nlpkg", &nano_id));
    TEST_ASSERT_TRUE(db_sync_dep_insert(db, nano_id, "depend", "ncurses", ">=6.0.0"));
    TEST_ASSERT_TRUE(db_sync_dep_insert(db, nano_id, "depend", "glibc", ">=2.30.0"));

    Solver s = solver_create(db);
    const char *roots[] = {"nano"};
    const char *constraints[] = {">=7.0.0"};
    SolvePlan plan = solver_solve(&s, roots, constraints, 1);

    TEST_ASSERT_TRUE(plan.success);
    TEST_ASSERT_EQUAL_UINT(3, plan.count);

    bool has_glibc = false, has_ncurses = false, has_nano = false;
    for (size_t i = 0; i < plan.count; ++i) {
        if (strcmp(plan.steps[i].pkg, "glibc") == 0) has_glibc = true;
        if (strcmp(plan.steps[i].pkg, "ncurses") == 0) has_ncurses = true;
        if (strcmp(plan.steps[i].pkg, "nano") == 0) has_nano = true;
    }
    TEST_ASSERT_TRUE(has_glibc);
    TEST_ASSERT_TRUE(has_ncurses);
    TEST_ASSERT_TRUE(has_nano);

    solve_plan_free(&plan);
    solver_destroy(&s);
    sqlite3_close(db);
}

void test_solver_missing_package(void) {
    sqlite3 *db = NULL;
    TEST_ASSERT_TRUE(db_sync_open(":memory:", &db));

    Solver s = solver_create(db);
    const char *roots[] = {"nonexistent"};
    const char *constraints[] = {"*"};
    SolvePlan plan = solver_solve(&s, roots, constraints, 1);

    TEST_ASSERT_FALSE(plan.success);
    TEST_ASSERT_TRUE(strlen(plan.error_msg) > 0);

    solve_plan_free(&plan);
    solver_destroy(&s);
    sqlite3_close(db);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_solver_diamond_dependency);
    RUN_TEST(test_solver_missing_package);
    return UNITY_END();
}
