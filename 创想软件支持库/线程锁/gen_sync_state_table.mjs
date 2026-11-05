// 创建状态命令只追加到总表末尾；已有方法的绝对序号保持不变。
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const dir=path.dirname(fileURLToPath(import.meta.url));
const base=633;
const methods=[
  ['线程锁','CxLock','是否已创建','IsCreated'],
  ['信号量','CxSem','是否已创建','IsCreated'],
  ['条件变量','CxCond','是否已创建','IsCreated'],
  ['线程会合','CxJoin','是否已创建','IsCreated'],
  ['线程会合','CxJoin','确保创建','EnsureCreated'],
];
const header='// 由 gen_sync_state_table.mjs 生成，勿手改。\n';
const cmds=[],funcs=[],names=[];
const additions=new Map();
for(const [i,[owner,tag,name,suffix]] of methods.entries()) {
  const fn=`${tag}_${suffix}`;
  const ensure=suffix==='EnsureCreated';
  const note=ensure?'原子检查并按需配置会合；已配置直接返回真，不重置当前轮次，取消或完成后也不重新开启。':'线程安全地查询是否已创建；未创建或销毁后返回假，查询本身不创建对象。';
  cmds.push(`// CX_APPEND_OWNER ${owner}`,`    CX_OBJ("${name}", "${fn}", "${note}", _CMD_OS(__OS_WIN), SDT_BOOL, ${ensure?1:0}, ${ensure?'g_syncEnsureJoinArgs':'NULL'}),`);
  funcs.push(`    ${fn},`); names.push(`    _T("${fn}"),`);
  if(!additions.has(tag))additions.set(tag,[]);
  additions.get(tag).push(base+i);
}
cmds.push('// CX_APPEND_OWNER -');
for(const file of ['lock_cmdindex.inc','join_cmdindex.inc']) {
  let text=fs.readFileSync(path.join(dir,file),'utf8');
  for(const [tag,extra] of additions) {
    const variable=`g_${tag}CmdIndex`;
    const re=new RegExp(`INT ${variable}\\[\\] = \\{([^}]+)\\};`);
    const old=text.match(re);if(!old)continue;
    const cells=[...old[1].matchAll(/\d+/g)].map(x=>Number(x[0])).filter(x=>x<base).concat(extra);
    text=text.replace(re,`INT ${variable}[] = { ${cells.join(', ')} };`)
      .replace(new RegExp(`sizeof\\(${variable}\\) / sizeof\\(INT\\) == \\d+`),`sizeof(${variable}) / sizeof(INT) == ${cells.length}`);
  }
  fs.writeFileSync(path.join(dir,file),text);
}
const write=(f,s)=>fs.writeFileSync(path.join(dir,`sync_state_${f}.inc`),header+s+'\n');
write('args','static ARG_INFO g_syncEnsureJoinArgs[] = {\n    { _T("线程总数"), _T("首次配置时使用的线程总数，必须大于0；已配置不重置轮次。"), 0, 0, SDT_INT, 0, 0 },\n};');
write('cmds',cmds.join('\n'));write('funcs',funcs.join('\n'));write('names',names.join('\n'));
write('assert',`static_assert(sizeof(g_commands)/sizeof(CMD_INFO) >= ${base+methods.length}, "同步类状态命令追加位置错误");`);
console.log(`[OK] 同步类创建状态: ${methods.length} commands, ${base}..${base+methods.length-1}`);

await import('./gen_cancel_table.mjs');
