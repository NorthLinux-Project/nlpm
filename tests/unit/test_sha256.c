#include "unity.h"
#include "crypto/sha256.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_sha256_empty_string(void) {
    char hex[65];
    sha256_buffer("", 0, hex);
    TEST_ASSERT_EQUAL_STRING("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hex);
}

void test_sha256_standard_vector(void) {
    char hex[65];
    const char *msg = "The quick brown fox jumps over the lazy dog";
    sha256_buffer(msg, strlen(msg), hex);
    TEST_ASSERT_EQUAL_STRING("d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592", hex);
}

void test_sha256_file(void) {
    FILE *fp = fopen("/tmp/test_sha256_file.txt", "wb");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("North Linux", fp);
    fclose(fp);

    char hex_file[65];
    char hex_buf[65];
    TEST_ASSERT_TRUE(sha256_file("/tmp/test_sha256_file.txt", hex_file));
    sha256_buffer("North Linux", 11, hex_buf);
    TEST_ASSERT_EQUAL_STRING(hex_buf, hex_file);
    remove("/tmp/test_sha256_file.txt");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sha256_empty_string);
    RUN_TEST(test_sha256_standard_vector);
    RUN_TEST(test_sha256_file);
    return UNITY_END();
}
