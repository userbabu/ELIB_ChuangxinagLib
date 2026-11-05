import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const dir=path.dirname(fileURLToPath(import.meta.url));
const base=638, header='// 由 gen_cancel_table.mjs 生成，勿手改。\n';
const methods=[
 ['创建','Create',0,'每次调用都重新开始，生成未取消、未暂停的新状态。先等旧任务结束，再为访问类重新绑定令牌。'],
 ['取消','Cancel',0,'取消本轮共享任务和绑定请求，并唤醒暂停等待；重复取消返回真。需要重新开始时调用创建。'],
 ['暂停','Pause',0,'暂停共享本轮的协作检查点；已取消返回假。'],
 ['恢复','Resume',0,'恢复暂停的本轮任务；本轮已取消时返回假，需要调用创建重新开始。'],
 ['是否已取消','IsCancelled',0,'线程安全查询本轮是否已取消。'],
 ['是否已暂停','IsPaused',0,'线程安全查询本轮是否暂停；取消后返回假。'],
 ['等待取消','WaitCancel',1,'等待取消，超时返回假；省略或-1无限等待，0只查询。可用作可取消延时。'],
 ['等待继续','WaitContinue',1,'运行时返回真；暂停时等待恢复，取消或超时返回假。省略或-1无限等待。'],
 ['是否已创建','IsCreated',0,'查询是否持有有效状态；取消或暂停后仍返回真，查询本身不会创建或重置状态。'],
 ['销毁','Destroy',0,'释放本变量引用，不强杀线程、不自动取消；绑定请求和其他副本保留其状态。'],
];
const args=[`static ARG_INFO g_cancelCopyArgs[] = { { _T("对象"), _T("副本共享同一轮取消与暂停状态。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, 0 } };`,
`static ARG_INFO g_cancelWaitArgs[] = { { _T("超时毫秒"), _T("省略或-1无限等待，0立即检查。"), 0, 0, SDT_INT, -1, AS_HAS_DEFAULT_VALUE } };`,
`static ARG_INFO g_bindCancelArgs[] = { { _T("令牌"), _T("引用绑定取消令牌当前轮次，内部保留引用，无需整数或指针。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, AS_RECEIVE_VAR } };`];
const funcs=['CxCancelConstruct','CxCancelCopy','CxCancelDestruct',...methods.map(m=>`CxCancel_${m[1]}`)];
const cmds=['    CX_LIFECYCLE("取消令牌", "取消令牌", "CxCancel", g_cancelCopyArgs, "构造后即可使用；创建用于重新开始。支持共享取消、暂停、恢复，复制共享当前状态，可绑定多个HTTP访问类。"),',
 ...methods.map(([cn,en,n,note])=>`    CX_OBJ("${cn}", "CxCancel_${en}", "${note}", _CMD_OS(__OS_WIN), ${en==='Destroy'?'_SDT_NULL':'SDT_BOOL'}, ${n}, ${n?'g_cancelWaitArgs':'NULL'}),`)];
const tokenCount=funcs.length;
for(const [tag,owner] of [['AccessCurl','HTTP访问_Curl访问类'],['AccessWinHttp','HTTP访问_WinHTTP访问类'],['AccessWinInet','HTTP访问_WinInet访问类']]) {
 const extra=[];cmds.push(`// CX_APPEND_OWNER ${owner}`);
 for(const [en,cn,n,note] of [['BindToken','绑定取消令牌',1,'绑定令牌当前状态；令牌调用创建后需要重新绑定。内部保留引用并替换整数绑定，请求期间改绑会等待返回，回调内禁止改绑。'],['UnbindToken','解绑取消令牌',0,'解除令牌绑定，不取消令牌，也不影响其他请求；请求期间调用会等待返回，不能用来停止请求。']]) {
  extra.push(base+funcs.length);funcs.push(`${tag}_${en}`);
  cmds.push(`    CX_OBJ("${cn}", "${tag}_${en}", "${note}", _CMD_OS(__OS_WIN), SDT_BOOL, ${n}, ${n?'g_bindCancelArgs':'NULL'}),`);
 }
 const f=path.join(dir,'../http访问/cxhttp_cmdindex.inc');let s=fs.readFileSync(f,'utf8');const v=`g_${tag}CmdIndex`;
 const re=new RegExp(`INT ${v}\\[\\] = \\{([^}]+)\\};`);const old=s.match(re);if(!old)throw Error(v);
 const cells=[...old[1].matchAll(/\d+/g)].map(x=>+x[0]).filter(x=>x<base).concat(extra);
 s=s.replace(re,`INT ${v}[] = { ${cells.join(', ')} };`).replace(new RegExp(`sizeof\\(${v}\\) / sizeof\\(INT\\) == \\d+`),`sizeof(${v}) / sizeof(INT) == ${cells.length}`);
 fs.writeFileSync(f,s);
}
cmds.push('// CX_APPEND_OWNER -');
const write=(name,s)=>fs.writeFileSync(path.join(dir,`cancel_${name}.inc`),header+s+'\n');
write('args',args.join('\n'));write('cmds',cmds.join('\n'));write('funcs',funcs.map(f=>`    ${f},`).join('\n'));write('names',funcs.map(f=>`    _T("${f}"),`).join('\n'));
write('cmdindex',`INT g_CxCancelCmdIndex[] = { ${Array.from({length:tokenCount},(_,i)=>base+i).join(', ')} };
static_assert(CX_TYPE_CANCEL == 25, "取消令牌类型号错误");
static_assert(sizeof(g_commands)/sizeof(CMD_INFO) == ${base+funcs.length}, "取消令牌命令表长度错误");`);
write('typereg','    CX_TYPE("取消令牌", "CxCancel", "共享取消、暂停、恢复、等待状态；可绑定多个HTTP请求，复制与绑定均持有引用。", g_CxCancelCmdIndex),');
console.log(`[OK] cancellation token: ${funcs.length} commands, total ${base+funcs.length}, type 25`);
