// Decide, once and for all, whether a static .lib can be linked by the EPL VC6 linker.
// It indexes every symbol defined by the libraries EPL really ships with
// (<EPL>\linker\VC6linker\Lib + <EPL>\MFC\Lib) and diffs them against the undefined
// symbols our archive still asks for. Anything left is a hard LNK2001 at static compile.
// Symbols satisfied by another member of the same archive are filtered out first.
// Usage: node check_static_undef.mjs <lib> [--epl <root>] [--dumpbin <path>] [--cache <file>]
import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync, writeFileSync, readdirSync, statSync, mkdirSync } from 'node:fs';
import path from 'node:path';
import os from 'node:os';

const argv = process.argv.slice(2);
const target = argv.shift();
if (!target) { console.error('[FAIL] usage: node check_static_undef.mjs <lib>'); process.exit(2); }
function opt(name, def) {
  const i = argv.indexOf('--' + name);
  return i >= 0 && argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[i + 1] : def;
}
const EPL = opt('epl', process.env.EPL_E_PATH ? path.dirname(process.env.EPL_E_PATH) : 'D:\\e5.5');
const DUMPBIN = opt('dumpbin', 'dumpbin');
const CACHE = opt('cache', path.join(os.tmpdir(), 'vc6lib_index.txt'));

function dumpbin(args) {
  try {
    return execFileSync(DUMPBIN, args, { maxBuffer: 1 << 30 }).toString('latin1');
  } catch (e) {
    const out = e.stdout ? e.stdout.toString('latin1') : '';
    if (out.includes('fatal error')) { console.error(`[FAIL] dumpbin ${args.join(' ')}: ${out.split('\r\n').slice(0, 3).join(' | ')}`); process.exit(2); }
    return out;
  }
}

const NAME_RE = /^\s+(?:\S+\s+)?(\??\S+?)\s*$/;

function definedSymbolsOf(file) {
  const set = new Set();
  const txt = dumpbin(['/LINKERMEMBER:1', file]);
  // /LINKERMEMBER:1 prints "    2A296 __imp__lclose@4" per public symbol.
  for (const line of txt.split(/\r?\n/)) {
    const m = /^\s+[0-9A-Fa-f]{4,16}\s+(\S+)\s*$/.exec(line);
    if (m) set.add(m[1]);
  }
  return set;
}

function buildIndex(dirs) {
  const idx = new Set();
  const files = [];
  for (const d of dirs) {
    if (!existsSync(d)) continue;
    for (const f of readdirSync(d)) if (/\.(lib|obj)$/i.test(f) && statSync(path.join(d, f)).isFile()) files.push(path.join(d, f));
  }
  for (const f of files) for (const s of definedSymbolsOf(f)) idx.add(s);
  return idx;
}

let index;
const libDirs = [path.join(EPL, 'linker', 'VC6linker', 'Lib'), path.join(EPL, 'MFC', 'Lib')].filter(existsSync);
if (!libDirs.length) { console.error(`[FAIL] no VC6 lib dir under ${EPL}`); process.exit(2); }
if (existsSync(CACHE)) {
  try { index = new Set(readFileSync(CACHE, 'utf8').split(/\r?\n/).filter(Boolean)); } catch { index = null; }
}
if (!index) {
  index = buildIndex(libDirs);
  mkdirSync(path.dirname(CACHE), { recursive: true });
  writeFileSync(CACHE, [...index].sort().join('\n'));
}
console.log(`[info] EPL=${EPL} vc6libs=${libDirs.length} indexed symbols=${index.size}`);

// what our own archive already defines (so intra-library refs are not false positives)
const own = new Set();
for (const s of definedSymbolsOf(target)) own.add(s);
const undef = new Set();
for (const line of dumpbin(['/SYMBOLS', target]).split(/\r?\n/)) {
  if (!/UNDEF/.test(line)) continue;
  const m = /\|\s*(\S+)\s*$/.exec(line);
  if (m) undef.add(m[1]);
}

const missing = [...undef].filter((s) => !index.has(s) && !own.has(s)).sort();
console.log(`[info] undefined=${undef.size} resolved-by-self=${[...undef].filter((s) => own.has(s)).length}`);
if (missing.length) {
  console.error(`[ERROR] ${missing.length} symbol(s) the EPL VC6 linker cannot resolve:`);
  for (const s of missing) console.error('  ' + s);
  console.error('[ERROR] either rewrite the source away from them, or add a definition to the static archive.');
  process.exit(1);
}
console.log('[OK] every undefined symbol is resolvable by the EPL VC6 linker');
