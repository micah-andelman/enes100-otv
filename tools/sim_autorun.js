// Runs a sim sketch in the ENES100 browser simulator with no clicks:
// opens a dedicated Chrome window, loads the code, sets the execution speed
// to "instant", clicks the setup buttons, Run, then Continue, and prints
// the robot pose + simulator console here until the run finishes.
//
//   node --experimental-websocket tools/sim_autorun.js <sketch.ino> [Setup,Buttons] [watchSeconds]
//   e.g. node --experimental-websocket tools/sim_autorun.js sim/ms5/task7a_wallhug.ino Randomize 150
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
const DONE = /Run complete|Stopped before the limbo|aborting|MS5 4: done|MS5 5: sent (un)?polluted|MS5 7b: past the obstacles|MS5 7b: (vision lost|timeout|every lane blocked)|STOPPED:|Program finished/;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const die = (msg) => { console.error('SIM AUTORUN FAILED: ' + msg); process.exit(1); };

const [, , sketchArg, setupArg = 'Randomize', watchArg = '150'] = process.argv;
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
    ws.onopen = () => resolve({
      send(method, params = {}) {
        return new Promise((res) => { const n = ++id; pending.set(n, res); ws.send(JSON.stringify({ id: n, method, params })); });
      },
      async eval(expr) {
        const m = await this.send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true });
        if (m.result && m.result.exceptionDetails) throw new Error(m.result.exceptionDetails.exception?.description || 'page error');
        return m.result?.result?.value;
      },
      close() { ws.close(); },
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

async function main() {
  console.log('Opening the simulator (its own Chrome window)...');
  await startChrome();
  const tab = await simTab();
  const cdp = await connect(tab.webSocketDebuggerUrl);
  await cdp.send('Page.enable');
  await cdp.send('Page.bringToFront');

  // Speed must be "instant" (index 2). It is read at page load, so set it, then reload.
  await cdp.eval(`localStorage.setItem('Editor.intervalIndex', '2'); localStorage.setItem('clearOutputWhenRun', 'true'); localStorage.setItem('resetRobotWhenRun', 'true'); 1`);
  await cdp.send('Page.reload', { ignoreCache: false });
  let ready = false;
  for (let i = 0; i < 80 && !ready; i++) {
    await sleep(250);
    try {
      ready = await cdp.eval(`document.readyState === 'complete' && !!window.ace && !!document.querySelector('.ace_editor') && [...document.querySelectorAll('button')].some(b => b.title === 'Run code')`);
    } catch { ready = false; }
  }
  if (!ready) die('simulator page did not load (internet?)');
  await cdp.eval(HELPERS + '1');

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
  await cdp.eval(`__btn(/Continue code/).click(); 1`);
  console.log(`Running (${setupButtons.join(' + ') || 'no setup'}, speed instant). Pose and console below.\n`);

  // Watch: print new console lines and the pose every 3 s.
  const t0 = Date.now();
  let printed = 0, lastPose = '', samePoseS = 0, finished = false;
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
    for (const l of fresh) console.log('  sim> ' + l);
    const s = Math.round((Date.now() - t0) / 1000);
    if (s % 3 === 0) console.log(`  [${String(s).padStart(3)} s] x, y, theta = ${st.pose}`);
    samePoseS = st.pose === lastPose ? samePoseS + 1 : 0;
    lastPose = st.pose;
    if (fresh.some((l) => DONE.test(l))) { finished = true; await sleep(1500); break; }
    if (!st.running) { console.log('Simulator session ended.'); finished = true; break; }
  }
  console.log(finished ? '\nDone. The simulator window stays open.' : '\nStopped watching (time limit). The simulator keeps running.');
  cdp.close();
}

main().catch((e) => die(e.message));
