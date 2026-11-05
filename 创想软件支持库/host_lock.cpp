// host_lock：线程锁族的并发压力与误用验证（离线宿主，不依赖易语言 IDE）。
// 直接 LoadLibrary 我们的 .fne，用 m_pCmdsFunc 驱动命令，多线程真跑。
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

#define AS_TEXT(p) ((const char*)(p))

typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);
static PLIB_INFO g_inf = NULL;
static int g_failed = 0, g_mallocCnt = 0, g_freeCnt = 0, g_rtErr = 0;
static LONG g_rtErrSup = 0;
static int g_part = 0;      // 预期内的误用：计数但不计红

static DWORD WINAPI FakeNotifySys(INT nMsg, DWORD dwEAX, DWORD dwECX)
{
    switch (nMsg)
    {
    case NRS_MALLOC: g_mallocCnt++; return (DWORD)malloc(dwEAX == 0 ? 1 : dwEAX);
    case NRS_MFREE: case NRS_FREE_ARY:
        if (nMsg == NRS_FREE_ARY) { g_freeCnt++; return 0; }
        if ((void*)dwEAX != NULL) free((void*)dwEAX);
        g_freeCnt++;
        return 0;
    case NRS_RUNTIME_ERR:
        if (InterlockedIncrement((LPLONG)&g_rtErr) && g_rtErrSup == 0) { printf("    [运行时错误][小节 %d] %s\n", g_part, (LPCSTR)dwEAX); g_failed++; }
        return 0;
    default: break;
    }
    return 0;
}
static void Fail(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("[FAIL] "); vprintf(fmt, ap); printf("\n"); va_end(ap);
    g_failed++;
}
typedef struct { MDATA_INF self; INT slot; } Obj;
static void ObjInit(Obj* o) { memset(o, 0, sizeof(*o)); o->self.m_dtDataType = SDT_INT; o->self.m_pCompoundData = (void*)&o->slot; }
static INT FindCmd(INT typeNo, LPCSTR nm)
{
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[typeNo];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        INT idx = d->m_pnCmdsIndex[i];
        if (lstrcmpA((LPCSTR)g_inf->m_pBeginCmdInfo[idx].m_szName, nm) == 0) return idx;
    }
    Fail("类型 %d 里找不到命令 %s", typeNo + 1, nm);
    return -1;
}
static MDATA_INF A(int v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_INT; m.m_int = v; return m; }
static void Call(INT cmd, MDATA_INF* args, int n, MDATA_INF* ret)
{
    memset(ret, 0, sizeof(*ret));
    g_inf->m_pCmdsFunc[cmd](ret, n, args);
}
struct Ctx { INT cmdEnter, cmdLeave, cmdRec, cmdOwner; LONG* pSum; int nRounds; INT cmdWait; DWORD msWait; INT cmdSetH; INT hObj; INT cmdWaiters; };

static DWORD WINAPI Worker(LPVOID p)
{
    Ctx* c = (Ctx*)p;
    Obj o; ObjInit(&o);
    MDATA_INF r;
    MDATA_INF a1[2] = { o.self, A(c->hObj) };
    Call(c->cmdSetH, a1, 2, &r);                     // 绑到主线程那把锁（与 易语言 里 置句柄 同一路）
    a1[1] = A(c->msWait);
    Call(c->cmdEnter, a1, 2, &r);                    // 每个线程都要拿到锁
    if (r.m_bool != BL_TRUE) { Fail("工作线程 进入 失败"); return 1; }
    for (int i = 0; i < c->nRounds; i++)
    {
        InterlockedAdd(c->pSum, 0);               // 只在锁内改共享量（下面模拟非原子累加）
        *c->pSum += 1;                            // 故意非原子：靠锁保证正确
    }
    MDATA_INF a0[1] = { o.self };
    Call(c->cmdLeave, a0, 1, &r);
    if (r.m_bool != BL_TRUE) Fail("工作线程 离开 失败");
    return 0;
}
// 等待者：在锁上条件等待
static DWORD WINAPI Waiter(LPVOID p)
{
    Ctx* c = (Ctx*)p;
    Obj o; ObjInit(&o);
    MDATA_INF r, a[2] = { o.self, A(c->hObj) };
    Call(c->cmdSetH, a, 2, &r);
    a[1] = A(-1);
    Call(c->cmdEnter, a, 2, &r);
    a[1] = A(3000);
    Call(c->cmdWait, a, 2, &r);                    // 内部会放锁再拿回
    const INT nWait = r.m_bool;
    Call(c->cmdLeave, a, 1, &r);
    return nWait == BL_TRUE ? 0 : 1;
}
static int nThreads = 8, nRounds = 20000;
static LONG g_sum = 0;

// 省略超时毫秒：易语言把可空实参标成 _SDT_NULL + 0，必须等于「一直等」，不能当成 0 毫秒立即超时
struct HoldCtx { INT cmdEnter, cmdLeave, cmdSetH; INT hObj; volatile LONG nAcquired; };
static MDATA_INF EmptyInt() { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = _SDT_NULL; m.m_int = 0; return m; }
static DWORD WINAPI OmitWaiter(LPVOID p)
{
    HoldCtx* c = (HoldCtx*)p;
    Obj o; ObjInit(&o);
    MDATA_INF r, a[2] = { o.self, A(c->hObj) };
    Call(c->cmdSetH, a, 2, &r);
    a[1] = EmptyInt();                                   // ← 相当于易语言里写 进入 ()
    Call(c->cmdEnter, a, 2, &r);
    if (r.m_bool != BL_TRUE) return 1;
    InterlockedIncrement((LPLONG)&c->nAcquired);
    a[1] = A(-1);
    Call(c->cmdLeave, a, 1, &r);
    return 0;
}
// 传 0 的探测必须由「另一个线程」来做：同线程重入是直接被允许的，测不出超时
static DWORD WINAPI ProbeZero(LPVOID p)
{
    HoldCtx* c = (HoldCtx*)p;
    Obj o; ObjInit(&o);
    MDATA_INF r, a[2] = { o.self, A(c->hObj) };
    Call(c->cmdSetH, a, 2, &r);
    a[1] = A(0);
    const DWORD t0 = GetTickCount();
    InterlockedExchange((LPLONG)&c->nAcquired, (LONG)(GetTickCount() - t0));   // 借这个字段回传耗时
    Call(c->cmdEnter, a, 2, &r);
    InterlockedExchange((LPLONG)&c->nAcquired, (LONG)(GetTickCount() - t0));
    return r.m_bool == BL_TRUE ? 1 : 0;                 // 拿到锁才算错
}

struct JoinCommands { INT create, wait, cancel, destroy, arrived, complete, cancelled, construct, copy, destruct; };
struct JoinWorkerCtx { Obj* obj; INT wait; LONG returned; LONG elected; };
static DWORD WINAPI JoinWorker(LPVOID arg)
{
    JoinWorkerCtx* c = (JoinWorkerCtx*)arg;
    MDATA_INF r, a[1] = { c->obj->self };
    Call(c->wait, a, 1, &r);
    InterlockedExchange(&c->elected, r.m_bool == BL_TRUE ? 1 : 0);
    InterlockedExchange(&c->returned, 1);
    return 0;
}
static int JoinCall(INT cmd, Obj* obj, int value = 0, bool withValue = false)
{
    MDATA_INF r, a[2] = { obj->self, A(value) };
    Call(cmd, a, withValue ? 2 : 1, &r);
    return r.m_int;
}
static void JoinStart(JoinWorkerCtx* ctx, HANDLE* threads, Obj* obj, INT wait, int from, int to)
{
    for (int i = from; i < to; i++)
    {
        ctx[i].obj = obj; ctx[i].wait = wait; ctx[i].returned = 0; ctx[i].elected = 0;
        threads[i] = CreateThread(NULL, 0, JoinWorker, &ctx[i], 0, NULL);
        if (!threads[i]) { Fail("会合验证创建线程失败"); ExitProcess(2); }
    }
}
static void JoinCheckArrival(JoinCommands* c, Obj* obj, int count)
{
    DWORD begin = GetTickCount();
    while (JoinCall(c->arrived, obj) != count)
    {
        if (GetTickCount() - begin > 3000)
        {
            Fail("会合线程未全部到达：期望 %d", count);
            JoinCall(c->cancel, obj);
            ExitProcess(2);
        }
        Sleep(1);
    }
}
static int JoinCollect(JoinWorkerCtx* ctx, HANDLE* threads, int n)
{
    if (WaitForMultipleObjects(n, threads, TRUE, 3000) != WAIT_OBJECT_0)
    {
        Fail("线程会合没有在到齐/取消后结束"); ExitProcess(2);
    }
    int elected = 0;
    for (int i = 0; i < n; i++) { elected += ctx[i].elected; CloseHandle(threads[i]); }
    return elected;
}
static void TestJoin()
{
    g_part = 13;
    const int type = 23;
    if (g_inf->m_nDataTypeCount <= type) { Fail("缺少线程会合类型"); return; }
    JoinCommands c = { FindCmd(type,"创建"), FindCmd(type,"等待"), FindCmd(type,"取消"),
        FindCmd(type,"销毁"), FindCmd(type,"取到达数量"), FindCmd(type,"是否已完成"),
        FindCmd(type,"是否已取消"), FindCmd(type,"线程会合构造函数"),
        FindCmd(type,"线程会合复制函数"), FindCmd(type,"线程会合析构函数") };
    Obj obj; ObjInit(&obj);
    MDATA_INF r; Call(c.construct, NULL, 0, &r); obj.slot = r.m_int;
    g_rtErrSup++;
    if (JoinCall(c.create,&obj,0,true) != BL_FALSE) Fail("会合不能接受0个线程");
    g_rtErrSup--;
    JoinWorkerCtx ctx[5]; HANDLE threads[5];
    JoinCall(c.create,&obj,5,true);
    JoinStart(ctx,threads,&obj,c.wait,0,4);
    JoinCheckArrival(&c,&obj,4);
    for(int i=0;i<4;i++) if(InterlockedCompareExchange(&ctx[i].returned,0,0)) Fail("不足5个线程时提前返回");
    g_rtErrSup++;
    if(JoinCall(c.create,&obj,6,true) != BL_FALSE) Fail("会合不能在上一轮仍等待时重置");
    g_rtErrSup--;
    JoinStart(ctx,threads,&obj,c.wait,4,5);
    if(JoinCollect(ctx,threads,5)!=1 || ctx[4].elected!=1) Fail("必须恰好只有第5个到达者返回真");
    if(JoinCall(c.complete,&obj)!=BL_TRUE || JoinCall(c.arrived,&obj)!=5) Fail("会合完成状态错误");
    if(JoinCall(c.wait,&obj)!=BL_FALSE || JoinCall(c.cancel,&obj)!=BL_FALSE) Fail("到齐后不能再次选出最后一个或取消结果");

    JoinCall(c.create,&obj,5,true);
    JoinStart(ctx,threads,&obj,c.wait,0,3); JoinCheckArrival(&c,&obj,3);
    if(JoinCall(c.cancel,&obj)!=BL_TRUE || JoinCollect(ctx,threads,3)!=0) Fail("取消后必须全部返回假");
    if(JoinCall(c.cancelled,&obj)!=BL_TRUE || JoinCall(c.wait,&obj)!=BL_FALSE) Fail("取消后晚到线程不能继续等待");

    // 复制共享会合状态；释放原变量不能破坏其他副本。
    JoinCall(c.create,&obj,5,true);
    Obj copies[5];
    for(int i=0;i<5;i++)
    {
        ObjInit(&copies[i]);
        MDATA_INF a[2] = { copies[i].self, obj.self };
        Call(c.copy,a,2,&r);
    }
    JoinCall(c.destruct,&obj);
    for(int i=0;i<5;i++) JoinStart(ctx,threads,&copies[i],c.wait,i,i+1);
    if(JoinCollect(ctx,threads,5)!=1) Fail("会合副本未共享同一个计数与事件");
    for(int i=0;i<5;i++) JoinCall(c.destruct,&copies[i]);

    // 最后一个拥有者销毁时，已进入等待的调用仍有引用，必须安全唤醒后再释放事件。
    JoinCall(c.create,&obj,5,true);
    JoinStart(ctx,threads,&obj,c.wait,0,4); JoinCheckArrival(&c,&obj,4);
    JoinCall(c.destroy,&obj);
    if(JoinCollect(ctx,threads,4)!=0) Fail("销毁未完成会合没有取消等待者");
    JoinCall(c.create,&obj,1,true);
    if(JoinCall(c.wait,&obj)!=BL_TRUE || JoinCall(c.wait,&obj)!=BL_FALSE) Fail("单线程会合必须只返回一次真");

    DWORD before = 0, after = 0; GetProcessHandleCount(GetCurrentProcess(),&before);
    for(int round=0;round<100;round++)
    {
        JoinCall(c.create,&obj,5,true);
        JoinStart(ctx,threads,&obj,c.wait,0,4); JoinCheckArrival(&c,&obj,4);
        JoinStart(ctx,threads,&obj,c.wait,4,5);
        if(round%2) JoinCall(c.cancel,&obj);
        const int elected=JoinCollect(ctx,threads,5);
        const bool complete=JoinCall(c.complete,&obj)==BL_TRUE;
        if(elected!=(complete?1:0) || (complete && ctx[4].elected!=1)) Fail("到齐与取消竞争结果不一致，第%d轮",round);
        if(!complete && JoinCall(c.cancelled,&obj)!=BL_TRUE) Fail("会合未完成也未取消，第%d轮",round);
    }
    GetProcessHandleCount(GetCurrentProcess(),&after);
    if(after>before) Fail("重复会合泄漏句柄：%lu -> %lu",before,after);
    JoinCall(c.destruct,&obj);
    printf("       线程会合：不足人数不返回、最后到达者唯一继续、取消、共享副本、销毁安全、100轮竞争通过\n");
}

struct SyncStateCtx
{
    Obj* obj; INT create, destroy, query; int type, mode, rounds; HANDLE start; volatile LONG failures;
};
static int StateCreate(SyncStateCtx* c)
{
    MDATA_INF r, a[3] = { c->obj->self, A(c->type==20 ? 0 : c->type==21 ? 2 : 5), A(2) };
    Call(c->create,a,c->type==22 ? 1 : c->type==21 ? 3 : 2,&r);
    return r.m_bool;
}
static DWORD WINAPI StateWorker(LPVOID arg)
{
    SyncStateCtx* c=(SyncStateCtx*)arg;
    WaitForSingleObject(c->start,INFINITE);
    for(int i=0;i<c->rounds;i++)
    {
        if(c->mode==1 || c->mode==3)
            if(StateCreate(c)!=BL_TRUE)InterlockedIncrement(&c->failures);
        if(c->mode==3) JoinCall(c->destroy,c->obj);
        const int value=JoinCall(c->query,c->obj);
        if((value!=BL_TRUE && value!=BL_FALSE) || (c->mode==0 && value!=BL_FALSE) || (c->mode==1 && value!=BL_TRUE))
            InterlockedIncrement(&c->failures);
    }
    return 0;
}
static void StateWave(SyncStateCtx* seed,int mode)
{
    SyncStateCtx ctx[16]; HANDLE threads[16];
    const int count=mode==2 ? 9 : 16;
    HANDLE start=CreateEvent(NULL,TRUE,FALSE,NULL);
    for(int i=0;i<count;i++)
    {
        ctx[i]=*seed;ctx[i].start=start;ctx[i].failures=0;
        ctx[i].mode=mode==2 && i==8 ? 3 : mode;
        ctx[i].rounds=mode==0 ? 200 : mode==1 ? 500 : i==8 ? 2000 : 6000;
        threads[i]=CreateThread(NULL,0,StateWorker,&ctx[i],0,NULL);
        if(!threads[i]){Fail("创建状态验证无法启动线程");ExitProcess(2);}
    }
    SetEvent(start);
    if(WaitForMultipleObjects(count,threads,TRUE,15000)!=WAIT_OBJECT_0){Fail("创建状态并发检查挂起");ExitProcess(2);}
    for(int i=0;i<count;i++){if(ctx[i].failures)Fail("创建状态并发结果错误：类型%d、模式%d、次数%ld",seed->type,mode,ctx[i].failures);CloseHandle(threads[i]);}
    CloseHandle(start);
}
static void TestSyncState()
{
    g_part=14;
    const char* names[]={"线程锁","信号量","条件变量","线程会合"};
    for(int type=20;type<=23;type++)
    {
        Obj obj;ObjInit(&obj);
        char name[100];wsprintfA(name,"%s构造函数",names[type-20]);
        MDATA_INF r;Call(FindCmd(type,name),NULL,0,&r);obj.slot=r.m_int;
        SyncStateCtx c={};c.obj=&obj;c.type=type;c.query=FindCmd(type,"是否已创建");
        c.create=FindCmd(type,type==23 ? "确保创建" : "创建");c.destroy=FindCmd(type,"销毁");
        // 第一轮包含生命周期锁的并发首次初始化，查询不能顺便创建对象。
        StateWave(&c,0);
        if(JoinCall(c.query,&obj)!=BL_FALSE || (type!=23 && obj.slot!=0))Fail("构造后应未创建，查询不能创建对象");
        Obj copy;ObjInit(&copy);wsprintfA(name,"%s复制函数",names[type-20]);INT cp=FindCmd(type,name);
        MDATA_INF a[2]={copy.self,obj.self};Call(cp,a,2,&r);
        if(JoinCall(c.query,&copy)!=BL_FALSE)Fail("复制未创建变量后不应自动创建");
        JoinCall(c.destroy,&copy);
        StateWave(&c,1);
        if(JoinCall(c.query,&obj)!=BL_TRUE)Fail("按需创建后状态不是已创建");
        a[0]=copy.self;a[1]=obj.self;Call(cp,a,2,&r);
        if(JoinCall(c.query,&copy)!=BL_TRUE)Fail("复制已创建对象丢失状态");
        if((type==23 && copy.slot!=obj.slot)||(type!=23 && copy.slot==obj.slot))Fail("同步对象复制共享规则错误");
        JoinCall(c.destroy,&copy);
        if(type==21)
        {
            INT take=FindCmd(type,"尝试等待");
            if(JoinCall(take,&obj)!=BL_TRUE || JoinCall(take,&obj)!=BL_TRUE || JoinCall(take,&obj)!=BL_FALSE)
                Fail("信号量首次创建的初始计数没有生效");
            StateCreate(&c);
            if(JoinCall(take,&obj)!=BL_FALSE)Fail("重复创建错误地补充了信号量计数");
        }
        if(type==23)
        {
            JoinCall(FindCmd(type,"取消"),&obj);
            StateCreate(&c);
            if(JoinCall(c.query,&obj)!=BL_TRUE || JoinCall(FindCmd(type,"是否已取消"),&obj)!=BL_TRUE)
                Fail("确保创建不能重开已取消的会合");
        }
        JoinCall(c.destroy,&obj);
        if(JoinCall(c.query,&obj)!=BL_FALSE)Fail("销毁后状态未变成未创建");
        DWORD before=0,after=0;GetProcessHandleCount(GetCurrentProcess(),&before);
        StateWave(&c,2);
        JoinCall(c.destroy,&obj);
        GetProcessHandleCount(GetCurrentProcess(),&after);
        if(after>before)Fail("创建/查询/销毁竞争泄漏句柄：类型%d %lu -> %lu",type,before,after);
        if(JoinCall(c.query,&obj)!=BL_FALSE)Fail("并发销毁收尾后状态错误");
        StateCreate(&c);
        if(JoinCall(c.query,&obj)!=BL_TRUE)Fail("销毁后再次创建失败");
        JoinCall(c.destroy,&obj);
        printf("       %s：真实构造未创建、复制状态、16线程按需创建8000次、48000次查询/2000轮销毁竞争通过\n",names[type-20]);
    }
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* dll = argc > 1 ? argv[1] : "out\\cxsoft.fne";
    HMODULE h = LoadLibraryA(dll);
    if (h == NULL) { printf("[FAIL] 加载 %s 失败 %lu\n", dll, GetLastError()); return 1; }
    g_inf = ((fn_GetNewInf)GetProcAddress(h, "GetNewInf"))();
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);
    TestSyncState();
    const int T_LOCK = 20, T_SEM = 21, T_COND = 22;      // 数据类型下标（0 基）
    printf("[host_lock] %s  命令 %d 条 / 类 %d 个\n", dll, g_inf->m_nCmdCount, g_inf->m_nDataTypeCount);
    if (g_inf->m_nDataTypeCount < 23) { Fail("库里没有 线程锁/信号量/条件变量 三类（%d）", g_inf->m_nDataTypeCount); return 1; }
    for (int i = T_LOCK; i <= T_COND; i++)
        printf("       类 %d = %s（成员 %d 条）\n", i + 1, AS_TEXT(g_inf->m_pDataType[i].m_szName), g_inf->m_pDataType[i].m_nCmdCount);

    Obj lock; ObjInit(&lock);
    INT cCreate = FindCmd(T_LOCK, "创建"), cEnter = FindCmd(T_LOCK, "进入"), cLeave = FindCmd(T_LOCK, "离开"),
        cTry = FindCmd(T_LOCK, "尝试进入"), cWait = FindCmd(T_LOCK, "等待"), cSig = FindCmd(T_LOCK, "信号"),
        cRec = FindCmd(T_LOCK, "取锁定次数"), cOwn = FindCmd(T_LOCK, "取持有线程id"), cLocked = FindCmd(T_LOCK, "是否已锁定"),
        cDest = FindCmd(T_LOCK, "销毁"), cNamed = FindCmd(T_LOCK, "按名取锁"), cSetH = FindCmd(T_LOCK, "置句柄"), cGetH = FindCmd(T_LOCK, "取句柄"),
        cWaiters = FindCmd(T_LOCK, "取等待数量");
    MDATA_INF r, a[3];

    g_part = 1;
    // 1创建 + 递归计数
    a[0] = lock.self; a[1] = A(0);
    Call(cCreate, a, 2, &r);
    if (r.m_bool != BL_TRUE || lock.slot == 0) { Fail("线程锁.创建 失败"); return 1; }
    a[1] = A(-1);
    for (int i = 0; i < 3; i++) Call(cEnter, a, 2, &r);
    if (r.m_bool != BL_TRUE) Fail("递归进入失败");
    a[0] = lock.self;
    Call(cRec, a, 1, &r);
    if (r.m_int != 3) Fail("取锁定次数 应为 3，实际 %d", r.m_int);
    Call(cOwn, a, 1, &r);
    if (r.m_int != (INT)GetCurrentThreadId()) Fail("取持有线程id 应为 %lu，实际 %d", GetCurrentThreadId(), r.m_int);
    for (int i = 0; i < 3; i++) Call(cLeave, a, 1, &r);
    Call(cLocked, a, 1, &r);
    if (r.m_bool == BL_TRUE) Fail("全部离开后仍显示被锁定");

    g_part = 2;
    // 2多线程抢锁：计数不丢（每个线程用 置句柄 绑到同一把锁）
    INT hLock = 0;
    a[0] = lock.self;
    Call(cGetH, a, 1, &r); hLock = r.m_int;
    if (hLock == 0) { Fail("取句柄 返回 0，后面没法测"); return 1; }
    {
        Ctx ctx; ctx.cmdEnter = cEnter; ctx.cmdLeave = cLeave; ctx.pSum = &g_sum; ctx.nRounds = nRounds;
        ctx.cmdWait = cWait; ctx.msWait = -1; ctx.cmdSetH = cSetH; ctx.hObj = hLock; ctx.cmdRec = cRec; ctx.cmdOwner = cOwn;
        HANDLE hTh[16];
        g_sum = 0;
        for (int i = 0; i < nThreads; i++) hTh[i] = CreateThread(NULL, 0, Worker, &ctx, 0, NULL);
        WaitForMultipleObjects(nThreads, hTh, TRUE, INFINITE);
        for (int i = 0; i < nThreads; i++) CloseHandle(hTh[i]);
        if (g_sum != (LONG)nThreads * nRounds) Fail("%d 线程 × %d 轮累加丢失：得 %ld", nThreads, nRounds, g_sum);
        else printf("       %d 线程 × %d 轮 计数无丢失\n", nThreads, nRounds);
    }

    g_part = 3;
    // 3超时进入：自线程已持有 → 别的线程拿不到；这里用第二对象绑同一把锁模拟
    {
        Obj other; ObjInit(&other);
        a[0] = other.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        a[0] = other.self; a[1] = A(hLock);
        Call(cSetH, a, 2, &r);                       // 绑到同一把锁
        a[0] = lock.self; a[1] = A(-1);
        Call(cEnter, a, 2, &r);                      // 持有
        a[0] = other.self; a[1] = A(50);
        Call(cEnter, a, 2, &r);                      // 同一线程会重入，故这里只验重入计数
        if (r.m_bool != BL_TRUE) Fail("同线程绑定同一把锁应可重入");
        a[0] = lock.self;
        Call(cRec, a, 1, &r);
        if (r.m_int < 2) Fail("重入后 取锁定次数 太少：%d", r.m_int);
        Call(cLeave, a, 1, &r); Call(cLeave, a, 1, &r);
        a[0] = other.self; a[1] = A(0);
        Call(cSetH, a, 2, &r);                       // 解绑：other 不再持有，锁还归 lock
        a[0] = lock.self;
        Call(cGetH, a, 1, &r);
        if (r.m_int != hLock) Fail("解绑附属对象后主对象句柄变了（引用计数算错）");
    }
    g_part = 4;
    // 4非持有者解锁：预期内报错，不崩
    {
        Obj other; ObjInit(&other);
        a[0] = other.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        g_rtErrSup++;
        a[0] = other.self;
        Call(cLeave, a, 1, &r);                      // 没持有就解锁
        g_rtErrSup--;
        if (r.m_bool == BL_TRUE) Fail("非持有者解锁却返回真");
    }
    g_part = 5;
    // 5等待 + 信号：另一线程唤醒
    {
        Ctx ctx; ctx.cmdEnter = cEnter; ctx.cmdLeave = cLeave; ctx.cmdWait = cWait; ctx.msWait = -1;
        ctx.pSum = &g_sum; ctx.nRounds = 0; ctx.cmdSetH = cSetH; ctx.hObj = hLock;
        ctx.cmdRec = cRec; ctx.cmdOwner = cOwn;
        HANDLE hT = CreateThread(NULL, 0, Waiter, &ctx, 0, NULL);
        Sleep(200);
        Obj lock2; ObjInit(&lock2);
        a[0] = lock2.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        a[0] = lock2.self; a[1] = A(hLock);
        Call(cSetH, a, 2, &r);
        a[0] = lock2.self; a[1] = A(-1);
        Call(cEnter, a, 2, &r);
        if (r.m_bool != BL_TRUE) Fail("第二个对象绑同一把锁后进不去（等待者已放开锁，应能拿到）");
        a[0] = lock2.self;
        INT nW = 0;
        Call(cWaiters, a, 1, &r); nW = r.m_int;
        Call(cSig, a, 1, &r);                        // 唤醒等待者（同句柄同对象）
        Call(cLeave, a, 1, &r);
        DWORD code = 0;
        for (int i = 0; i < 60 && (GetExitCodeThread(hT, &code) && code == STILL_ACTIVE); i++) Sleep(50);
        if (nW != 1) Fail("取等待数量 应为 1，实际 %d", nW);
        if (code != 0) Fail("等待 的线程没被唤醒（退出码 %lu）", code);
        else printf("       等待/信号 正常唤醒\n");
        CloseHandle(hT);
    }
    g_part = 6;
    // 6等待超时：无人信号 → 返回假，不挂死
    {
        Obj l; ObjInit(&l);
        a[0] = l.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        a[0] = l.self; a[1] = A(120);
        Call(cEnter, a, 2, &r);
        DWORD t0 = GetTickCount();
        Call(cWait, a, 2, &r);
        if (r.m_bool != BL_FALSE) Fail("等待 超时却返回真");
        if (GetTickCount() - t0 > 1200) Fail("等待 超时没生效（耗 %lu 毫秒）", GetTickCount() - t0);
        Call(cLeave, a, 1, &r);
    }
    g_part = 7;
    // 7命名锁：同名同一对象
    {
        a[0] = A(0); a[1] = A(0);
        MDATA_INF t = A(0); t.m_dtDataType = SDT_TEXT; t.m_pText = (char*)"cxsoft_lock_probe";
        MDATA_INF na[2] = { A(0), t };
        // 全局式调用（成员命令第 0 参是对象，这里随便给一个已建对象占位）
        na[0] = lock.self;
        Call(cNamed, na, 2, &r);
        INT h1 = r.m_int;
        Call(cNamed, na, 2, &r);
        if (h1 == 0 || h1 != r.m_int) Fail("按名取锁 两次返回不同地址：%d / %d", h1, r.m_int);
    }
    g_part = 8;
    // 8销毁后使用：只报错不崩
    {
        Obj l; ObjInit(&l);
        a[0] = l.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        a[0] = l.self;
        Call(cDest, a, 1, &r);
        g_rtErrSup++;
        a[0] = l.self; a[1] = A(0);
        Call(cEnter, a, 2, &r);
        g_rtErrSup--;
        if (r.m_bool == BL_TRUE) Fail("销毁后仍可进入（对象槽未被清空）");
    }
    g_part = 9;
    // 9信号量：限并发
    {
        Obj s; ObjInit(&s);
        INT cSemCreate = FindCmd(T_SEM, "创建"), cSemWait = FindCmd(T_SEM, "等待"), cSemTry = FindCmd(T_SEM, "尝试等待"),
            cSemRel = FindCmd(T_SEM, "释放"), cSemDest = FindCmd(T_SEM, "销毁");
        a[0] = s.self; a[1] = A(2); a[2] = A(2);
        Call(cSemCreate, a, 3, &r);
        if (r.m_bool != BL_TRUE) Fail("信号量.创建 失败");
        a[0] = s.self;
        Call(cSemTry, a, 1, &r); if (r.m_bool != BL_TRUE) Fail("第 1 次尝试等待应成功");
        Call(cSemTry, a, 1, &r); if (r.m_bool != BL_TRUE) Fail("第 2 次尝试等待应成功");
        Call(cSemTry, a, 1, &r); if (r.m_bool == BL_TRUE) Fail("计数耗尽后尝试等待却成功");
        a[1] = A(80);
        Call(cSemWait, a, 2, &r); if (r.m_bool == BL_TRUE) Fail("计数为 0 时等待却成功");
        Call(cSemRel, a, 1, &r); if (r.m_bool != BL_TRUE) Fail("释放 失败");
        Call(cSemTry, a, 1, &r); if (r.m_bool != BL_TRUE) Fail("释放后尝试等待应成功");
        Call(cSemDest, a, 1, &r);
    }
    g_part = 10;
    // 10条件变量：等待 + 广播
    {
        Obj cd; ObjInit(&cd);
        INT cCondCreate = FindCmd(T_COND, "创建"), cCondWait = FindCmd(T_COND, "等待"), cCondBc = FindCmd(T_COND, "广播"),
            cCondDest = FindCmd(T_COND, "销毁");
        Obj l; ObjInit(&l);
        a[0] = l.self; a[1] = A(hLock);
        Call(cSetH, a, 2, &r);
        if (r.m_dtDataType != _SDT_NULL) Fail("置句柄 声明无返回值，实际填了类型");
        a[0] = cd.self;
        Call(cCondCreate, a, 1, &r);
        if (r.m_bool != BL_TRUE) Fail("条件变量.创建 失败");
        a[0] = cd.self;
        a[1].m_dtDataType = MAKELONG(T_LOCK + 1, 0); a[1].m_pCompoundData = (void*)&l.slot;
        a[2] = A(100);
        MDATA_INF la[2] = { l.self, A(-1) };
        Call(cEnter, la, 2, &r);
        Call(cCondWait, a, 3, &r);
        if (r.m_bool == BL_TRUE) Fail("条件变量 无人唤醒却返回真");
        Call(cCondBc, a, 1, &r);
        Call(cCondDest, a, 1, &r);
    }
    g_part = 11;
    // 11收尾：主对象 销毁 后回到未创建状态，且不影响别的对象
    {
        a[0] = lock.self;
        Call(cDest, a, 1, &r);
        a[0] = lock.self;
        Call(cGetH, a, 1, &r);
        if (r.m_int != 0) Fail("销毁后 取句柄 应为 0，实际 %d", r.m_int);
        g_rtErrSup++;
        a[0] = lock.self; a[1] = A(0);
        Call(cEnter, a, 2, &r);
        g_rtErrSup--;
        if (r.m_bool == BL_TRUE) Fail("销毁后仍可进入");
        a[0] = lock.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        if (r.m_bool != BL_TRUE) Fail("销毁后重新 创建 失败");
        a[0] = lock.self; a[1] = A(-1);
        Call(cEnter, a, 2, &r);
        if (r.m_bool != BL_TRUE) Fail("重建后的锁进不去");
        Call(cLeave, a, 1, &r);
        a[0] = lock.self;
        Call(cDest, a, 1, &r);
    }
    // 12 省略超时毫秒 = 一直等（回归：曾经把可空实参的 0 当成"0 毫秒立即超时"，与说明正好相反）
    {
        Obj holder; ObjInit(&holder);
        a[0] = holder.self; a[1] = A(0);
        Call(cCreate, a, 2, &r);
        INT h = 0;
        a[0] = holder.self;
        Call(cGetH, a, 1, &r); h = r.m_int;
        a[0] = holder.self; a[1] = A(-1);
        Call(cEnter, a, 2, &r);                      // 主线程持有
        HoldCtx hc; hc.cmdEnter = cEnter; hc.cmdLeave = cLeave; hc.cmdSetH = cSetH; hc.hObj = h; hc.nAcquired = 0;
        HoldCtx hp; hp.cmdEnter = cEnter; hp.cmdLeave = cLeave; hp.cmdSetH = cSetH; hp.hObj = h; hp.nAcquired = 0;
        HANDLE hT = CreateThread(NULL, 0, OmitWaiter, &hc, 0, NULL);
        Sleep(300);
        DWORD code = 0; GetExitCodeThread(hT, &code);
        if (code == STILL_ACTIVE) printf("       省略超时毫秒的 进入 会一直等（300 毫秒后仍在等锁）\n");
        else Fail("省略超时毫秒的 进入 提前返回了（退出码 %lu）—— 省略被当成了 0 毫秒", code);
        // 传 0 必须"只试一次"：换个线程来探，同线程重入是允许的、测不出超时
        HANDLE hP = CreateThread(NULL, 0, ProbeZero, &hp, 0, NULL);
        WaitForSingleObject(hP, 3000);
        DWORD pc = 0; GetExitCodeThread(hP, &pc);
        if (pc != 0) Fail("别的线程传 0 却拿到了被持有的锁（退出码 %lu）", pc);
        else if (hp.nAcquired > 120) Fail("传 0 不该等待（耗 %ld 毫秒）", (LONG)hp.nAcquired);
        else printf("       传 0 = 只试一次不等待，立即返回假（耗 %ld 毫秒）\n", (LONG)hp.nAcquired);
        CloseHandle(hP);
        a[0] = holder.self;
        Call(cLeave, a, 1, &r);                      // 放开 → 等待者该拿到
        WaitForSingleObject(hT, 3000);
        GetExitCodeThread(hT, &code);
        if (code != 0 || hc.nAcquired != 1) Fail("主线程放开后，省略参数的等待者没拿到锁（退出码 %lu，拿到次数 %ld）", code, (LONG)hc.nAcquired);
        else printf("       主线程放开后，省略参数的等待者正常拿到并退出\n");
        CloseHandle(hT);
        a[0] = holder.self;
        Call(cDest, a, 1, &r);
    }
    TestJoin();
    printf("\n内存池：申请 %d / 释放 %d / 运行时错误 %d\n", g_mallocCnt, g_freeCnt, g_rtErr);
    printf(g_failed ? "结论: %d 项不合格\n" : "结论: 线程锁族全部通过\n", g_failed);
    FreeLibrary(h);
    return g_failed ? 1 : 0;
}
