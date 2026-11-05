// 与 lib.cpp 相同的包含前奏：给 reflect_gen.cpp 等独立编译单元复用。
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <oleauto.h>
#include <lib2.h>
#include <lang.h>

#ifndef interface
#define interface __interface
#endif
// 库常量表用的系统掩码（lib2.h 只给了 _CMD_OS / _PROP_OS / _EVENT_OS / _DT_OS）。
#ifndef _CONST_OS
#define _CONST_OS(os) ((os) >> 16)
#endif
#include <WebView2.h>

// 放最后：里面会把 snprintf 宏掉，不能被后面的头文件重新定义回去。
#include "wv_format.h"
