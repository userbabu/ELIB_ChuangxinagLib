// 从本文件的类清单生成「线程锁」族的表与桩（与 http访问/gen_http_table.mjs 同一套路，独立目录互不影响）。
// 用法：node 线程锁/gen_lock_table.mjs
// 产物（落在 线程锁\，勿手改）：lock_ops.h / lock_args.inc / lock_cmds.inc / lock_impl.inc /
//                               lock_funcs.inc / lock_names.inc / lock_typereg.inc / lock_cmdindex.inc
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const HDR = '// 由 线程锁/gen_lock_table.mjs 生成，勿手改。';
const BASE = Number(process.argv[2] || 577);   // 本族第一行在 g_commands 里的下标

const DT = {
  int: 'SDT_INT', long: 'SDT_INT64', text: 'SDT_TEXT', bin: 'SDT_BIN', double: 'SDT_DOUBLE',
  bool: 'SDT_BOOL', subptr: 'SDT_SUB_PTR', null: '_SDT_NULL', lockref: 'MAKELONG(CX_TYPE_LOCK, 0)',
};
const esc = (s) => (s || '').replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/[\r\n]+/g, ' ');
function paramTy(ty) {
  const opt = ty.endsWith('?');
  const b = opt ? ty.slice(0, -1) : ty;
  const dt = DT[b] || 'SDT_INT';
  return [dt, opt ? 'AS_DEFAULT_VALUE_IS_EMPTY' : '0'];
}

// 参数说明：优先取专门词典，其次按类型给一句
const PN = {
  '锁类型': '锁类型：0=互斥锁（默认，同一线程可重复进入）；2=抢占式自旋锁（只适合极短临界区，不能嵌套加锁）。',
  '超时毫秒': '等待超时毫秒数：省略或传 -1 表示一直等到成功为止；传 0 表示只试一次不等待；传正数表示最多等这么多毫秒。',
  '锁名': '锁的名字；同一进程内同名即同一把锁，可让多个对象、多个线程共用一把锁。',
  '对象地址': '由 按名取锁 或 取句柄 得到的锁对象地址；传 0 表示解绑（不会销毁对方的锁）。',
  '初始计数': '创建时即可用的计数个数，省略为 0。',
  '最大计数': '计数上限，省略或传 0 表示不限（2147483647）。',
  '锁': '要配对释放与重新拿回的 线程锁 对象。',
};
function 说明参数(pn, pt, 释) {
  return PN[pn] || (({ int: '整数型', bool: '逻辑型', text: '文本型', lockref: '线程锁对象' })[pt.replace('?', '')] || '整数型')
    + '参数：' + pn + '。';
}

// 对象种类：0=互斥锁 1=命名锁 2=信号量 3=条件变量
const 类 = [
  {
    名: '线程锁', 英文: 'CxLock', 说明: '稳定的线程锁：递归进入、超时等待、误用只报错不崩溃；条件等待与命名锁都挂在它上面。',
    类型宏: 'CX_TYPE_LOCK', 序号: 21, 对象: 0,
    命令: [
      ['创建', [['锁类型', 'int?']], 'bool', 'Create', '线程安全地按需创建锁。0=递归互斥锁，2=不可递归自旋型锁；已创建则返回真，保留首次创建的配置。'],
      ['销毁', [], 'null', 'Destroy', '销毁锁。若仍被持有会强制释放并在调试输出里提示，不会崩进程；销毁后本对象回到未创建状态，可再次 创建()。'],
      ['进入', [['超时毫秒', 'int?']], 'bool', 'Enter', '加锁。超时毫秒省略或传 -1 = 一直等到拿到锁；传 0 = 只试一次不等待；传正数 = 最多等这么多毫秒，超时拿不到就返回假。同一线程可重复进入（取锁定次数 累加），只有持有线程能 离开。'],
      ['离开', [], 'bool', 'Leave', '解锁一次；嵌套加锁时要配对调用同样次数。只有持有线程能解锁，误用返回假并报运行时错误。'],
      ['尝试进入', [], 'bool', 'TryEnter', '非阻塞加锁，抢到返回真；锁已被别人持有则立即返回假。'],
      ['等待', [['超时毫秒', 'int?']], 'bool', 'Wait', '在本锁上条件等待：原子地放开锁并挂起，被 信号/唤醒全部 唤醒或超时后自动把锁重新拿回。超时毫秒省略或传 -1 = 一直等；传 0 = 不等直接返回；传正数 = 最多等这么多毫秒。返回假表示超时。必须由持有锁的线程调用。'],
      ['信号', [], 'null', 'Signal', '唤醒本锁上等待的一个线程。只能由持有锁的线程调用。'],
      ['唤醒全部', [], 'null', 'Broadcast', '唤醒本锁上所有等待线程。只能由持有锁的线程调用。'],
      ['是否已锁定', [], 'bool', 'IsLocked', '当前是否被任何线程持有。'],
      ['取持有线程id', [], 'int', 'Owner', '返回当前持有锁的线程 id，0 表示无人持有。'],
      ['取锁定次数', [], 'int', 'Recursion', '当前持有线程的嵌套加锁层数，0 表示未锁定。'],
      ['取等待数量', [], 'int', 'Waiters', '在本锁上条件等待的线程数。'],
      ['按名取锁', [['锁名', 'text']], 'int', 'Named', '按名字取进程内共享的同一把锁，返回锁对象地址（整数型）。配合 置句柄 可让多个对象/多个线程共用一把锁，避免到处传来传去拿到无效锁。'],
      ['置句柄', [['对象地址', 'int']], 'null', 'SetHandle', '把 按名取锁 或 取句柄 得到的对象地址交给本对象使用；传 0 表示解绑（不会销毁对方）。地址不属于本库对象时只报错不接管。'],
      ['取句柄', [], 'int', 'GetHandle', '返回内部锁对象地址，0 表示尚未创建。可交给自己线程外的对象 置句柄 使用。'],
    ],
  },
  {
    名: '信号量', 英文: 'CxSem', 说明: '计数信号量：限制并发数量、跨线程投递计数。',
    类型宏: 'CX_TYPE_SEM', 序号: 22, 对象: 2,
    命令: [
      ['创建', [['初始计数', 'int?'], ['最大计数', 'int?']], 'bool', 'Create', '创建信号量。初始计数默认为 0，最大计数默认为 2147483647。已创建时再次调用不会重建。'],
      ['销毁', [], 'null', 'Destroy', '销毁信号量；销毁后本对象回到未创建状态。'],
      ['等待', [['超时毫秒', 'int?']], 'bool', 'Wait', '取走一个计数；计数为 0 时挂起等待。超时毫秒省略或传 -1 = 一直等到有计数；传 0 = 不等待；传正数 = 最多等这么多毫秒。返回假表示超时。'],
      ['释放', [], 'bool', 'Release', '归还一个计数（可跨线程调用）。返回假表示已达最大计数、计数未增加。'],
      ['尝试等待', [], 'bool', 'TryWait', '非阻塞取计数；没有可用计数立即返回假。'],
      ['取当前计数', [], 'int', 'Value', '返回本对象记录的可用计数（观察用，多线程下取到之后就可能已变，不要拿它做加锁判断）。'],
    ],
  },
  {
    名: '条件变量', 英文: 'CxCond', 说明: '独立条件变量：配任意 线程锁 使用，支持超时等待与广播。',
    类型宏: 'CX_TYPE_COND', 序号: 23, 对象: 3,
    命令: [
      ['创建', [], 'bool', 'Create', '创建条件变量。已创建时再次调用不会重建。'],
      ['销毁', [], 'null', 'Destroy', '销毁条件变量；仍在被等待时会先唤醒全部等待者再释放。'],
      ['等待', [['锁', 'lockref'], ['超时毫秒', 'int?']], 'bool', 'Wait', '挂起等待，并在等待期间放开传入的 线程锁、被唤醒后自动重新拿回。超时毫秒省略或传 -1 = 一直等；传 0 = 不等直接返回；传正数 = 最多等这么多毫秒。返回假表示超时。'],
      ['信号', [], 'null', 'Signal', '唤醒一个等待者（不需要持有锁）。'],
      ['广播', [], 'null', 'Broadcast', '唤醒全部等待者（不需要持有锁）。'],
      ['取等待数量', [], 'int', 'Waiters', '当前正在等待的线程数。'],
    ],
  },
];

const ops = [], args = [], cmds = [], impls = [], funcs = [], names = [], typereg = [], cmdidx = [];
let row = 0;
for (const cls of 类) {
  const tag = cls['英文'], base = row;
  args.push(`static ARG_INFO g_lkArgCopy${tag}[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(${cls['类型宏']}, 0), 0, 0 },
};`);
  cmds.push(`    CX_LIFECYCLE("${cls['名']}", "${cls['名']}", "${tag}", g_lkArgCopy${tag}, "${esc(cls['说明'])}"),`);
  impls.push(`void ${tag}Construct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = 0; // 构造只初始化对象槽，显式创建才分配同步对象。
}

void ${tag}Copy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkCopy(pArgInf, ${cls['对象']});
}

void ${tag}Destruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkFree(&pArgInf[0]);
}`);
  funcs.push(`    ${tag}Construct, ${tag}Copy, ${tag}Destruct,`);
  names.push(`    _T("${tag}Construct"), _T("${tag}Copy"), _T("${tag}Destruct"),`);
  row += 3;
  for (const c of cls['命令']) {
    const [名, 参, 返, impl, 释] = c;
    const plist = (参 || []).map(([pn, pt]) => [pn, pt]);
    const op = `LK_${tag}_${impl}`;
    if (!ops.includes(op)) ops.push(op);
    const an = `g_lkArg${tag}${impl}`;
    if (plist.length) {
      args.push(`static ARG_INFO ${an}[] = {`);
      for (const [pn, pt] of plist) {
        const [dt, fl] = paramTy(pt);
        args.push(`    { _T("${esc(pn)}"), _T("${esc(说明参数(pn, pt, 释))}"), 0, 0, ${dt}, 0, ${fl} },`);
      }
      args.push('};');
    }
    const ret = 返 === 'null' ? '_SDT_NULL' : (DT[返] || 'SDT_INT');
    cmds.push(`    CX_OBJ("${esc(名)}", "${tag}${String(row + 1).padStart(3, '0')}", "${esc(释)}", _CMD_OS(__OS_WIN), ${ret}, ${plist.length}, ${plist.length ? an : 'NULL'}),`);
    impls.push(`void ${tag}_${impl}(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)\n{\n    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_${op}, ${cls['对象']});\n}`);
    funcs.push(`    ${tag}_${impl},`);
    names.push(`    _T("${tag}_${impl}"),`);
    row += 1;
  }
  const nAll = row - base;
  cmdidx.push(`// ${cls['名']}：生命周期 3 + 成员 ${nAll - 3}
INT g_${tag}CmdIndex[] = { ${Array.from({ length: nAll }, (_, i) => BASE + base + i).join(', ')} };
static_assert(sizeof(g_${tag}CmdIndex) / sizeof(INT) == ${nAll}, "${cls['名']} 成员命令条数变了");
static_assert(${cls['类型宏']} == ${cls['序号']}, "${cls['名']} 数据类型序号与登记顺序不一致");`);
  typereg.push(`    CX_TYPE("${cls['名']}", "${tag}", "${esc(cls['说明'])}", g_${tag}CmdIndex),`);
  console.log('     %s: 成员 %d + 生命周期 3，绝对行 %d..%d，数据类型序号 %d', cls['名'], nAll - 3, BASE + base, BASE + row - 1, cls['序号']);
}
// 参数说明缺省时给一条按类型生成的说明
const W = (f, s) => fs.writeFileSync(path.join(HERE, f), s, 'utf8');
W('lock_ops.h', `${HDR}\nenum\n{\n${ops.map((o) => `    LOP_${o},`).join('\n')}\n    LOP_COUNT\n};\n`);
W('lock_args.inc', `${HDR}\n${args.join('\r\n')}\r\n`);
W('lock_cmds.inc', `${HDR}\n${cmds.join('\r\n')}\r\n`);
W('lock_impl.inc', `${HDR}\n${impls.join('\r\n\r\n')}\n`);
W('lock_funcs.inc', `${HDR}\n${funcs.join('\r\n')}\r\n`);
W('lock_names.inc', `${HDR}\n${names.join('\r\n')}\r\n`);
W('lock_typereg.inc', `${HDR}\n${typereg.join('\r\n')}\r\n`);
W('lock_cmdindex.inc', `${HDR}
${cmdidx.join('\r\n')}
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= ${BASE + row}, "线程锁 族的行基址 ${BASE} 与 g_commands 实际长度不符");
`);
console.log('[OK] 共 %d 行命令（成员 %d + 生命周期 %d）/ 操作码 %d 个 / 表尾应为 %d', row, row - 9, 9, ops.length, BASE + row);
await import('./gen_sync_state_table.mjs');
