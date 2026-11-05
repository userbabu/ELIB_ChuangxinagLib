// 不依赖 C 运行库的格式化与小工具。
// 易语言静态编译收尾时链接的是它自己那套运行时，UCRT / libcpmt 都不在符号表里：
// 用 snprintf 会留下 __stdio_common_vsprintf，用 new/delete 会留下 ??2/??3，全部链接不上。
// 所以这里自带一份只支持本库用到的格式符的实现，走 user32!wsprintfA + 进程堆。
#pragma once

#ifndef WV_NOFORMAT
#define WV_NOFORMAT

#include <stdarg.h>

namespace wv {

int WvSnprintf(char* dst, int cap, const char* fmt, ...);

// %g 的替代：能整数就整数，否则定点 6 位并去掉尾零。
inline int WvFmtDouble(char* buf, int cap, double v)
{
    if (buf == NULL || cap <= 1) return 0;
    if (v == 0.0) { buf[0] = '0'; buf[1] = '\0'; return 1; }

    const double av = (v < 0) ? -v : v;
    char tmp[64];
    int n = 0;
    if (v < 0) { tmp[n++] = '-'; }
    const long ip = (long)av;
    if (av == (double)ip && av < 2e9)
    {
        n += WvSnprintf(tmp + n, (int)sizeof(tmp) - n, "%d", ip);
    }
    else
    {
        n += WvSnprintf(tmp + n, (int)sizeof(tmp) - n, "%d.", ip);
        double frac = av - (double)ip;
        for (int i = 0; i < 6 && n < (int)sizeof(tmp) - 2; i++)
        {
            frac *= 10.0;
            const int d = (int)frac;
            tmp[n++] = (char)('0' + d);
            frac -= d;
        }
        while (n > 1 && tmp[n - 1] == '0' && tmp[n - 2] != '.') n--;
    }
    tmp[n] = '\0';
    return WvSnprintf(buf, cap, "%s", tmp);
}

// 只认本库用到的：%s %c %d %i %u %X %I64d %g %f %%；返回写入长度（不含结尾 0）。
// 注意：这里不能出现 double <-> __int64 的转换，MSVC 会调老 CRT 的 __dtol3 / __ltod3，
// 易语言静态编译那套链接库里没有它们（LNK2001）。整数一律走 32 位 long。
inline int WvSnprintf(char* dst, int cap, const char* fmt, ...)
{
    if (dst == NULL || cap <= 0 || fmt == NULL) return 0;
    dst[0] = '\0';

    va_list ap;
    va_start(ap, fmt);
    int at = 0;
    char num[64];

    for (int i = 0; fmt[i] && at < cap - 1; i++)
    {
        if (fmt[i] != '%')
        {
            dst[at++] = fmt[i];
            dst[at] = '\0';
            continue;
        }
        i++;
        if (fmt[i] == '\0') break;

        int len = 0;
        const char* s = NULL;
        num[0] = '\0';

        if (fmt[i] == '%') { num[0] = '%'; len = 1; }
        else if (fmt[i] == 's')
        {
            s = va_arg(ap, const char*);
            if (s == NULL) s = "(null)";
            len = 0;
            while (s[len] && len < (int)sizeof(num) - 1) { num[len] = s[len]; len++; }
        }
        else if (fmt[i] == 'c')
        {
            num[0] = (char)va_arg(ap, int);
            len = 1;
        }
        else if (fmt[i] == 'd' || fmt[i] == 'i')
        {
            len = wsprintfA(num, "%d", va_arg(ap, int));
        }
        else if (fmt[i] == 'u')
        {
            len = wsprintfA(num, "%u", (unsigned)va_arg(ap, unsigned));
        }
        else if (fmt[i] == 'X')
        {
            len = wsprintfA(num, "%X", (unsigned)va_arg(ap, unsigned));
        }
        else if (fmt[i] == 'I')
        {
            if (fmt[i + 1] == '6' && fmt[i + 2] == '4' && fmt[i + 3] == 'd')
            {
                const long long v64 = va_arg(ap, long long);
                i += 3;
                len = wsprintfA(num, "%I64d", v64);
            }
        }
        else if (fmt[i] == 'g' || fmt[i] == 'f')
        {
            const double d = va_arg(ap, double);
            len = WvFmtDouble(num, (int)sizeof(num), d);
        }

        if (len > 0)
        {
            for (int k = 0; k < len && at < cap - 1; k++) dst[at++] = num[k];
            dst[at] = '\0';
        }
    }
    va_end(ap);
    return at;
}

}  // namespace wv

// 全局替换：所有编译单元（含生成器产物）里的 snprintf 都落到上面这份实现。
#define snprintf  wv::WvSnprintf
#define _snprintf wv::WvSnprintf

#endif  // WV_NOFORMAT
