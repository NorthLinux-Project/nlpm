#ifndef UNITY_INTERNALS_H
#define UNITY_INTERNALS_H

#include <stdio.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*UnityTestFunction)(void);

typedef struct {
    const char *TestFile;
    const char *CurrentTestName;
    size_t CurrentTestLineNumber;
    size_t NumberOfTests;
    size_t TestFailures;
    size_t TestIgnores;
    size_t CurrentTestFailed;
    size_t CurrentTestIgnored;
    jmp_buf AbortFrame;
} UnityStorageType;

extern UnityStorageType Unity;

void UnityBegin(const char *filename);
int UnityEnd(void);
void UnityConcludeTest(void);
void UnityDefaultTestRun(UnityTestFunction Func, const char *FuncName, const int FuncLineNum);

void UnityAssertEqualNumber(int64_t expected, int64_t actual, const char *msg, size_t line);
void UnityAssertEqualString(const char *expected, const char *actual, const char *msg, size_t line);
void UnityAssertEqualMemory(const void *expected, const void *actual, size_t length, const char *msg, size_t line);
void UnityAssertTrue(int condition, const char *msg, size_t line);
void UnityAssertFalse(int condition, const char *msg, size_t line);
void UnityAssertNull(const void *ptr, const char *msg, size_t line);
void UnityAssertNotNull(const void *ptr, const char *msg, size_t line);
void UnityFail(const char *msg, size_t line);

#ifdef __cplusplus
}
#endif

#endif
