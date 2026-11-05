// 反射层的胶水：组名解析与成员名列表拼接。
#include "reflect.h"
#include <string.h>
#include <stdio.h>

namespace wv {

void ReflJoinNames(char* buf, INT cap, const char** items, INT count)
{
    if (buf == NULL || cap <= 0) return;
    buf[0] = '\0';
    INT used = 0;
    for (INT i = 0; i < count; i++)
    {
        const INT n = (INT)lstrlenA(items[i]);
        if (used + n + 2 > cap) break;
        lstrcpyA(buf + used, items[i]);
        used += n;
        buf[used++] = '\n';
        buf[used] = '\0';
    }
}

// 组名同时收中文、英文与生成器里的组标识，大小写不敏感
BOOL ReflTargetFromName(LPCSTR szName, WvReflTarget* pOut)
{
    if (szName == NULL || pOut == NULL) return FALSE;
    struct Row { LPCSTR cn; LPCSTR en; WvReflTarget t; };
    static const Row rows[] = {
        { "内核",           "core",         RF_T_CORE },
        { "设置",           "settings",     RF_T_SETTINGS },
        { "控制器",         "controller",   RF_T_CONTROLLER },
        { "环境",           "env",          RF_T_ENV },
        { "环境选项",       "options",      RF_T_OPTIONS },
        { "Cookie管理器",   "cookiemanage", RF_T_COOKIEMGR },
        { "Cookie",         "cookie",       RF_T_COOKIE },
        { "配置文件",       "profile",      RF_T_PROFILE },
        { "框架",           "frame",        RF_T_FRAME },
        { "下载",           "download",     RF_T_DOWNLOAD },
        { "请求",           "request",      RF_T_REQUEST },
        { "响应",           "response",     RF_T_RESPONSE },
        { "进程信息",       "processinfo",  RF_T_PROCESSINFO },
        { "协议事件接收器", "receiver",     RF_T_RECEIVER },
        { "证书",           "cert",         RF_T_CERT },
        { "事件对象",       "event",        RF_T_EVENT },
    };
    for (INT i = 0; i < (INT)(sizeof(rows) / sizeof(rows[0])); i++)
    {
        if (_stricmp(szName, rows[i].en) == 0 || lstrcmpA(szName, rows[i].cn) == 0)
        {
            *pOut = rows[i].t;
            return TRUE;
        }
    }
    return FALSE;
}

LPCSTR ReflTargetName(WvReflTarget t)
{
    switch (t)
    {
    case RF_T_CORE: return "core";
    case RF_T_SETTINGS: return "settings";
    case RF_T_CONTROLLER: return "controller";
    case RF_T_ENV: return "env";
    case RF_T_OPTIONS: return "options";
    case RF_T_COOKIEMGR: return "cookiemanage";
    case RF_T_COOKIE: return "cookie";
    case RF_T_PROFILE: return "profile";
    case RF_T_FRAME: return "frame";
    case RF_T_DOWNLOAD: return "download";
    case RF_T_REQUEST: return "request";
    case RF_T_RESPONSE: return "response";
    case RF_T_PROCESSINFO: return "processinfo";
    case RF_T_RECEIVER: return "receiver";
    case RF_T_CERT: return "cert";
    case RF_T_EVENT: return "event";
    default: return "?";
    }
}

// 事件带出来的对象（帧 / 下载操作 / 请求 / 响应 / 进程信息…）事先不知道是哪一族，逐组试。
// 注意顺序：COOKIE 要排在 COOKIEMGR 前？不需要——各组成员名不重叠时才准，重叠时按此顺序取胜者。
int ReflDispatchAny(IUnknown* pObj, INT mode, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText,
                    INT* outInt, DOUBLE* outDouble, LPSTR outText, INT cap, WvReflTarget* pHitGroup)
{
    if (pObj == NULL || name == NULL) return RF_ERR;

    static const WvReflTarget order[] = {
        RF_T_CORE, RF_T_SETTINGS, RF_T_CONTROLLER, RF_T_ENV, RF_T_OPTIONS,
        RF_T_COOKIEMGR, RF_T_COOKIE, RF_T_PROFILE, RF_T_FRAME, RF_T_DOWNLOAD,
        RF_T_REQUEST, RF_T_RESPONSE, RF_T_PROCESSINFO, RF_T_RECEIVER, RF_T_CERT,
    };
    for (INT i = 0; i < (INT)(sizeof(order) / sizeof(order[0])); i++)
    {
        const int rc = ReflDispatch(order[i], pObj, mode, name, inInt, inDouble, inText,
                                    outInt, outDouble, outText, cap);
        if (rc != RF_MISS)
        {
            if (pHitGroup != NULL) *pHitGroup = order[i];
            return rc;
        }
    }
    return RF_MISS;
}

}  // namespace wv
