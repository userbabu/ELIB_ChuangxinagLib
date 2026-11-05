// 一键跑离线宿主：造种子数据 -> 命令宿主 -> 界面宿主 -> 类宿主。
// 命令宿主不是幂等的（注册/试用/换机都会改库），所以每轮必须先重新 seed，
// 否则会撞上「该邮箱已注册过本软件」这类上一轮留下的状态。
// 用法：node run_hosts.mjs [--php <php.exe>] [--server <url>] [--secret <app_secret>]
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const arg = (name, def) => {
  const i = process.argv.indexOf('--' + name);
  return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : def;
};
const PHP = arg('php', 'D:\\phpstudy_pro\\Extensions\\php\\php8.0.2nts\\php.exe');
const SERVER = arg('server', 'http://127.0.0.1:8317/api.php');
const SECRET = arg('secret', 'CXAH-DEV-SECRET-CHANGE-ME');
const FNE = path.join(here, 'out', 'cxah.fne');

function php(script, ...params) {
  return execFileSync(PHP, [path.join(here, 'php', 'tests', script), ...params],
    { cwd: here, encoding: 'latin1' });
}
// 宿主自己打印的是 GBK，绝不能按 latin1 再转一遍，否则日志全成乱码。
function runHost(exe, args) {
  process.stdout.write('\r\n>>> ' + exe + '\r\n');
  try {
    const out = execFileSync(path.join(here, 'out', exe), args, { cwd: here, maxBuffer: 1 << 28 });
    process.stdout.write(out);
    return 0;
  } catch (e) {
    if (e.stdout) process.stdout.write(e.stdout);
    return typeof e.status === 'number' ? e.status : -1;
  }
}

// 两个宿主都会真的注册/试用/换机，注册过的邮箱不能再注册，
// 所以各要一份独立种子（run 标记差一个字母即可），不能共用同一批账号。
const stamp = String(Date.now()).slice(-9);
function seedFor(tag) {
  const out = php('host_seed.php', tag).trim().split(/\r?\n/).pop();
  const s = JSON.parse(out);
  console.log('[seed] ' + tag + ' -> ' + s.email);
  return s;
}
const cmdSeed = seedFor('c' + stamp);
const uiSeed = seedFor('u' + stamp);
const clsSeed = seedFor('h' + stamp);   // 类宿主也要一份干净的账号，注册/试用不幂等

const hosts = [
  ['host_test.exe', [FNE, SERVER, SECRET, cmdSeed.email, cmdSeed.keyOwn, cmdSeed.keyNew, cmdSeed.emailLock, cmdSeed.keyFeat]],
  ['host_ui.exe', [FNE, SERVER, SECRET, uiSeed.email, uiSeed.keyOwn, uiSeed.keyNew, uiSeed.keyFeat]],
  ['host_class.exe', [FNE, SERVER, SECRET, clsSeed.email, clsSeed.keyOwn, clsSeed.keyNew, clsSeed.keyFeat]],
];
let bad = 0;
for (const [exe, args] of hosts) {
  const code = runHost(exe, args);
  if (code === 0) console.log('[OK] ' + exe + ' 全绿');
  else { console.error('[ERROR] ' + exe + ' 退出码 ' + code); bad++; }
}

php('host_unseed.php');   // 按 cxah-host / HOSTKEY- 前缀清数据，两个标记一起收掉
if (bad) { console.error('[FAIL] 离线宿主未全绿'); process.exit(1); }
console.log('[DONE] 命令宿主、界面宿主、类宿主全部通过');
