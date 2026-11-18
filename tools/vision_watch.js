// Prints one team's live data from the ENES100 vision system in the terminal:
// the pose row (marker, visible, x, y, theta) and everything the Arduino
// prints with Enes100.print/println (team console), plus system lines that
// mention the team (mission messages, connects).
// Read-only: it listens to the same WebSocket the vision page uses.
//
//   node --experimental-websocket tools/vision_watch.js [--team "H2Woah!"] [--url ws://host:8080/ws] [--seconds N]
//
// Called by otvwatch / otv4tank / otv5tank in tools/otv.ps1. Ctrl+C to quit.

const args = process.argv.slice(2);
function arg(name, def) {
  const i = args.indexOf(name);
  return i >= 0 && i + 1 < args.length ? args[i + 1] : def;
}
const team = arg('--team', 'H2Woah!');
const url = arg('--url', 'ws://lx-enes100-4.eng.umd.edu:8080/ws');
const seconds = Number(arg('--seconds', '0'));
const POSE_EVERY_MS = 500;  // the server pushes the roster ~5x/s; print at 2 Hz

const C = { dim: '\x1b[90m', green: '\x1b[32m', yellow: '\x1b[33m', cyan: '\x1b[36m', reset: '\x1b[0m' };
const stamp = () => new Date().toTimeString().slice(0, 8);

let lastPose = 0;
let lastState = '';
let consoleBuf = '';
let everSeen = false;

function printPose(st) {
  if (!st) {
    if (lastState !== 'absent') {
      console.log(`${C.yellow}[${stamp()}] ${team}: not on the vision system yet (Arduino not connected)${C.reset}`);
      lastState = 'absent';
    }
    return;
  }
  const conn = st.connected ? 'connected' : 'DISCONNECTED';
  if (!st.connected || !st.visible) {
    const s = `${conn}|${st.visible}|${st.aruco}`;
    if (s !== lastState) {
      console.log(`${C.yellow}[${stamp()}] ${team}: ${conn}, marker ${st.aruco} ${st.visible ? 'visible' : 'NOT VISIBLE'}${C.reset}`);
      lastState = s;
    }
    return;
  }
  lastState = 'ok';
  const deg = (st.theta * 180 / Math.PI).toFixed(1);
  console.log(`${C.cyan}[${stamp()}] VISION  marker ${st.aruco}  x = ${st.x.toFixed(3)} m   y = ${st.y.toFixed(3)} m   theta = ${st.theta.toFixed(3)} rad (${deg} deg)${C.reset}`);
}

// Team prints arrive as stream chunks; print complete lines only.
function teamChunk(text) {
  consoleBuf += text;
  let i;
  while ((i = consoleBuf.indexOf('\n')) >= 0) {
    const line = consoleBuf.slice(0, i).replace(/\r$/, '');
    consoleBuf = consoleBuf.slice(i + 1);
    if (line.length) console.log(`${C.green}[${stamp()}] ARDUINO ${line}${C.reset}`);
  }
}

function connect() {
  const ws = new WebSocket(url);
  ws.onopen = () => console.log(`${C.dim}[${stamp()}] Listening to ${url} for team "${team}". Ctrl+C to quit.${C.reset}`);
  ws.onmessage = (ev) => {
    let m;
    try { m = JSON.parse(String(ev.data || '')); } catch (_) { return; }
    if ((m.type === 'team_log' || m.type === 'team_raw') && m.team === team) {
      teamChunk(String(m.line));
    } else if (m.type === 'system_log' && typeof m.line === 'string' && m.line.includes(team)) {
      console.log(`${C.dim}[${stamp()}] SYSTEM  ${m.line}${C.reset}`);
    } else if (m.type === 'team_roster' && Array.isArray(m.teams)) {
      const st = m.teams.find(t => t && t.name === team);
      if (st) everSeen = true;
      const now = Date.now();
      if (now - lastPose >= POSE_EVERY_MS) {
        lastPose = now;
        printPose(st);
      }
    }
  };
  ws.onclose = () => {
    console.log(`${C.yellow}[${stamp()}] Vision system connection closed, retrying...${C.reset}`);
    setTimeout(connect, 1000);
  };
  ws.onerror = () => { try { ws.close(); } catch (_) {} };
}

connect();
if (seconds > 0) setTimeout(() => process.exit(everSeen ? 0 : 2), seconds * 1000);
