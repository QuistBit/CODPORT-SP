/* Test-only stand-in, never used in the Windows build. */
#ifndef R31_TEST_WINDOWS_H
#define R31_TEST_WINDOWS_H
#include <stdint.h>
typedef struct {int64_t QuadPart;} LARGE_INTEGER;
static int QueryPerformanceCounter(LARGE_INTEGER *q);
static int QueryPerformanceFrequency(LARGE_INTEGER *q);
#endif
