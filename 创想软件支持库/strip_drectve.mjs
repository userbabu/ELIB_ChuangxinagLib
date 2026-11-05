// EPL static-compiles with the VC6 (VC98) linker, which rejects /FAILIFMISMATCH and
// /alternatename with LNK4044. EPL reads link.exe output and reports 静态连接失败 even
// though the link exits 0, so those two directives must never reach the archive.
// Usage: node strip_drectve.mjs <obj-or-lib>...   (blanking keeps section size, so no
// COFF header fixups are needed; other directives such as /DEFAULTLIB are left intact)
import { readFileSync, writeFileSync } from 'node:fs';

const BAD = ['/failifmismatch:', '/alternatename:'];
const WS = new Set([0x20, 0x09, 0x0d, 0x0a, 0x00]);

function drectveRanges(buf) {
  const sections = buf.readUInt16LE(2);
  const optSize = buf.readUInt16LE(16);
  const at = 20 + optSize;
  const out = [];
  for (let i = 0; i < sections; i++) {
    const h = at + i * 40;
    const name = buf.toString('ascii', h, h + 8).replace(/\0+$/, '');
    if (name !== '.drectve') continue;
    const size = buf.readUInt32LE(h + 16);
    const ptr = buf.readUInt32LE(h + 20);
    if (size && ptr) out.push([ptr, ptr + size]);
  }
  return out;
}

function strip(buf) {
  const removed = [];
  for (const [start, end] of drectveRanges(buf)) {
    const text = buf.toString('latin1', start, end);
    const lower = text.toLowerCase();
    for (const opt of BAD) {
      let from = 0;
      for (;;) {
        const i = lower.indexOf(opt, from);
        if (i < 0) break;
        let j = i + opt.length;
        while (j < text.length && WS.has(text.charCodeAt(j))) j++;
        if (text[j] === '"') {
          const close = text.indexOf('"', j + 1);
          j = close < 0 ? text.length : close + 1;
        } else {
          while (j < text.length && !WS.has(text.charCodeAt(j))) j++;
        }
        removed.push(text.slice(i, j).trim());
        buf.fill(0x20, start + i, start + j);
        from = j;
      }
    }
  }
  return removed;
}

let failed = false;
for (const f of process.argv.slice(2)) {
  const buf = readFileSync(f);
  const removed = strip(buf);
  const left = strip(buf); // second pass doubles as the "nothing survived" check
  if (left.length) {
    console.error(`[ERROR] ${f}: still carries ${left.length} VC6-incompatible directive(s)`);
    failed = true;
    continue;
  }
  if (removed.length) {
    writeFileSync(f, buf);
    console.log(`[OK] ${f}: blanked ${removed.length} directive(s): ${[...new Set(removed)].join(' | ')}`);
  } else {
    console.log(`[OK] ${f}: no VC6-incompatible directives`);
  }
}
process.exit(failed ? 1 : 0);
