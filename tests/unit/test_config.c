#include "unity.h"
#include "config/config.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_config_defaults(void) {
    NlpmConfig cfg;
    TEST_ASSERT_TRUE(config_load_string("", &cfg));
    TEST_ASSERT_EQUAL_STRING("/", cfg.rootdir);
    TEST_ASSERT_EQUAL_STRING("/var/cache/nlpm", cfg.cachedir);
    TEST_ASSERT_EQUAL_STRING("/var/lib/nlpm", cfg.dbdir);
    TEST_ASSERT_EQUAL_INT(4, cfg.parallel);
    TEST_ASSERT_EQUAL_UINT(0, cfg.repo_count);
    config_free(&cfg);
}

void test_config_custom_values(void) {
    const char *toml_src =
        "[general]\n"
        "rootdir = \"/mnt/sysroot\"\n"
        "cachedir = \"/tmp/cache\"\n"
        "parallel = 8\n"
        "\n"
        "[[repo]]\n"
        "name = \"core\"\n"
        "url = \"https://pkg.northlinux.org/core\"\n"
        "\n"
        "[[repo]]\n"
        "name = \"extra\"\n"
        "url = \"https://pkg.northlinux.org/extra\"\n";

    NlpmConfig cfg;
    TEST_ASSERT_TRUE(config_load_string(toml_src, &cfg));
    TEST_ASSERT_EQUAL_STRING("/mnt/sysroot", cfg.rootdir);
    TEST_ASSERT_EQUAL_STRING("/tmp/cache", cfg.cachedir);
    TEST_ASSERT_EQUAL_INT(8, cfg.parallel);
    TEST_ASSERT_EQUAL_UINT(2, cfg.repo_count);
    TEST_ASSERT_EQUAL_STRING("core", cfg.repos[0].name);
    TEST_ASSERT_EQUAL_STRING("https://pkg.northlinux.org/core", cfg.repos[0].url);
    TEST_ASSERT_TRUE(cfg.repos[0].enabled);
    TEST_ASSERT_EQUAL_STRING("extra", cfg.repos[1].name);
    config_free(&cfg);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_config_custom_values);
    return UNITY_END();
}
