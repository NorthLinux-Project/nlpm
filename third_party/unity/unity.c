#include "unity.h"
#include <string.h>
#include <inttypes.h>

UnityStorageType Unity;

void UnityBegin(const char *filename) {
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

int UnityEnd(void) {
    printf("-----------------------\n");
    printf("%zu Tests %zu Failures %zu Ignored\n",
           Unity.NumberOfTests, Unity.TestFailures, Unity.TestIgnores);
    if (Unity.TestFailures == 0) {
        printf("OK\n");
        return 0;
    }
    printf("FAIL\n");
    return (int)Unity.TestFailures;
}

void UnityDefaultTestRun(UnityTestFunction Func, const char *FuncName, const int FuncLineNum) {
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (size_t)FuncLineNum;
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    if (setjmp(Unity.AbortFrame) == 0) {
        setUp();
        Func();
    }
    if (setjmp(Unity.AbortFrame) == 0) {
        tearDown();
    }
    if (Unity.CurrentTestFailed) {
        Unity.TestFailures++;
    } else {
        printf("%s:%d:%s:PASS\n", Unity.TestFile, FuncLineNum, FuncName);
    }
}

void UnityFail(const char *msg, size_t line) {
    Unity.CurrentTestFailed = 1;
    printf("%s:%zu:%s:FAIL:%s\n", Unity.TestFile, line, Unity.CurrentTestName, msg);
    longjmp(Unity.AbortFrame, 1);
}

void UnityAssertTrue(int condition, const char *msg, size_t line) {
    if (!condition) {
        UnityFail(msg, line);
    }
}

void UnityAssertFalse(int condition, const char *msg, size_t line) {
    if (condition) {
        UnityFail(msg, line);
    }
}

void UnityAssertNull(const void *ptr, const char *msg, size_t line) {
    if (ptr != NULL) {
        UnityFail(msg, line);
    }
}

void UnityAssertNotNull(const void *ptr, const char *msg, size_t line) {
    if (ptr == NULL) {
        UnityFail(msg, line);
    }
}

void UnityAssertEqualNumber(int64_t expected, int64_t actual, const char *msg, size_t line) {
    if (expected != actual) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s Expected %" PRId64 " Was %" PRId64, msg, expected, actual);
        UnityFail(buf, line);
    }
}

void UnityAssertEqualString(const char *expected, const char *actual, const char *msg, size_t line) {
    if (expected == NULL && actual == NULL) return;
    if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s Expected '%s' Was '%s'", msg, expected ? expected : "NULL", actual ? actual : "NULL");
        UnityFail(buf, line);
    }
}

void UnityAssertEqualMemory(const void *expected, const void *actual, size_t length, const char *msg, size_t line) {
    if (expected == NULL && actual == NULL) return;
    if (expected == NULL || actual == NULL || memcmp(expected, actual, length) != 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s Memory mismatch for length %zu", msg, length);
        UnityFail(buf, line);
    }
}
