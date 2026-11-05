// 静态编译补洞：易语言静态编译用的是自带的 VC6 链接器 + VC6 CRT，
// 现代工具链在这里留不下符号的几个入口必须我们自己给定义，否则 LNK2001。
// 只在 __E_STATIC_LIB 分支参与编译，动态 .fne 走 MSVC CRT 不需要它们。
// 语义逐字照 MSVC 头文件（xutility / vcruntime_exception.h）：找不到一律回 _Last。
#ifdef __E_STATIC_LIB

#include <stdlib.h>
#include <string.h>

// MSVC 为 AVX2 版本的宽字符内存例程留的特性开关（.drectve 里的 /alternatename
// 已被 strip_drectve.mjs 抹掉，引用会悬空）。置 0 就走普通实现。
extern "C" { int _Avx2WmemEnabled = 0; }

// ---- <algorithm> / std::string::find 的平凡类型加速入口 ----
extern "C" const void* __stdcall __std_find_trivial_1(
    const void* _First, const void* _Last, unsigned char _Val)
{
    const unsigned char* p = (const unsigned char*)_First;
    const unsigned char* e = (const unsigned char*)_Last;
    for (; p < e; ++p) { if (*p == _Val) return p; }
    return e;
}

extern "C" const void* __stdcall __std_find_trivial_2(
    const void* _First, const void* _Last, unsigned short _Val)
{
    const unsigned short* p = (const unsigned short*)_First;
    const unsigned short* e = (const unsigned short*)_Last;
    for (; p < e; ++p) { if (*p == _Val) return p; }
    return e;
}

extern "C" const void* __stdcall __std_find_trivial_4(
    const void* _First, const void* _Last, unsigned int _Val)
{
    const unsigned int* p = (const unsigned int*)_First;
    const unsigned int* e = (const unsigned int*)_Last;
    for (; p < e; ++p) { if (*p == _Val) return p; }
    return e;
}

extern "C" const void* __stdcall __std_search_2(
    const void* _First1, const void* _Last1, const void* _First2, size_t _Count2)
{
    const unsigned char* hay = (const unsigned char*)_First1;
    const unsigned char* end = (const unsigned char*)_Last1;
    const unsigned char* nee = (const unsigned char*)_First2;
    if (_Count2 == 0) return hay;
    const size_t n = (size_t)(end - hay);
    if (n < _Count2) return end;
    for (size_t i = 0; i + _Count2 <= n; ++i) {
        if (memcmp(hay + i, nee, _Count2) == 0) return hay + i;
    }
    return end;
}

// ---- std::exception 携带的 what 串 ----
// 布局必须与 <vcruntime_exception.h> 的 __std_exception_data 一致。
struct CxStdExceptionData { const char* _What; int _DoFree; };

extern "C" void __cdecl __std_exception_copy(const CxStdExceptionData* _From, CxStdExceptionData* _To)
{
    if (_From == _To) return;
    _To->_What = _From->_What;
    _To->_DoFree = 0;
    if (_From->_DoFree && _From->_What != NULL) {
        const size_t len = strlen(_From->_What) + 1;
        char* copy = (char*)malloc(len);
        if (copy != NULL) { memcpy(copy, _From->_What, len); _To->_What = copy; _To->_DoFree = 1; }
    }
}

extern "C" void __cdecl __std_exception_destroy(CxStdExceptionData* _Data)
{
    if (_Data->_DoFree && _Data->_What != NULL) free((void*)_Data->_What);
    _Data->_What = NULL;
    _Data->_DoFree = 0;
}

#endif  // __E_STATIC_LIB
