// 编译前置：SDK 的 lib2.h 需要 DATE/DOUBLE/FLOAT/INT64，而它的 mtypes.h 与 windows.h 冲突，
// 因此这里手工补四个 typedef（与 lib.cpp 保持同一套）。
#ifndef CX_PCH_H
#define CX_PCH_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

#endif  // CX_PCH_H
