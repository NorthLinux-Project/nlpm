#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#include "unity_internals.h"

#ifdef __cplusplus
extern "C" {
#endif

void setUp(void);
void tearDown(void);

#define UNITY_BEGIN() UnityBegin(__FILE__)
#define UNITY_END()   UnityEnd()

#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

#define TEST_ASSERT_TRUE(condition)            UnityAssertTrue((condition) ? 1 : 0, #condition, __LINE__)
#define TEST_ASSERT_FALSE(condition)           UnityAssertFalse((condition) ? 1 : 0, #condition, __LINE__)
#define TEST_ASSERT_NULL(ptr)                  UnityAssertNull((const void*)(ptr), #ptr, __LINE__)
#define TEST_ASSERT_NOT_NULL(ptr)              UnityAssertNotNull((const void*)(ptr), #ptr, __LINE__)
#define TEST_ASSERT_EQUAL_INT(exp, act)        UnityAssertEqualNumber((int64_t)(exp), (int64_t)(act), #act, __LINE__)
#define TEST_ASSERT_EQUAL_UINT(exp, act)       UnityAssertEqualNumber((int64_t)(exp), (int64_t)(act), #act, __LINE__)
#define TEST_ASSERT_EQUAL_INT64(exp, act)      UnityAssertEqualNumber((int64_t)(exp), (int64_t)(act), #act, __LINE__)
#define TEST_ASSERT_EQUAL_STRING(exp, act)     UnityAssertEqualString((exp), (act), #act, __LINE__)
#define TEST_ASSERT_EQUAL_MEMORY(exp, act, len) UnityAssertEqualMemory((exp), (act), (len), #act, __LINE__)
#define TEST_FAIL_MESSAGE(msg)                 UnityFail((msg), __LINE__)

#ifdef __cplusplus
}
#endif

#endif
