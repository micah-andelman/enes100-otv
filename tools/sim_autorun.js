// Runs a sim sketch in the ENES100 browser simulator with no clicks:
// opens a dedicated Chrome window, loads the code, sets the execution speed
// to "instant", clicks the setup buttons, Run, then Continue, and prints
// the robot pose + simulator console here until the run finishes.
//
//   node --experimental-websocket tools/sim_autorun.js <sketch.ino> [Setup,Buttons] [watchSeconds] [--check] [--quiet]
//   e.g. node --experimental-websocket tools/sim_autorun.js sim/ms5/task7a_wallhug.ino Randomize 150
//
// --check: footprint monitor. Every physics frame it checks the Tank as the
// sim DRAWS it (treads included: 0.18 m long x 0.20 m wide, bigger than the
// 0.15 x 0.12 m collision box) against the arena walls, the solid obstacles
// (read from the canvas at the start) and the limbo opening. Prints a CHECK
// line at the end and exits with code 2 on any violation.
// --quiet: only print the END line (and the CHECK line).
//
// Called by otv.ps1 (otv4..otv7b, otvsim). Uses the Chrome DevTools protocol
// on a separate Chrome profile (%LOCALAPPDATA%\OTV\sim-chrome), so it never
// touches your normal Chrome. Node 20 needs --experimental-websocket.

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const SIM_URL = 'https://umdenes100.github.io/enes100-simulator/';
const PORT = 9333;
const PROFILE = path.join(process.env.LOCALAPPDATA || '.', 'OTV', 'sim-chrome');
const CHROME_PATHS = [
  path.join(process.env.ProgramFiles || 'C:\\Program Files', 'Google\\Chrome\\Application\\chrome.exe'),
  path.join(process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)', 'Google\\Chrome\\Application\\chrome.exe'),
  path.join(process.env.LOCALAPPDATA || '', 'Google\\Chrome\\Application\\chrome.exe'),
  path.join(process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)', 'Microsoft\\Edge\\Application\\msedge.exe'),
];
// Lines that mean the demo is over.
const DONE = /Run complete|Stopped before the limbo|aborting|MS5 (2|3|4|10): done|MS5 8: (obstacle -> stopped|timeout)|MS5 5: sent (un)?polluted|MS5 7b: past the obstacles|MS5 7b: (vision lost|timeout|every lane blocked|stopped)|STOPPED:|Program finished/;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const die = (msg) => { console.error('SIM AUTORUN FAILED: ' + msg); process.exit(1); };

const flags = process.argv.filter((a) => a.startsWith('--'));
const CHECK = flags.includes('--check');
const QUIET = flags.includes('--quiet');
const [sketchArg, setupArg = 'Randomize', watchArg = '150'] = process.argv.slice(2).filter((a) => !a.startsWith('--'));
if (!sketchArg) die('usage: sim_autorun.js <sketch.ino> [Setup,Buttons] [watchSeconds]');
const code = fs.readFileSync(sketchArg, 'utf8');
const setupButtons = setupArg.split(',').map((s) => s.trim()).filter(Boolean);
const watchMs = Number(watchArg) * 1000;

async function getJson(url) {
  const r = await fetch(url);
  return r.json();
}

async function devtoolsUp() {
  try { await getJson(`http://127.0.0.1:${PORT}/json/version`); return true; } catch { return false; }
}

async function startChrome() {
  if (await devtoolsUp()) return;
  const exe = CHROME_PATHS.find((p) => fs.existsSync(p));
  if (!exe) die('Chrome not found');
  fs.mkdirSync(PROFILE, { recursive: true });
  // The background flags keep the sim's physics running even if the window
  // ends up behind PowerShell (normal Chrome pauses animation there).
  spawn(exe, [
    `--remote-debugging-port=${PORT}`, `--user-data-dir=${PROFILE}`,
    '--no-first-run', '--no-default-browser-check',
    '--disable-backgrounding-occluded-windows', '--disable-renderer-backgrounding',
    '--disable-background-timer-throttling', '--new-window', SIM_URL,
  ], { detached: true, stdio: 'ignore' }).unref();
  for (let i = 0; i < 60; i++) { if (await devtoolsUp()) return; await sleep(250); }
  die('Chrome did not start its DevTools port');
}

async function simTab() {
  for (let i = 0; i < 40; i++) {
    const tabs = await getJson(`http://127.0.0.1:${PORT}/json/list`);
    const t = tabs.find((x) => x.type === 'page' && x.url.startsWith(SIM_URL));
    if (t) return t;
    const blank = tabs.find((x) => x.type === 'page');
    if (blank && i === 8) await fetch(`http://127.0.0.1:${PORT}/json/new?${SIM_URL}`, { method: 'PUT' });
    await sleep(250);
  }
  die('simulator tab did not open');
}

function connect(wsUrl) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(wsUrl);
    let id = 0;
    const pending = new Map();
    ws.onmessage = (ev) => {
      const m = JSON.parse(ev.data);
      if (m.id && pending.has(m.id)) { pending.get(m.id)(m); pending.delete(m.id); }
    };
    ws.onerror = () => reject(new Error('DevTools websocket error'));
    // A dropped connection must fail loudly: with no pending timers Node
    // would otherwise exit silently with code 0 (seen 2026-11-14).
    ws.onclose = () => { if (!closing) die('lost the DevTools connection to the simulator window'); };
    let closing = false;
    ws.onopen = () => resolve({
      // Every request times out (the timer also keeps Node alive while waiting).
      send(method, params = {}, timeoutMs = 20000) {
        return new Promise((res, rej) => {
          const n = ++id;
          const t = setTimeout(() => { pending.delete(n); rej(new Error(`no reply to ${method}`)); }, timeoutMs);
          pending.set(n, (m) => { clearTimeout(t); res(m); });
          ws.send(JSON.stringify({ id: n, method, params }));
        });
      },
      async eval(expr, timeoutMs = 20000) {
        const m = await this.send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true }, timeoutMs);
        if (m.result && m.result.exceptionDetails) throw new Error(m.result.exceptionDetails.exception?.description || 'page error');
        return m.result?.result?.value;
      },
      close() { closing = true; ws.close(); },
    });
  });
}

// Page helpers, injected as text.
// The sim's physics runs on requestAnimationFrame, which Chrome stops
// completely when the window is minimized (and normal Chrome also when it is
// covered). The robot then freezes while the code keeps running and every
// move times out. So every frame request also arms a 20 ms timer, and
// whichever fires first runs the frame (timers keep running in the
// background: Chrome is started with --disable-background-timer-throttling).
const HELPERS = `
  if (!window.__otvRaf) {
    window.__otvRaf = window.requestAnimationFrame.bind(window);
    window.requestAnimationFrame = (cb) => {
      let done = false;
      const run = () => { if (!done) { done = true; cb(performance.now()); } };
      window.__otvRaf(run);
      setTimeout(run, 20);
      return 0;
    };
  }
  window.__B = () => [...document.querySelectorAll('button')];
  window.__btn = (re) => __B().find((b) => re.test(b.title || '') || re.test(b.textContent.trim()));
  window.__sleep = (ms) => new Promise((r) => setTimeout(r, ms));
`;

// Footprint checker (page side). Geometry from the sim's JS (index.js):
// canvas 400 px/m, arena 4 x 2 m, obstacles 0.2 x 0.5 m in columns
// x 1.4-1.6 / 2.2-2.4 and rows y 0.25-0.75 / 0.75-1.25 / 1.25-1.75,
// rumbles drawn with an extra dark line at 1/3 of the width.
// Limbo opening per the Arena Details doc: y 1.22-1.78 at x 3.35-3.45.
const CHECKER = `
  window.__chkInit = () => {
    const cv = document.querySelector('canvas');
    const g = cv.getContext('2d');
    const px = (x, y) => g.getImageData(Math.round(x * 400), Math.round(y * 400), 1, 1).data;
    const obs = [];
    for (const left of [1.4, 2.2]) for (const top of [0.25, 0.75, 1.25]) {
      const c = px(left + 0.1, top + 0.4);
      if (c[0] > 200 && c[1] > 200 && c[2] > 200) {
        const r = px(left + 0.2 / 3, top + 0.4);
        obs.push({ x0: left, x1: left + 0.2, y0: top, y1: top + 0.5, rumble: r[0] < 120 });
      }
    }
    window.__chk = { obs, out: 0, outPose: '', hitSolid: 0, hitPose: '', minClear: 9, clearPose: '',
                     rumble: false, limboMin: 9, limboMax: -9, frames: 0 };
  };
  window.__chkFrame = () => {
    const c = window.__chk; if (!c) return;
    const v = [...document.querySelectorAll('input')].filter(e => e.type !== 'checkbox').slice(1, 4).map(e => parseFloat(e.value));
    if (v.some(isNaN)) return;
    const [x, y, th] = v; c.frames++;
    const pose = x.toFixed(3) + ', ' + y.toFixed(3) + ', ' + th.toFixed(2);
    const cs = Math.cos(th), sn = Math.sin(th);
    const pts = [];
    for (let i = 0; i <= 6; i++) for (let j = 0; j <= 6; j++) {
      if (i > 0 && i < 6 && j > 0 && j < 6) continue;
      const a = -0.09 + 0.18 * i / 6, b = -0.10 + 0.20 * j / 6;
      pts.push([x + a * cs - b * sn, y + a * sn + b * cs]);
    }
    let out = 0, xmin = 9, xmax = -9, ymin = 9, ymax = -9;
    for (const [qx, qy] of pts) {
      out = Math.max(out, -qx, qx - 4, -qy, qy - 2);
      xmin = Math.min(xmin, qx); xmax = Math.max(xmax, qx); ymin = Math.min(ymin, qy); ymax = Math.max(ymax, qy);
    }
    if (out > c.out) { c.out = out; c.outPose = pose; }
    for (const o of c.obs) {
      let d = 9;
      for (const [qx, qy] of pts) {
        const dx = Math.max(o.x0 - qx, 0, qx - o.x1), dy = Math.max(o.y0 - qy, 0, qy - o.y1);
        d = Math.min(d, Math.hypot(dx, dy));
      }
      for (const [ox, oy] of [[o.x0, o.y0], [o.x1, o.y0], [o.x0, o.y1], [o.x1, o.y1]]) {
        const lx = (ox - x) * cs + (oy - y) * sn, ly = -(ox - x) * sn + (oy - y) * cs;
        if (Math.abs(lx) <= 0.09 && Math.abs(ly) <= 0.10) d = 0;
      }
      if (o.rumble) { if (d === 0) c.rumble = true; continue; }
      if (d < c.minClear) { c.minClear = d; c.clearPose = pose; }
      if (d === 0 && !c.hitSolid) { c.hitSolid = 1; c.hitPose = pose; }
    }
    if (xmax >= 3.35 && xmin <= 3.45) { c.limboMin = Math.min(c.limboMin, ymin); c.limboMax = Math.max(c.limboMax, ymax); }
  };
  if (!window.__chkHooked) {
    window.__chkHooked = true;
    const raf = window.requestAnimationFrame;
    window.requestAnimationFrame = (cb) => raf((t) => { cb(t); try { window.__chkFrame(); } catch (e) {} });
  }
`;

// Only one run may drive the sim window at a time (two runs reload and
// reprogram the same tab under each other).
const LOCK = path.join(PROFILE, 'otv-autorun.lock');
function takeLock() {
  fs.mkdirSync(PROFILE, { recursive: true });
  try {
    const pid = Number(fs.readFileSync(LOCK, 'utf8'));
    if (pid && pid !== process.pid) {
      try { process.kill(pid, 0); die(`another sim run is still going (process ${pid}). Wait for it, or close that PowerShell window.`); } catch (e) { if (e.code === 'EPERM') die(`another sim run is still going (process ${pid}).`); if (e.code !== 'ESRCH') throw e; }
    }
  } catch (e) { if (e.code !== 'ENOENT' && e.code !== 'ESRCH') throw e; }
  fs.writeFileSync(LOCK, String(process.pid));
  process.on('exit', () => { try { if (fs.readFileSync(LOCK, 'utf8') === String(process.pid)) fs.unlinkSync(LOCK); } catch {} });
}

async function main() {
  takeLock();
  console.log('Opening the simulator (its own Chrome window)...');
  await startChrome();
  const tab = await simTab();
  const cdp = await connect(tab.webSocketDebuggerUrl);
  await cdp.send('Page.enable');
  await cdp.send('Page.bringToFront');

  // Speed must be "instant" (index 2). It is read at page load, so set it, then reload.
  // A freshly opened (or reloading) tab is briefly a blank/placeholder
  // document whose localStorage throws "Access is denied for this document"
  // (seen 2026-11-14 on the first run from the OTV menu), so wait until the
  // simulator page itself is loaded and retry.
  let stored = false, lastErr = '';
  for (let i = 0; i < 80 && !stored; i++) {
    try {
      stored = await cdp.eval(`(location.href.startsWith(${JSON.stringify(SIM_URL)}) && document.readyState === 'complete')
        ? (localStorage.setItem('Editor.intervalIndex', '2'), localStorage.setItem('clearOutputWhenRun', 'true'), localStorage.setItem('resetRobotWhenRun', 'true'), true)
        : false`, 2000);
    } catch (e) { lastErr = e.message; stored = false; }
    if (!stored) await sleep(250);
  }
  if (!stored) die('simulator page did not finish loading (internet?)' + (lastErr ? ': ' + lastErr : ''));
  await cdp.send('Page.reload', { ignoreCache: false });
  let ready = false;
  for (let i = 0; i < 80 && !ready; i++) {
    await sleep(250);
    try {
      ready = await cdp.eval(`document.readyState === 'complete' && !!window.ace && !!document.querySelector('.ace_editor') && [...document.querySelectorAll('button')].some(b => b.title === 'Run code')`, 2000);
    } catch { ready = false; }
  }
  if (!ready) die('simulator page did not load (internet?)');
  await cdp.eval(HELPERS + '1');
  if (CHECK) await cdp.eval(CHECKER + '1');

  const speed = await cdp.eval(`(__btn(/execution speed/) || {}).title || ''`);
  if (!/^instant/.test(speed)) die(`speed button says "${speed}", expected instant`);

  await cdp.eval(`ace.edit(document.querySelector('.ace_editor')).setValue(${JSON.stringify(code)}, -1); 1`);
  for (const name of setupButtons) {
    const ok = await cdp.eval(`(async () => { const b = __B().find(b => b.textContent.trim() === ${JSON.stringify(name)}); if (!b) return false; b.click(); await __sleep(300); return true; })()`);
    if (!ok) die(`no "${name}" button`);
  }
  await cdp.eval(`__btn(/^Run code$/).click(); 1`);
  let cont = false;
  for (let i = 0; i < 40 && !cont; i++) {
    await sleep(250);
    cont = await cdp.eval(`!!__btn(/Continue code/)`);
  }
  if (!cont) {
    const out = await cdp.eval(`(document.querySelectorAll('textarea')[1] || {}).value || ''`);
    die('Run did not start. Simulator says:\n' + out);
  }
  if (CHECK) await cdp.eval(`(async () => { await __sleep(200); __chkInit(); return 1; })()`);
  await cdp.eval(`__btn(/Continue code/).click(); 1`);
  if (!QUIET) console.log(`Running (${setupButtons.join(' + ') || 'no setup'}, speed instant). Pose and console below.\n`);

  // Watch: print new console lines and the pose every 3 s.
  const t0 = Date.now();
  let printed = 0, lastPose = '', samePoseS = 0, finished = false, lastDone = '';
  while (Date.now() - t0 < watchMs) {
    await sleep(1000);
    const st = await cdp.eval(`({
      out: (document.querySelectorAll('textarea')[1] || {}).value || '',
      pose: [...document.querySelectorAll('input')].filter(e => e.type !== 'checkbox').slice(1, 4).map(e => e.value).join(', '),
      running: !!__btn(/Continue code|Pause code/)
    })`);
    const lines = st.out.split('\n');
    const fresh = lines.slice(printed, lines.length - 1);
    printed = Math.max(printed, lines.length - 1);
    if (!QUIET) for (const l of fresh) console.log('  sim> ' + l);
    const s = Math.round((Date.now() - t0) / 1000);
    if (!QUIET && s % 3 === 0) console.log(`  [${String(s).padStart(3)} s] x, y, theta = ${st.pose}`);
    samePoseS = st.pose === lastPose ? samePoseS + 1 : 0;
    lastPose = st.pose;
    const doneLine = fresh.find((l) => DONE.test(l));
    if (doneLine) { finished = true; lastDone = doneLine.trim(); await sleep(1500); break; }
    if (!st.running) { console.log('Simulator session ended.'); finished = true; break; }
  }
  if (!QUIET) console.log(finished ? '\nDone. The simulator window stays open.' : '\nStopped watching (time limit). The simulator keeps running.');
  else console.log(`END ${finished ? lastDone || 'finished' : 'TIME LIMIT'} | pose ${lastPose}`);
  let exitCode = 0;
  if (CHECK) {
    const c = await cdp.eval(`window.__chk`);
    const cm = (m) => (m * 100).toFixed(1) + ' cm';
    const lay = c.obs.map((o) => `${o.rumble ? 'R' : 'S'}(${(o.x0 + 0.1).toFixed(1)},${(o.y0 + 0.25).toFixed(2)})`).join(' ');
    const bad = [];
    if (c.out > 0.002) bad.push(`left the arena by ${cm(c.out)} at ${c.outPose}`);
    if (c.hitSolid) bad.push(`touched a solid obstacle at ${c.hitPose}`);
    if (c.limboMax > -9 && (c.limboMin < 1.22 || c.limboMax > 1.78)) bad.push(`limbo y ${c.limboMin.toFixed(3)}-${c.limboMax.toFixed(3)} outside 1.22-1.78`);
    if (!finished) bad.push('did not finish');
    console.log(`CHECK ${bad.length ? 'FAIL' : 'PASS'} | frames ${c.frames} | worst outside ${cm(Math.max(0, c.out))}${c.out > 0 ? ' at ' + c.outPose : ''} | min obstacle gap ${c.minClear < 9 ? cm(c.minClear) + ' at ' + c.clearPose : 'n/a'}` +
      ` | limbo y ${c.limboMax > -9 ? c.limboMin.toFixed(3) + '-' + c.limboMax.toFixed(3) : 'not reached'} | rumble ${c.rumble ? 'driven over' : 'no'} | layout ${lay}` +
      (bad.length ? '\n  ' + bad.join('\n  ') : ''));
    if (bad.length) exitCode = 2;
  }
  cdp.close();
  process.exit(exitCode);
}

main().catch((e) => die(e.message));
