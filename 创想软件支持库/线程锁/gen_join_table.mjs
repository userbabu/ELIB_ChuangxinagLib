// 独立追加到命令表末尾，既有线程锁与 HTTP 命令序号保持不变。
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const dir = path.dirname(fileURLToPath(import.meta.url));
const header = '// 由 gen_join_table.mjs 生成，勿手改。\n';
const base = 623;
const methods = [
  ['创建', 'Create', 'SDT_BOOL', 1, '设置本轮线程总数（必须大于0）；只能在上一轮等待全部返回后重新创建。'],
  ['等待', 'Wait', 'SDT_BOOL', 0, '到齐前一直等待；仅最后到达者返回真，其余返回假。取消后均返回假，本轮不会再次选出最后到达者。'],
  ['取消', 'Cancel', 'SDT_BOOL', 0, '取消未到齐的一轮并唤醒所有等待者；取消成功返回真，已到齐或已取消返回假。'],
  ['销毁', 'Destroy', '_SDT_NULL', 0, '解除本变量的引用；最后一个变量销毁时取消未完成会合，等待调用返回后释放内部事件。'],
  ['取到达数量', 'Arrived', 'SDT_INT', 0, '本轮已经到达会合点的线程数量，供观察使用。'],
  ['是否已完成', 'IsComplete', 'SDT_BOOL', 0, '线程是否已经到齐；不代表工作线程已经结束，也不代表最后一个线程的后续操作已经完成。'],
  ['是否已取消', 'IsCancelled', 'SDT_BOOL', 0, '本轮是否已被取消。'],
];
const count = 3 + methods.length;
const args = `static ARG_INFO g_joinCopyArgs[] = {
    { _T("对象"), _T("复制后共享同一个会合点；每个副本独立持有引用。"), 0, 0, MAKELONG(CX_TYPE_JOIN, 0), 0, 0 },
};
static ARG_INFO g_joinCreateArgs[] = {
    { _T("线程总数"), _T("本轮需要到达的线程总数，必须大于0；每个线程每轮调用一次等待。"), 0, 0, SDT_INT, 0, 0 },
};`;
const cmds = ['    CX_LIFECYCLE("线程会合", "线程会合", "CxJoin", g_joinCopyArgs, "人数到齐后仅最后到达者返回真，其余返回假；复制对象共享同一个会合点。"),',
  ...methods.map(([cn,en,ret,n,note]) => `    CX_OBJ("${cn}", "CxJoin_${en}", "${note}", _CMD_OS(__OS_WIN), ${ret}, ${n}, ${n ? 'g_joinCreateArgs' : 'NULL'}),`)];
const funcs = ['CxJoinConstruct','CxJoinCopy','CxJoinDestruct',...methods.map(m => `CxJoin_${m[1]}`)];
const write = (name, text) => fs.writeFileSync(path.join(dir, `join_${name}.inc`), header + text + '\n');
write('args', args);
write('cmds', cmds.join('\n'));
write('funcs', funcs.map(f => `    ${f},`).join('\n'));
write('names', funcs.map(f => `    _T("${f}"),`).join('\n'));
write('cmdindex', `INT g_CxJoinCmdIndex[] = { ${funcs.map((_,i) => base+i).join(', ')} };
static_assert(sizeof(g_CxJoinCmdIndex) / sizeof(INT) == ${count}, "线程会合命令数不一致");
static_assert(CX_TYPE_JOIN == 24, "线程会合数据类型序号不一致");
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= ${base+count}, "线程会合追加命令位置不一致");`);
write('typereg', '    CX_TYPE("线程会合", "CxJoin", "人数到齐前等待，仅最后到达者返回真；取消后所有等待者返回假。复制对象共享会合状态。", g_CxJoinCmdIndex),');
console.log(`[OK] 线程会合: ${count} commands, ${base}..${base+count-1}, data type 24`);
await import('./gen_sync_state_table.mjs');
