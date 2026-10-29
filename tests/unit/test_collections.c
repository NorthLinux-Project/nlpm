#include "unity.h"
#include "util/stretchy_buffer.h"
#include "util/hash_map.h"

void setUp(void) {}
void tearDown(void) {}

void test_stretchy_buffer_push_pop(void) {
    int *nums = NULL;
    for (int i = 0; i < 50; ++i) {
        sb_push(nums, i * 2);
    }
    TEST_ASSERT_EQUAL_UINT(50, sb_count(nums));
    TEST_ASSERT_EQUAL_INT(0, nums[0]);
    TEST_ASSERT_EQUAL_INT(98, sb_last(nums));

    int last = sb_pop(nums);
    TEST_ASSERT_EQUAL_INT(98, last);
    TEST_ASSERT_EQUAL_UINT(49, sb_count(nums));

    sb_free(nums);
    TEST_ASSERT_NULL(nums);
}

void test_hash_map_crud(void) {
    HashMap map = hash_map_create(8);
    TEST_ASSERT_TRUE(hash_map_set(&map, "kernel", 100));
    TEST_ASSERT_TRUE(hash_map_set(&map, "systemd", 200));

    uintptr_t val = 0;
    TEST_ASSERT_TRUE(hash_map_get(&map, "kernel", &val));
    TEST_ASSERT_EQUAL_UINT(100, val);

    TEST_ASSERT_TRUE(hash_map_get(&map, "systemd", &val));
    TEST_ASSERT_EQUAL_UINT(200, val);

    TEST_ASSERT_FALSE(hash_map_get(&map, "nonexistent", &val));

    TEST_ASSERT_TRUE(hash_map_delete(&map, "kernel"));
    TEST_ASSERT_FALSE(hash_map_get(&map, "kernel", &val));

    hash_map_destroy(&map);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stretchy_buffer_push_pop);
    RUN_TEST(test_hash_map_crud);
    return UNITY_END();
}
