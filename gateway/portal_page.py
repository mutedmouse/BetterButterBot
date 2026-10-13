"""The robot portal page served by camera.py: camera view, VC/RC and CS switches, joystick and chat.

Kept as Python strings so every install path (native installers, Docker, file deploys) that copies
*.py picks it up. The page loads no inline script: the Content-Security-Policy only allows /app.js.
"""

PAGE = """<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ButterBot portal</title><link rel="stylesheet" href="/app.css"></head><body>
<header>
  <span class="title">ButterBot</span>
  <div class="seg" id="mode" role="group" aria-label="Control mode">
    <button type="button" data-mode="vc" title="Voice Control: wake word and voice commands">VC</button>
    <button type="button" data-mode="rc" title="Remote Control: you drive, voice is off">RC</button>
  </div>
  <button type="button" class="toggle" id="cs" aria-pressed="false" title="Camera stream on/off">CS</button>
  <span class="battery" id="battery" hidden></span>
  <span class="battery" id="signal" hidden></span>
  <span class="spacer"></span>
  <span class="who">{user}</span>
  <form method="post" action="/logout"><button type="submit" class="plain">Sign out</button></form>
</header>
<main>
  <section class="view">
    <img id="video" alt="Camera stream">
    <div class="overlay" id="overlay">Camera off</div>
  </section>
  <div class="status" id="status">Connecting&hellip;</div>
  <section class="controls">
    <div class="drive">
      <div class="pad" id="pad" aria-label="Drive joystick"><div class="dead"></div><div class="knob" id="knob"></div></div>
      <div class="tracks" id="tracks" aria-live="off"></div>
      <div class="hint" id="drivehint">Switch to RC to drive</div>
      <button type="button" id="keys" class="keys" aria-pressed="false" title="Shift+D turns arrow-key driving on or off; Esc turns it off">&#9000; Arrow keys: off <kbd>Shift+D</kbd></button>
    </div>
    <div class="chat">
      <ol class="log" id="log" aria-live="polite"></ol>
      <form id="say" autocomplete="off">
        <input id="text" maxlength="300" placeholder="Type something for ButterBot to say" aria-label="Text to say">
        <button type="submit" id="send">Send</button>
      </form>
      <button type="button" id="listen" class="listen">&#127908; Listen</button>
    </div>
  </section>
</main>
<script src="/app.js"></script></body></html>"""

STYLE = """:root{--bg:#111;--panel:#1c1c1c;--line:#333;--text:#ddd;--dim:#888;--accent:#d9a400;--accent-text:#111;--bad:#f77;--you:#2a2a2a;--heard:#1f2b36}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px system-ui,sans-serif}
header{display:flex;align-items:center;gap:10px;padding:8px 12px;border-bottom:1px solid var(--line);flex-wrap:wrap}
header form{margin:0}
.title{font-weight:700;color:var(--accent)}
.spacer{flex:1}
.who{color:var(--dim)}
.battery{font-size:14px;padding:4px 9px;border-radius:6px;border:1px solid var(--line);white-space:nowrap}
.battery.low{color:#f0b429;border-color:#f0b429}
.battery.crit{color:var(--bad);border-color:var(--bad);font-weight:600}
.battery.charging{color:#7bd389;border-color:#7bd389}
button{font:inherit;padding:7px 14px;border:1px solid var(--line);border-radius:6px;background:var(--panel);color:var(--text);cursor:pointer}
button:disabled{opacity:.4;cursor:not-allowed}
button.plain{background:none}
.seg{display:inline-flex}
.seg button{border-radius:0}
.seg button:first-child{border-radius:6px 0 0 6px}
.seg button:last-child{border-radius:0 6px 6px 0;border-left:0}
.seg button.on,.toggle.on{background:var(--accent);color:var(--accent-text);border-color:var(--accent);font-weight:600}
main{max-width:660px;margin:0 auto;padding:10px 16px 24px}
.view{position:relative;width:100%;aspect-ratio:4/3;background:#000;border-radius:8px;overflow:hidden}
.view img{width:100%;height:100%;object-fit:contain;display:block}
.overlay{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:var(--dim);background:rgba(0,0,0,.55);font-size:18px}
.overlay[hidden]{display:none}
.status{min-height:1.4em;margin:8px 2px;color:var(--dim);font-size:14px}
.status.bad{color:var(--bad)}
.controls{display:grid;grid-template-columns:auto 1fr;gap:16px;align-items:start}
.drive{display:flex;flex-direction:column;align-items:center;gap:8px}
.pad{position:relative;width:180px;height:180px;border-radius:50%;background:var(--panel);border:2px solid var(--line);touch-action:none;user-select:none}
.dead{position:absolute;left:50%;top:50%;width:calc(var(--dz,12px) * 2);height:calc(var(--dz,12px) * 2);transform:translate(-50%,-50%);border-radius:50%;border:2px dashed #555}
.pad.active.idle .knob{background:#8a7420;border-color:#b0941f}
.knob{position:absolute;left:50%;top:50%;width:64px;height:64px;margin:-32px;border-radius:50%;background:#555;border:2px solid #777;cursor:grab;box-shadow:0 2px 6px rgba(0,0,0,.5);transition:transform .22s cubic-bezier(.3,1.6,.5,1),width .15s,height .15s,margin .15s}
.pad.active .knob{width:22px;height:22px;margin:-11px;background:var(--accent);border-color:var(--accent);cursor:grabbing;transition:width .1s,height .1s,margin .1s}
.pad.off.active .knob{background:#777;border-color:#999}
.pad.off{opacity:.6}
.tracks{font:13px ui-monospace,monospace;color:var(--accent);min-height:1.2em}
.hint{font-size:13px;color:var(--dim);text-align:center;max-width:180px}
.hint.flash{animation:flash 1.2s}
.keys{font-size:13px;padding:5px 10px}
.keys.on{background:var(--accent);color:var(--accent-text);border-color:var(--accent);font-weight:600}
.keys kbd{font:11px ui-monospace,monospace;opacity:.7;margin-left:4px}
@keyframes flash{0%,40%{color:var(--accent)}}
.chat{display:flex;flex-direction:column;gap:8px;min-width:0}
.log{list-style:none;margin:0;padding:8px;height:180px;overflow-y:auto;background:var(--panel);border:1px solid var(--line);border-radius:8px}
.log li{margin:4px 0;padding:5px 9px;border-radius:6px;overflow-wrap:anywhere}
.log li.you{background:var(--you)}
.log li.heard{background:var(--heard)}
.log li.note{color:var(--dim);font-size:13px;padding:2px 9px}
.log .tag{color:var(--dim);font-size:12px;margin-right:6px}
#say{display:flex;gap:8px}
#say input{flex:1;min-width:0;padding:8px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--text);font:inherit}
#send{background:var(--accent);color:var(--accent-text);border-color:var(--accent);font-weight:600}
.listen.on{background:var(--heard);border-color:#4a7ca8}
@media (max-width:560px){.controls{grid-template-columns:1fr}.drive{order:0}}
"""

SCRIPT = r"""'use strict';
(() => {
const $ = id => document.getElementById(id);
const video = $('video'), overlay = $('overlay'), statusEl = $('status'), log = $('log');
const pad = $('pad'), knob = $('knob'), driveHint = $('drivehint'), tracks = $('tracks');
const csBtn = $('cs'), modeBtns = [...document.querySelectorAll('#mode button')];
const text = $('text'), send = $('send'), listen = $('listen');
let ws = null, st = null, retry = 1000, streamUrlOn = false, pingTimer = null;

// ---- socket ----
function connect() {
  ws = new WebSocket((location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws');
  ws.onopen = () => { retry = 1000; clearInterval(pingTimer); pingTimer = setInterval(() => sendMsg({type: 'ping'}), 25000); };
  ws.onmessage = e => { let m; try { m = JSON.parse(e.data); } catch { return; } handle(m); };
  ws.onclose = e => {
    clearInterval(pingTimer); st = null; render();
    if (e.code === 1008) { location.reload(); return; }   // signed out elsewhere
    setTimeout(connect, retry); retry = Math.min(retry * 2, 15000);
  };
}
function sendMsg(m) { if (ws && ws.readyState === 1) { ws.send(JSON.stringify(m)); return true; } return false; }
function handle(m) {
  if (m.type === 'status') { st = m; if (m.deadzone) setDeadzone(m.deadzone); render(); }
  else if (m.type === 'history') { log.textContent = ''; (m.lines || []).forEach(addLine); }
  else if (m.type === 'chat') addLine(m);
  else if (m.type === 'drive' && (dragging || keyTimer)) tracks.textContent = m.l || m.r ? `L ${m.l}  R ${m.r}` : 'stopped';
}

// ---- chat ----
const TAGS = {you: 'you said', heard: 'heard', note: ''};
function addLine(l) {
  const li = document.createElement('li');
  li.className = l.who;
  if (TAGS[l.who]) { const t = document.createElement('span'); t.className = 'tag'; t.textContent = TAGS[l.who]; li.append(t); }
  li.append(document.createTextNode(l.text));
  const atEnd = log.scrollHeight - log.scrollTop - log.clientHeight < 30;
  log.append(li);
  while (log.children.length > 200) log.firstChild.remove();
  if (atEnd) log.scrollTop = log.scrollHeight;
}
$('say').addEventListener('submit', e => {
  e.preventDefault();
  const t = text.value.trim();
  if (t && sendMsg({type: 'say', text: t})) text.value = '';
});
listen.addEventListener('click', () => sendMsg({type: 'listen'}));

// ---- switches ----
csBtn.addEventListener('click', () => { if (st) sendMsg({type: 'camera', on: !st.camera.wanted}); });
modeBtns.forEach(b => b.addEventListener('click', () => sendMsg({type: 'mode', mode: b.dataset.mode})));

// ---- joystick ----
// The knob can always be grabbed and moved anywhere inside the ring; how far it is from the centre
// sets the speed. While held it shrinks to a third of its size (a fingertip grip with more room to
// travel); letting go springs it back to the centre at full size, which stops the robot. Only the
// driver's stick (RC mode) is sent; otherwise the hint flashes to say why nothing moves.
const GRIP = 22;             // held knob diameter, px: a third of the 64 px knob (matches the CSS)
let deadzone = 0.4, dragging = false, driving = false, vec = {x: 0, y: 0}, driveTimer = null;
function travel() { return (pad.clientWidth - GRIP) / 2; }   // how far the held knob's centre can go
function setDeadzone(dz) {
  deadzone = dz;
  pad.style.setProperty('--dz', (travel() * dz) + 'px');
}
function canDrive() { return st && st.mode === 'rc' && st.driver_is_you; }
function setKnob(dx, dy) { knob.style.transform = `translate(${dx}px,${dy}px)`; }
function padMove(e) {
  // The held knob sits under the finger, so grabbing the knob off-centre moves it within the dead zone.
  const r = pad.getBoundingClientRect(), radius = travel();
  let dx = e.clientX - (r.left + r.width / 2), dy = e.clientY - (r.top + r.height / 2);
  const d = Math.hypot(dx, dy);
  if (d > radius) { dx *= radius / d; dy *= radius / d; }
  setKnob(dx, dy);
  vec = {x: +(dx / radius).toFixed(2), y: +(-dy / radius).toFixed(2)};
  // Inside the dashed ring the robot stops, but the stick stays held: swing through it to reverse.
  pad.classList.toggle('idle', Math.hypot(vec.x, vec.y) <= deadzone);
}
function stopDrive() {
  if (!dragging) return;
  dragging = false; pad.classList.remove('active', 'idle'); setKnob(0, 0);   // springs back (CSS transition)
  clearInterval(driveTimer); driveTimer = null; vec = {x: 0, y: 0}; tracks.textContent = '';
  if (driving) sendMsg({type: 'drive', x: 0, y: 0});
  driving = false;
}
pad.addEventListener('pointerdown', e => {
  e.preventDefault();
  dragging = true; pad.classList.add('active'); pad.setPointerCapture(e.pointerId); padMove(e);
  driving = canDrive();
  if (!driving) { driveHint.classList.remove('flash'); void driveHint.offsetWidth; driveHint.classList.add('flash'); return; }
  sendMsg({type: 'drive', ...vec});
  driveTimer = setInterval(() => sendMsg({type: 'drive', ...vec}), 100);   // the robot stops if these stop
});
pad.addEventListener('pointermove', e => { if (dragging) padMove(e); });
['pointerup', 'pointercancel', 'lostpointercapture'].forEach(t => pad.addEventListener(t, stopDrive));
window.addEventListener('blur', () => { stopDrive(); releaseKeys(); });
document.addEventListener('visibilitychange', () => { if (document.hidden) { stopDrive(); releaseKeys(); } });

// ---- arrow-key driving ----
// Shift+D (or the button) turns it on; Esc or Shift+D turns it off. Keys drive at full speed only:
// up/down forward/back, left/right spin, one of each for a diagonal; opposite keys cancel out.
// Held keys repeat the stick message every 100 ms like the joystick; releasing them all stops.
const keysBtn = $('keys'), ARROWS = ['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'];
const held = new Set();
let keysOn = false, keyTimer = null;
function keyVec() {
  return {x: (held.has('ArrowRight') ? 1 : 0) - (held.has('ArrowLeft') ? 1 : 0),
          y: (held.has('ArrowUp') ? 1 : 0) - (held.has('ArrowDown') ? 1 : 0)};
}
function keyUpdate() {
  const v = keyVec(), moving = !!(v.x || v.y) && keysOn && canDrive();
  if (!dragging) {   // show it on the joystick: the knob pushed to the rim in that direction
    const n = Math.hypot(v.x, v.y) || 1, r = travel();
    setKnob(moving ? v.x / n * r : 0, moving ? -v.y / n * r : 0);
    pad.classList.toggle('active', moving);
  }
  if (moving) {
    sendMsg({type: 'drive', x: v.x, y: v.y});
    if (!keyTimer) keyTimer = setInterval(keyUpdate, 100);   // the robot stops if these stop
  } else if (keyTimer) {
    clearInterval(keyTimer); keyTimer = null; tracks.textContent = '';
    sendMsg({type: 'drive', x: 0, y: 0});
  }
}
function releaseKeys() { held.clear(); keyUpdate(); }
function setKeys(on) {
  keysOn = on; releaseKeys();
  keysBtn.classList.toggle('on', on); keysBtn.setAttribute('aria-pressed', on);
  keysBtn.firstChild.textContent = on ? '\u2328 Arrow keys: on ' : '\u2328 Arrow keys: off ';
}
keysBtn.addEventListener('click', () => { setKeys(!keysOn); keysBtn.blur(); });
document.addEventListener('keydown', e => {
  const typing = !!(e.target.closest && e.target.closest('input, textarea'));
  if (typing) return;
  if (e.shiftKey && !e.ctrlKey && !e.altKey && !e.metaKey && e.code === 'KeyD') { e.preventDefault(); setKeys(!keysOn); return; }
  if (e.key === 'Escape' && keysOn) { setKeys(false); return; }
  if (!keysOn || !ARROWS.includes(e.key)) return;
  e.preventDefault();   // no page scrolling while driving
  if (!canDrive()) { driveHint.classList.remove('flash'); void driveHint.offsetWidth; driveHint.classList.add('flash'); return; }
  if (!held.has(e.key)) { held.add(e.key); keyUpdate(); }
});
document.addEventListener('keyup', e => {
  if (held.delete(e.key)) keyUpdate();
});

// ---- rendering ----
function setStatus(t, bad) { statusEl.textContent = t; statusEl.classList.toggle('bad', !!bad); }
function render() {
  const online = !!st, robot = online && st.robot;
  // camera
  const cam = online ? st.camera : {wanted: false, on: false};
  csBtn.classList.toggle('on', cam.wanted); csBtn.setAttribute('aria-pressed', cam.wanted);
  csBtn.disabled = !robot || !st.camera_supported;
  if (cam.on && !streamUrlOn) { video.src = '/stream?t=' + Date.now(); streamUrlOn = true; }
  if (!cam.wanted && streamUrlOn) { video.removeAttribute('src'); streamUrlOn = false; }
  overlay.hidden = cam.on;
  overlay.textContent = !cam.wanted ? 'Camera off' : 'Camera paused';
  // battery: reported by the robot every minute and when the charger is plugged in or out
  const bat = robot ? st.battery : null, batEl = $('battery');
  batEl.hidden = !bat;
  if (bat) {
    const plugged = bat.charging !== 'unplugged';
    // While charging the robot can't measure its level (the charger lifts the voltage), so no number then.
    batEl.textContent = bat.charging === 'full' ? '\u26a1 Full' : plugged ? '\u26a1 Charging' : '\ud83d\udd0b ' + bat.percent + '%';
    batEl.className = 'battery' + (plugged ? ' charging' : bat.percent < 15 ? ' crit' : bat.percent < 25 ? ' low' : '');
    batEl.title = (plugged ? 'On the charger: the level shows again once unplugged' : `Battery ${bat.percent}% (measured while parked)`) + `; updated ${new Date(bat.at * 1000).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'})}`;
  }
  // Wi-Fi signal at the robot (dBm): every 5 s in RC, every minute otherwise
  const sigEl = $('signal'), rssi = bat ? bat.rssi : null;
  sigEl.hidden = rssi == null;
  if (rssi != null) {
    const bars = rssi >= -60 ? 4 : rssi >= -67 ? 3 : rssi >= -74 ? 2 : 1;
    sigEl.textContent = '\ud83d\udcf6 ' + '\u2582\u2584\u2586\u2588'.slice(0, bars).padEnd(4, '\u00b7') + ' ' + rssi + ' dBm';
    sigEl.className = 'battery' + (bars <= 1 ? ' crit' : bars === 2 ? ' low' : '');
    sigEl.title = 'Wi-Fi signal at the robot. Below about -74 dBm the camera gets choppy; below -78 the link may drop (RC then stops the robot).';
  }
  const batteryCrit = !!bat && bat.charging === 'unplugged' && bat.percent < 15;
  // mode
  const mode = online ? st.mode : 'vc';
  modeBtns.forEach(b => {
    b.classList.toggle('on', b.dataset.mode === mode);
    b.disabled = !robot || !st.rc_supported || (!!st.driver && !st.driver_is_you);
  });
  // joystick
  const drive = canDrive();
  pad.classList.toggle('off', !drive);
  if (!drive && driving) stopDrive();   // lost RC or the driver seat mid-drag
  if (!drive && keyTimer) releaseKeys();
  driveHint.textContent = !robot ? '' : !st.rc_supported ? 'Driving needs a robot firmware update'
    : st.mode !== 'rc' ? 'Switch to RC to drive'
    : !st.driver_is_you ? `Driven by ${st.driver}` : st.blocked ? `Can't move: ${st.blocked}` : keysOn ? 'Drag, or hold the arrow keys' : 'Drag to drive';
  // chat
  const busy = robot && (st.speaking || st.listening);
  send.disabled = !robot;
  listen.disabled = !robot || !st.listen_supported || busy;
  listen.classList.toggle('on', robot && st.listening);
  listen.title = robot && !st.listen_supported ? 'Listening needs a robot firmware update' : 'Listen once and show what was heard';
  // status line
  if (!online) setStatus('Connecting to the gateway…', true);
  else if (!robot) setStatus('ButterBot is not connected to the gateway.', true);
  else if (batteryCrit) setStatus(`Battery ${bat.percent}%: head back to the charger.`, true);
  else if (st.listening) setStatus('Listening…');
  else if (st.speaking) setStatus('Speaking…');
  else if (cam.wanted && cam.on) setStatus(`Streaming at ${cam.mode} (shown at 640×480).`);
  else if (cam.wanted) setStatus('Camera paused while ButterBot uses it; it resumes by itself.');
  else setStatus(mode === 'rc' ? 'Remote control: voice is off.' : 'Voice control.');
}
render();
connect();
})();
"""
