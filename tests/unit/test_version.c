#include "unity.h"
#include "util/version.h"

void setUp(void) {}
void tearDown(void) {}

void test_version_parse_simple(void) {
    Version v = {0};
    TEST_ASSERT_TRUE(version_parse("1.2.3", &v));
    TEST_ASSERT_EQUAL_UINT(0, v.epoch);
    TEST_ASSERT_EQUAL_UINT(1, v.major);
    TEST_ASSERT_EQUAL_UINT(2, v.minor);
    TEST_ASSERT_EQUAL_UINT(3, v.patch);
    TEST_ASSERT_NULL(v.prerelease);
    version_free(&v);
}

void test_version_parse_epoch(void) {
    Version v = {0};
    TEST_ASSERT_TRUE(version_parse("2:8.4.1", &v));
    TEST_ASSERT_EQUAL_UINT(2, v.epoch);
    TEST_ASSERT_EQUAL_UINT(8, v.major);
    TEST_ASSERT_EQUAL_UINT(4, v.minor);
    TEST_ASSERT_EQUAL_UINT(1, v.patch);
    version_free(&v);
}

void test_version_comparison_epoch_precedence(void) {
    TEST_ASSERT_TRUE(version_cmp("1:1.0.0", "9.9.9") > 0);
    TEST_ASSERT_TRUE(version_cmp("0:2.0.0", "1:1.0.0") < 0);
}

void test_version_comparison_semver(void) {
    TEST_ASSERT_TRUE(version_cmp("1.2.3", "1.2.3") == 0);
    TEST_ASSERT_TRUE(version_cmp("1.2.4", "1.2.3") > 0);
    TEST_ASSERT_TRUE(version_cmp("1.3.0", "1.2.9") > 0);
    TEST_ASSERT_TRUE(version_cmp("2.0.0", "1.99.99") > 0);
}

void test_version_constraints(void) {
    TEST_ASSERT_TRUE(version_satisfies_str("1.2.3", ">=1.2.0"));
    TEST_ASSERT_TRUE(version_satisfies_str("1.2.3", "<2.0.0"));
    TEST_ASSERT_TRUE(version_satisfies_str("1.2.3", "==1.2.3"));
    TEST_ASSERT_FALSE(version_satisfies_str("1.2.3", ">1.2.3"));
    TEST_ASSERT_FALSE(version_satisfies_str("0.9.0", ">=1.0.0"));
    TEST_ASSERT_TRUE(version_satisfies_str("1.2.3", "*"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_version_parse_simple);
    RUN_TEST(test_version_parse_epoch);
    RUN_TEST(test_version_comparison_epoch_precedence);
    RUN_TEST(test_version_comparison_semver);
    RUN_TEST(test_version_constraints);
    return UNITY_END();
}
