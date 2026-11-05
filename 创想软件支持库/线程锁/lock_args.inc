// 由 线程锁/gen_lock_table.mjs 生成，勿手改。
static ARG_INFO g_lkArgCopyCxLock[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_LOCK, 0), 0, 0 },
};
static ARG_INFO g_lkArgCxLockCreate[] = {
    { _T("锁类型"), _T("锁类型：0=互斥锁（默认，同一线程可重复进入）；2=抢占式自旋锁（只适合极短临界区，不能嵌套加锁）。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_lkArgCxLockEnter[] = {
    { _T("超时毫秒"), _T("等待超时毫秒数：省略或传 -1 表示一直等到成功为止；传 0 表示只试一次不等待；传正数表示最多等这么多毫秒。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_lkArgCxLockWait[] = {
    { _T("超时毫秒"), _T("等待超时毫秒数：省略或传 -1 表示一直等到成功为止；传 0 表示只试一次不等待；传正数表示最多等这么多毫秒。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_lkArgCxLockNamed[] = {
    { _T("锁名"), _T("锁的名字；同一进程内同名即同一把锁，可让多个对象、多个线程共用一把锁。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_lkArgCxLockSetHandle[] = {
    { _T("对象地址"), _T("由 按名取锁 或 取句柄 得到的锁对象地址；传 0 表示解绑（不会销毁对方的锁）。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_lkArgCopyCxSem[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_SEM, 0), 0, 0 },
};
static ARG_INFO g_lkArgCxSemCreate[] = {
    { _T("初始计数"), _T("创建时即可用的计数个数，省略为 0。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("最大计数"), _T("计数上限，省略或传 0 表示不限（2147483647）。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_lkArgCxSemWait[] = {
    { _T("超时毫秒"), _T("等待超时毫秒数：省略或传 -1 表示一直等到成功为止；传 0 表示只试一次不等待；传正数表示最多等这么多毫秒。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_lkArgCopyCxCond[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_COND, 0), 0, 0 },
};
static ARG_INFO g_lkArgCxCondWait[] = {
    { _T("锁"), _T("要配对释放与重新拿回的 线程锁 对象。"), 0, 0, MAKELONG(CX_TYPE_LOCK, 0), 0, 0 },
    { _T("超时毫秒"), _T("等待超时毫秒数：省略或传 -1 表示一直等到成功为止；传 0 表示只试一次不等待；传正数表示最多等这么多毫秒。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
