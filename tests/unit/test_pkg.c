#include "unity.h"
#include "pkg/pkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

void test_pkg_build_and_read(void) {
    mkdir("/tmp/test_nlpkg_data", 0755);
    mkdir("/tmp/test_nlpkg_data/usr", 0755);
    mkdir("/tmp/test_nlpkg_data/usr/bin", 0755);

    FILE *bin = fopen("/tmp/test_nlpkg_data/usr/bin/hello", "wb");
    TEST_ASSERT_NOT_NULL(bin);
    fputs("#!/bin/sh\necho Hello North Linux\n", bin);
    fclose(bin);
    chmod("/tmp/test_nlpkg_data/usr/bin/hello", 0755);

    const char *manifest =
        "name = \"hello\"\n"
        "version = \"1.0.0\"\n"
        "desc = \"North Linux Hello Program\"\n"
        "url = \"https://northlinux.org\"\n"
        "arch = \"x86_64\"\n"
        "isize = 1024\n"
        "[deps]\n"
        "depend = [\"glibc>=2.33\", \"bash\"]\n";

    FILE *mf = fopen("/tmp/test_nlpkg_manifest.toml", "wb");
    TEST_ASSERT_NOT_NULL(mf);
    fputs(manifest, mf);
    fclose(mf);

    const char *pkg_file = "/tmp/test_hello-1.0.0-x86_64.nlpkg";
    TEST_ASSERT_TRUE(pkg_build_archive("/tmp/test_nlpkg_manifest.toml", "/tmp/test_nlpkg_data", pkg_file));

    NlpkgMeta meta = {0};
    TEST_ASSERT_TRUE(pkg_read_meta(pkg_file, &meta));
    TEST_ASSERT_EQUAL_STRING("hello", meta.name);
    TEST_ASSERT_EQUAL_STRING("1.0.0", meta.version);
    TEST_ASSERT_EQUAL_STRING("North Linux Hello Program", meta.desc);
    TEST_ASSERT_EQUAL_UINT(2, meta.dep_count);
    TEST_ASSERT_EQUAL_STRING("glibc", meta.deps[0].name);
    TEST_ASSERT_EQUAL_STRING(">=2.33", meta.deps[0].constraint);
    TEST_ASSERT_EQUAL_STRING("bash", meta.deps[1].name);
    TEST_ASSERT_NOT_NULL(meta.sha256);
    TEST_ASSERT_EQUAL_UINT(64, strlen(meta.sha256));
    pkg_meta_free(&meta);

    NlpkgFileEntry *files = NULL;
    size_t file_count = 0;
    TEST_ASSERT_TRUE(pkg_read_file_list(pkg_file, &files, &file_count));
    TEST_ASSERT_TRUE(file_count > 0);

    bool found_bin = false;
    for (size_t i = 0; i < file_count; ++i) {
        if (strcmp(files[i].path, "usr/bin/hello") == 0) {
            found_bin = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_bin);
    pkg_file_entries_free(files, file_count);

    unlink("/tmp/test_nlpkg_data/usr/bin/hello");
    rmdir("/tmp/test_nlpkg_data/usr/bin");
    rmdir("/tmp/test_nlpkg_data/usr");
    rmdir("/tmp/test_nlpkg_data");
    unlink("/tmp/test_nlpkg_manifest.toml");
    unlink(pkg_file);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pkg_build_and_read);
    return UNITY_END();
}
