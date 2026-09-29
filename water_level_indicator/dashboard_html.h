// Dashboard web page, served from flash. Works offline (no external files).
#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0f766e">
<title>YUCCA Tank Water Level</title>
<style>
:root{--bg:#f1f5f9;--card:#fff;--text:#0f172a;--muted:#64748b;--line:#e2e8f0;--accent:#0f766e;--accent2:#14b8a6;
--danger:#dc2626;--warn:#d97706;--ok:#16a34a;--input:#f8fafc;--shadow:0 1px 3px rgba(15,23,42,.08),0 4px 16px rgba(15,23,42,.05)}
@media (prefers-color-scheme:dark){:root{--bg:#0b1220;--card:#131c2e;--text:#e2e8f0;--muted:#94a3b8;--line:#243047;
--input:#0f1726;--shadow:0 1px 3px rgba(0,0,0,.4)}}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;-webkit-text-size-adjust:100%}
body{padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left)}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:16px 20px;background:var(--accent);color:#fff}
header h1{font-size:18px;margin:0;font-weight:600;line-height:1.2}
header small{display:block;opacity:.8;font-weight:400;font-size:12px}
.chip{display:inline-flex;align-items:center;gap:6px;padding:5px 12px;border-radius:999px;font-size:13px;font-weight:600;background:rgba(255,255,255,.18);white-space:nowrap}
.chip i{width:8px;height:8px;border-radius:50%;background:#a7f3d0}
.chip.bad i{background:#fca5a5}.chip.warn i{background:#fcd34d}
main{max-width:1100px;margin:0 auto;padding:16px;display:grid;gap:16px;grid-template-columns:1fr}
@media (min-width:860px){main{grid-template-columns:380px 1fr;grid-template-areas:'setup setup' 'level cal' 'dev disp' 'wifi wifi' 'log log';padding:24px;align-items:start}
#setupBanner{grid-area:setup}#cLevel{grid-area:level}#cCal{grid-area:cal}#cDisp{grid-area:disp}#cWifi{grid-area:wifi}#cDev{grid-area:dev}#cLog{grid-area:log}}
.card{background:var(--card);border-radius:14px;box-shadow:var(--shadow);padding:18px}
.card h2{font-size:15px;margin:0 0 14px;color:var(--muted);font-weight:600;letter-spacing:.02em}
.hero{display:flex;gap:20px;align-items:stretch}
.tank{position:relative;width:110px;min-width:110px;height:230px;border:3px solid var(--line);border-top:none;border-radius:0 0 18px 18px;overflow:hidden;background:var(--input)}
.water{position:absolute;left:0;right:0;bottom:0;height:0;transition:height .8s ease,background-color .8s ease}
.water:before{content:"";position:absolute;left:0;right:0;top:-6px;height:12px;background:inherit;border-radius:50%;opacity:.7}
.leds{display:flex;flex-direction:column-reverse;gap:3px;height:230px;padding:2px 0}
.led{width:12px;flex:1 1 0;min-height:1px;max-height:6px;margin:auto 0;border-radius:2px;background:var(--line);transition:background-color .5s}
.big{font-size:56px;font-weight:700;line-height:1;margin:6px 0 4px}
.big small{font-size:22px;color:var(--muted);font-weight:600}
.kv{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;font-size:14px;margin-top:14px}
.kv span{color:var(--muted)}.kv b{font-weight:600;text-align:right}
.alert{margin-top:12px;padding:10px 12px;border-radius:10px;font-size:13px;font-weight:600;display:none}
.alert.show{display:block}.alert.ok{background:rgba(22,163,74,.12);color:var(--ok)}.alert.info{background:rgba(15,118,110,.12);color:var(--accent)}
.alert a{color:inherit}.alert.bad{background:rgba(220,38,38,.12);color:var(--danger)}.alert.warn{background:rgba(217,119,6,.12);color:var(--warn)}
.live{display:flex;align-items:baseline;justify-content:space-between;padding:12px 14px;border-radius:10px;background:var(--input);margin-bottom:14px}
.live b{font-size:26px}
.row2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
label{display:block;font-size:13px;color:var(--muted);margin-bottom:6px}
input[type=number],input[type=text],input[type=password]{width:100%;padding:11px 12px;font-size:16px;border:1px solid var(--line);border-radius:10px;background:var(--input);color:var(--text)}
input[type=range]{width:100%;accent-color:var(--accent)}
.field{margin-bottom:14px}
.btn{appearance:none;border:none;border-radius:10px;padding:12px 14px;min-height:44px;font-size:15px;font-weight:600;cursor:pointer;background:var(--accent);color:#fff;width:100%}
.btn:active{transform:scale(.98)}.btn:disabled{opacity:.5;cursor:not-allowed}
.btn.sec{background:var(--input);color:var(--text);border:1px solid var(--line)}
.btn.danger{background:transparent;color:var(--danger);border:1px solid var(--danger)}
.btns{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:12px}
.toggle{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:10px 0;border-top:1px solid var(--line);font-size:14px}
.toggle:first-of-type{border-top:none}
.sw{position:relative;width:48px;height:28px;flex:none}.sw input{opacity:0;width:0;height:0}
.sw i{position:absolute;inset:0;border-radius:999px;background:var(--line);transition:.2s;cursor:pointer}
.sw i:after{content:"";position:absolute;left:3px;top:3px;width:22px;height:22px;border-radius:50%;background:#fff;transition:.2s;box-shadow:0 1px 2px rgba(0,0,0,.3)}
.sw input:checked+i{background:var(--accent)}.sw input:checked+i:after{transform:translateX(20px)}
.help{font-size:12px;color:var(--muted);margin:6px 0 0;line-height:1.5}
.toast{position:fixed;left:50%;bottom:calc(20px + env(safe-area-inset-bottom));transform:translateX(-50%) translateY(120px);background:#0f172a;color:#fff;padding:12px 18px;border-radius:12px;font-size:14px;transition:transform .25s,opacity .25s;z-index:9;opacity:0;visibility:hidden;max-width:90vw;text-align:center}
.toast.show{transform:translateX(-50%) translateY(0);opacity:1;visibility:visible}.toast.err{background:var(--danger)}
.rssi{display:inline-flex;gap:2px;align-items:flex-end;height:14px;vertical-align:middle;margin-left:6px}
.rssi i{width:4px;background:var(--line);border-radius:1px}.rssi i.on{background:var(--ok)}
.modal{position:fixed;inset:0;background:rgba(15,23,42,.55);display:none;align-items:center;justify-content:center;padding:20px;z-index:10}
.modal.show{display:flex}.modal .card{max-width:380px;width:100%}.modal p{margin:0 0 18px;line-height:1.5;font-size:15px}
@media (max-width:380px){.tank{width:84px;min-width:84px}.big{font-size:44px}.hero{gap:14px}}
.wgrid{display:grid;gap:18px}@media (min-width:860px){.wgrid{grid-template-columns:1fr 1fr;gap:32px}}
.scanhead{display:flex;align-items:center;justify-content:space-between;margin-bottom:8px}.scanhead label{margin:0}
.btn.small{width:auto;min-height:36px;padding:8px 16px;font-size:14px}
.netlist{border:1px solid var(--line);border-radius:10px;max-height:230px;overflow-y:auto;margin-bottom:14px;background:var(--input)}
.netlist .help{padding:12px;margin:0}
.net{display:flex;align-items:center;justify-content:space-between;width:100%;gap:10px;padding:12px 14px;border:none;border-top:1px solid var(--line);background:transparent;color:var(--text);font-size:15px;text-align:left;cursor:pointer;min-height:44px}
.net:first-child{border-top:none}.net:hover,.net.sel{background:rgba(20,184,166,.12)}
.net span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.net small{color:var(--muted);flex:none;font-size:12px}
.pw{position:relative}.pw input{padding-right:70px}
.eye{position:absolute;right:6px;top:50%;transform:translateY(-50%);border:none;background:transparent;color:var(--accent);font-weight:600;font-size:13px;padding:8px;cursor:pointer}
.banner{background:var(--accent);color:#fff;border-radius:14px;padding:14px 18px;font-size:14px;line-height:1.5;display:none}
.banner.show{display:block}.banner a{color:#fff;font-weight:700}
.stats{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:14px}
.stat{background:var(--input);border:1px solid var(--line);border-radius:10px;padding:8px 12px;font-size:13px;color:var(--muted)}
.stat b{display:block;font-size:18px;color:var(--text)}.stat.bad b{color:var(--danger)}.stat.warn b{color:var(--warn)}
.logbar{display:flex;flex-wrap:wrap;align-items:center;gap:10px;margin-bottom:10px}
.logbar select{padding:9px 10px;font-size:14px;border:1px solid var(--line);border-radius:10px;background:var(--input);color:var(--text)}
.chk{display:flex;align-items:center;gap:6px;font-size:14px;color:var(--text);margin:0}
.logbtns{display:flex;gap:8px;margin-left:auto}
.logbox{max-height:440px;overflow:auto;border:1px solid var(--line);border-radius:10px;background:var(--input);font:12.5px/1.45 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}
.logbox .help{padding:12px;margin:0;font-family:inherit}
.lr{display:grid;grid-template-columns:150px 56px 1fr;gap:10px;padding:7px 12px;border-top:1px solid var(--line)}
.lr:first-child{border-top:none}.lr time{color:var(--muted);white-space:nowrap}
.lr .tag{color:var(--muted);text-transform:uppercase;font-size:11px;padding-top:1px}
.lr.W .msg{color:var(--warn)}.lr.E .msg{color:var(--danger);font-weight:600}
.lr.boot{background:rgba(15,118,110,.10)}.lr.boot .msg{font-weight:600}
@media (max-width:600px){.lr{grid-template-columns:auto 1fr;gap:2px 10px}.lr .msg{grid-column:1/-1}.logbtns{margin-left:0}}
footer{text-align:center;color:var(--muted);font-size:12px;padding:8px 0 24px}
</style>
</head>
<body>
<header>
  <h1>YUCCA Tank<small>Water level monitor</small></h1>
  <div class="chip" id="chip"><i></i><span id="chipText">Connecting…</span></div>
</header>

<main>
  <div class="banner" id="setupBanner">Not connected to your home Wi-Fi yet. <a href="#cWifi">Set up Wi-Fi</a> to use the dashboard from your home network.</div>

  <section class="card" id="cLevel">
    <h2>WATER LEVEL</h2>
    <div class="hero">
      <div class="tank"><div class="water" id="water"></div></div>
      <div class="leds" id="leds" title="LED strip preview"></div>
      <div style="flex:1;min-width:0">
        <div class="big"><span id="pct">--</span><small>%</small></div>
        <div class="kv">
          <span>Distance</span><b id="dist">-- cm</b>
          <span>Empty at</span><b id="kvEmpty">-- cm</b>
          <span>Full at</span><b id="kvFull">-- cm</b>
        </div>
      </div>
    </div>
    <div class="alert bad" id="alertFault">Sensor fault: no echo. Check the sensor wiring and that the probe faces the water.</div>
    <div class="alert warn" id="alertLow">Low water level.</div>
  </section>

  <section class="card" id="cCal">
    <h2>CALIBRATION</h2>
    <div class="live"><span>Live sensor reading</span><b id="liveDist">-- cm</b></div>
    <div class="btns">
      <button class="btn" id="btnEmpty" onclick="calNow('empty')">Tank is EMPTY now</button>
      <button class="btn" id="btnFull" onclick="calNow('full')">Tank is FULL now</button>
    </div>
    <p class="help">Quick calibration: when the tank is empty (or full), press the matching button to save the live reading.
      Or type the distances from the sensor face to the water surface below.</p>
    <div class="row2" style="margin-top:14px">
      <div class="field"><label for="inEmpty">Empty distance (cm)</label><input type="number" id="inEmpty" step="0.5" min="20" max="450" inputmode="decimal"></div>
      <div class="field"><label for="inFull">Full distance (cm)</label><input type="number" id="inFull" step="0.5" min="15" max="440" inputmode="decimal"></div>
    </div>
    <button class="btn sec" onclick="calManual()">Save distances</button>
    <p class="help">Keep the sensor at least 20–25 cm above the full water line. It can't measure closer than that.</p>
  </section>

  <section class="card" id="cDisp">
    <h2>DISPLAY &amp; SENSOR SETTINGS</h2>
    <div class="field"><label for="inLeds">Number of LEDs on the strip</label><input type="number" id="inLeds" min="1" max="300" step="1" inputmode="numeric">
      <p class="help" id="ampHelp"></p></div>
    <div class="field"><label>LED brightness: <b id="brVal">--</b></label><input type="range" id="inBr" min="5" max="255" step="5"></div>
    <div class="field"><label>Low water alarm below: <b id="lowVal">--</b>%</label><input type="range" id="inLow" min="0" max="50" step="1"></div>
    <div class="toggle"><span>Strip mounted top-down (LED 1 at top)</span><label class="sw"><input type="checkbox" id="inRev"><i></i></label></div>
    <div class="toggle"><span>Whole bar one color (by level)</span><label class="sw"><input type="checkbox" id="inCbl"><i></i></label></div>
    <div class="field" style="margin-top:10px"><label for="inTrig">Sensor trigger pulse (µs)</label><input type="number" id="inTrig" min="10" max="1000" step="10" inputmode="numeric">
      <p class="help">The AJ-SR04M needs 50–500 µs. Default 100.</p></div>
    <div class="btns">
      <button class="btn" onclick="saveSettings()">Save settings</button>
      <button class="btn sec" onclick="resetDefaults()">Defaults</button>
    </div>
  </section>

  <section class="card" id="cWifi">
    <h2>WI-FI SETTINGS</h2>
    <div class="wgrid">
      <div>
        <div class="kv" style="margin-top:0">
          <span>Home network</span><b id="wSaved">--</b>
          <span>Status</span><b id="wState">--</b>
          <span>Signal</span><b><span id="rssiTxt">--</span><span class="rssi" id="rssiBars"><i style="height:4px"></i><i style="height:7px"></i><i style="height:10px"></i><i style="height:14px"></i></span></b>
          <span>Home address</span><b id="wIp">--</b>
          <span>Hotspot</span><b id="wAp">--</b>
        </div>
        <div class="alert" id="wMsg"></div>
        <p class="help" style="margin-top:12px">The hotspot <b>YUCCA TANK WATER LEVEL</b> turns on automatically whenever the device can't reach your home Wi-Fi, so you can always get back to this page at <b>http://192.168.4.1</b>.</p>
        <div class="toggle" style="margin-top:6px;border-top:1px solid var(--line)"><span>Keep hotspot always on<br><span class="help">Otherwise it turns off 30 s after home Wi-Fi connects.</span></span>
          <label class="sw"><input type="checkbox" id="inApAlways" onchange="setApAlways(this.checked)"><i></i></label></div>
      </div>
      <div>
        <div class="scanhead"><label>Available networks</label><button class="btn sec small" id="btnScan" onclick="scan()">Scan</button></div>
        <div class="netlist" id="netList"><p class="help">Press Scan to find networks.</p></div>
        <div class="field"><label for="inSsid">Network name (SSID)</label><input type="text" id="inSsid" maxlength="32" autocomplete="off" autocapitalize="none" autocorrect="off" spellcheck="false"></div>
        <div class="field"><label for="inPass">Password</label><div class="pw"><input type="password" id="inPass" maxlength="63" autocomplete="off" autocapitalize="none" autocorrect="off" spellcheck="false"><button type="button" class="eye" id="btnEye" onclick="togglePw()">Show</button></div></div>
        <div class="btns">
          <button class="btn" onclick="connectWifi()">Connect</button>
          <button class="btn danger" onclick="forgetWifi()">Forget Wi-Fi</button>
        </div>
        <p class="help">Leave the password empty for open networks.</p>
      </div>
    </div>
  </section>

  <section class="card" id="cDev">
    <h2>DEVICE</h2>
    <div class="kv" style="margin-top:0">
      <span>Board</span><b id="board">--</b>
      <span>Firmware</span><b id="fw">--</b>
      <span>Uptime</span><b id="uptime">--</b>
      <span>Last restart</span><b id="reset">--</b>
      <span>Boot count</span><b id="boot">--</b>
      <span>Free memory</span><b id="heap">--</b>
      <span id="vccL">Supply voltage</span><b id="vcc">--</b>
      <span>Log storage</span><b id="logStore">--</b>
    </div>
    <div style="margin-top:16px"><button class="btn sec" onclick="restartDev()">Restart device</button></div>
  </section>

  <section class="card" id="cLog">
    <h2>CONNECTIVITY LOG</h2>
    <div class="stats" id="logStats"></div>
    <div class="logbar">
      <select id="logFilter" onchange="renderLog()">
        <option value="all">All events</option><option value="net">Wi-Fi &amp; hotspot</option>
        <option value="warn">Warnings &amp; errors</option><option value="boot">Restarts</option>
      </select>
      <label class="chk"><input type="checkbox" id="logAuto" checked> Auto-refresh</label>
      <div class="logbtns">
        <button class="btn sec small" onclick="loadLog()">Refresh</button>
        <button class="btn sec small" onclick="downloadLog()">Download</button>
        <button class="btn danger small" onclick="clearLog()">Clear</button>
      </div>
    </div>
    <div class="logbox" id="logBox"><p class="help">Loading…</p></div>
    <p class="help">Newest first. Saved in flash, so it survives restarts and power cuts (about 300–400 events).
      Events from before the clock was known show as boot number + time since that start.</p>
  </section>
</main>
<footer>YUCCA Tank Water Level · <span id="host"></span></footer>
<div class="toast" id="toast"></div>
<div class="modal" id="modal"><div class="card"><p id="modalMsg"></p>
  <div class="btns" style="margin:0"><button class="btn sec" id="modalNo">Cancel</button><button class="btn" id="modalYes">OK</button></div></div></div>

<script>
const $=id=>document.getElementById(id);
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
let N=0,st=null,formLoaded=false,online=false,timeSent=false,logRows=[];
function buildLeds(n){if(n===N)return;N=n;const box=$('leds');box.innerHTML='';
  box.style.gap=(n<=40?3:n<=80?2:n<=120?1:0)+'px';
  for(let i=0;i<n;i++){const d=document.createElement('div');d.className='led';box.appendChild(d);}}
const posT=i=>N>1?i/(N-1):1;

function ledColor(t){t=Math.min(1,Math.max(0,t));let r,g;
  if(t<.5){r=255;g=Math.round(t*510);}else{r=Math.round((1-t)*510);g=255;}return `rgb(${r},${g},0)`;}
function toast(msg,err){const t=$('toast');t.textContent=msg;t.className='toast show'+(err?' err':'');
  clearTimeout(t._h);t._h=setTimeout(()=>t.className='toast'+(err?' err':''),3500);}
function ask(msg){return new Promise(res=>{$('modalMsg').textContent=msg;$('modal').classList.add('show');
  const done=v=>{$('modal').classList.remove('show');res(v);};$('modalYes').onclick=()=>done(true);$('modalNo').onclick=()=>done(false);});}
function fmtUp(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
  return (d?d+'d ':'')+(d||h?h+'h ':'')+m+'m';}
function bars(rssi){return rssi>-55?4:rssi>-65?3:rssi>-75?2:1;}
function setAlert(el,cls,html){el.className='alert'+(cls?' show '+cls:'');if(cls)el.innerHTML=html;}
function esc(s){const d=document.createElement('div');d.textContent=s;return d.innerHTML;}

function render(){
  const s=st,w=s.wifi,lvl=s.valid?s.level:0;
  $('pct').textContent=s.valid?Math.round(s.level):'--';
  $('dist').textContent=s.valid?s.distance.toFixed(1)+' cm':'no echo';
  $('liveDist').textContent=s.valid?s.distance.toFixed(1)+' cm':'no echo';
  $('kvEmpty').textContent=s.empty.toFixed(1)+' cm';$('kvFull').textContent=s.full.toFixed(1)+' cm';
  const wt=$('water');wt.style.height=(s.valid?lvl:0)+'%';wt.style.backgroundColor=ledColor(lvl/100);
  buildLeds(s.leds);
  const lit=lvl/100*N,leds=$('leds').children;
  for(let i=0;i<N;i++){const on=s.valid&&i<Math.round(lit);
    leds[i].style.backgroundColor=on?ledColor(s.colorByLevel?lvl/100:posT(i)):'';}
  const low=s.valid&&lvl<s.lowAlarm;
  $('alertFault').classList.toggle('show',!s.valid);$('alertLow').classList.toggle('show',low);
  $('chip').className='chip'+(!s.valid?' bad':low?' warn':'');
  $('chipText').textContent=!s.valid?'Sensor fault':low?'Low water':'Online';
  $('btnEmpty').disabled=$('btnFull').disabled=!s.valid;

  // Wi-Fi
  const conn=w.state==='connected';
  $('setupBanner').classList.toggle('show',!w.saved&&w.state!=='connecting'&&!conn);
  $('wSaved').textContent=w.saved||'Not set';
  $('wState').textContent={connected:'Connected',connecting:'Connecting…',failed:'Not reachable',none:'Not set up'}[w.state];
  $('rssiTxt').textContent=conn?w.rssi+' dBm':'--';
  [...$('rssiBars').children].forEach((b,i)=>b.classList.toggle('on',conn&&i<bars(w.rssi)));
  $('wIp').textContent=conn?w.ip:'--';
  $('wAp').textContent=w.ap?`On · ${w.apClients} device${w.apClients==1?'':'s'}`:'Off';
  const m=$('wMsg');
  if(w.state==='connecting')setAlert(m,'info',esc(w.msg));
  else if(conn&&w.ap&&w.apOffIn>0)setAlert(m,'ok',`Connected to <b>${esc(w.ssid)}</b>. Switch your phone or PC back to your home Wi-Fi and open
    <a href="http://${w.host}.local">http://${w.host}.local</a> or <a href="http://${w.ip}">http://${w.ip}</a>.
    The hotspot turns off in ${w.apOffIn} s.`);
  else if(w.state==='failed')setAlert(m,'bad',esc(w.msg));
  else if(w.state==='none')setAlert(m,'info',esc(w.msg));
  else setAlert(m,'','');

  $('board').textContent=s.board;$('fw').textContent=s.fw;$('uptime').textContent=fmtUp(s.uptime);$('reset').textContent=s.reset;
  $('boot').textContent='#'+s.boot;$('heap').textContent=(s.heap/1024).toFixed(1)+' KB (lowest '+(s.minHeap/1024).toFixed(1)+' KB)';
  $('vcc').textContent=s.vcc?(s.vcc/1000).toFixed(2)+' V':'';$('vcc').style.display=$('vccL').style.display=s.vcc?'':'none';
  $('logStore').textContent=s.logStore==='flash'?'Flash (kept after restart)':'Memory only (lost on restart)';
  $('host').textContent=conn?`${w.host}.local · ${w.ip}`:`hotspot · ${w.apIp}`;
  if(!formLoaded)fillForm();
}

function fillForm(){const s=st;formLoaded=true;
  $('inEmpty').value=s.empty;$('inFull').value=s.full;
  $('inLeds').value=s.leds;$('inBr').value=s.brightness;$('brVal').textContent=s.brightness;
  $('inLow').value=s.lowAlarm;$('lowVal').textContent=s.lowAlarm;
  $('inRev').checked=s.reversed;$('inCbl').checked=s.colorByLevel;$('inTrig').value=s.trigUs;updateAmps();
  if(!$('inSsid').value&&s.wifi.saved)$('inSsid').value=s.wifi.saved;
  $('inApAlways').checked=s.apAlways;}
// Worst case current: full tank, ~20 mA per lit color channel at full brightness, plus ~0.2 A for the board
function updateAmps(){
  const n=Math.max(1,Math.min(300,parseInt($('inLeds').value)||1)),br=$('inBr').value/255,one=$('inCbl').checked;
  let mA=0;for(let i=0;i<n;i++){const t=one?1:(n>1?i/(n-1):1);const r=t<.5?1:(1-t)*2,g=t<.5?t*2:1;mA+=(r+g)*20*br;}
  const a=mA/1000+0.2,psu=Math.max(1,Math.ceil(a*1.25));
  $('ampHelp').textContent=`Up to about ${a.toFixed(1)} A at this brightness. Use a 5V supply of at least ${psu} A.`;}
$('inLeds').oninput=updateAmps;$('inCbl').onchange=updateAmps;
$('inBr').oninput=e=>{$('brVal').textContent=e.target.value;updateAmps();};
$('inLow').oninput=e=>$('lowVal').textContent=e.target.value;

async function poll(){
  try{const r=await fetch('/api/status',{cache:'no-store'});st=await r.json();online=true;render();
    if(!st.epoch&&!timeSent){timeSent=true;fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+Math.floor(Date.now()/1000)}).catch(()=>{});}}
  catch(e){if(online)toast('Reconnecting to device…',true);online=false;
    $('chip').className='chip bad';$('chipText').textContent='Offline';}
}
async function post(url,data){
  try{const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:new URLSearchParams(data||{})});const j=await r.json();toast(j.message,!j.ok);return j;}
  catch(e){toast('Request failed',true);return {ok:false};}
}

async function calNow(point){
  if(!await ask(`Save the live reading (${st.distance.toFixed(1)} cm) as ${point==='empty'?'EMPTY':'FULL'}?`))return;
  const j=await post('/api/calibrate',{point});if(j.ok){formLoaded=false;poll();}}
async function calManual(){const j=await post('/api/calibrate',{empty:$('inEmpty').value,full:$('inFull').value});
  if(j.ok){formLoaded=false;poll();}}
async function saveSettings(){
  const n=parseInt($('inLeds').value);
  if(!(n>=1&&n<=300)){toast('Number of LEDs must be 1–300',true);return;}
  const j=await post('/api/settings',{leds:n,brightness:$('inBr').value,lowAlarm:$('inLow').value,
    reversed:$('inRev').checked?1:0,colorByLevel:$('inCbl').checked?1:0,trigUs:$('inTrig').value});
  if(j.ok){formLoaded=false;poll();}}
async function resetDefaults(){if(!await ask('Reset calibration and display settings to defaults? The number of LEDs and Wi-Fi are not changed.'))return;
  const j=await post('/api/settings',{defaults:1});if(j.ok){formLoaded=false;poll();}}
async function restartDev(){if(await ask('Restart the device?'))await post('/api/restart');}

// ---- Wi-Fi ----
async function scan(){
  const b=$('btnScan');b.disabled=true;b.textContent='Scanning…';
  $('netList').innerHTML='<p class="help">Scanning for networks…</p>';
  try{
    await fetch('/api/scan?start=1',{cache:'no-store'});
    for(let i=0;i<20;i++){await sleep(1000);
      const j=await (await fetch('/api/scan',{cache:'no-store'})).json();
      if(!j.running){renderNets(j.networks||[]);break;}}
  }catch(e){$('netList').innerHTML='<p class="help">Scan failed. Try again.</p>';}
  b.disabled=false;b.textContent='Scan';
}
function renderNets(list){
  const box=$('netList');box.innerHTML='';
  if(!list.length){box.innerHTML='<p class="help">No networks found. Try again, or type the name below.</p>';return;}
  list.forEach(n=>{const b=document.createElement('button');b.type='button';b.className='net';
    const name=document.createElement('span');name.textContent=n.ssid;
    const info=document.createElement('small');info.textContent=(n.open?'Open':'🔒')+'  '+'▂▄▆█'.slice(0,bars(n.rssi));
    b.append(name,info);
    b.onclick=()=>{[...box.children].forEach(c=>c.classList.remove('sel'));b.classList.add('sel');
      $('inSsid').value=n.ssid;$('inPass').value='';if(!n.open)$('inPass').focus();};
    box.appendChild(b);});
}
function togglePw(){const p=$('inPass'),show=p.type==='password';p.type=show?'text':'password';$('btnEye').textContent=show?'Hide':'Show';}
async function connectWifi(){
  const ssid=$('inSsid').value.trim(),pass=$('inPass').value;
  if(!ssid){toast('Choose or type a network name',true);return;}
  if(pass&&(pass.length<8||pass.length>63)){toast('Password must be 8–63 characters',true);return;}
  const j=await post('/api/wifi/connect',{ssid,pass});if(j.ok)$('inPass').value='';
}
async function forgetWifi(){
  if(!await ask('Forget the saved home Wi-Fi? The device keeps working and the hotspot stays on.'))return;
  await post('/api/wifi/forget');$('inSsid').value='';
}

async function setApAlways(on){const j=await post('/api/settings',{apAlways:on?1:0});if(!j.ok)$('inApAlways').checked=!on;}

// ---- Connectivity log ----
const pad=n=>String(n).padStart(2,'0');
function upStr(s){return Math.floor(s/3600)+':'+pad(Math.floor(s%3600/60))+':'+pad(s%60);}
function entryTime(e){
  let ep=e.epoch;
  if(!ep&&st&&st.epoch&&e.boot===st.boot)ep=st.epoch-(st.uptime-e.up);
  return ep?new Date(ep*1000):null;}
function fmtTime(e){const d=entryTime(e);
  return d?d.toLocaleString([], {day:'2-digit',month:'short',hour:'2-digit',minute:'2-digit',second:'2-digit',hour12:false})
          :`Boot #${e.boot} +${upStr(e.up)}`;}
function parseLog(txt){return txt.split('\n').map(l=>l.split('\t')).filter(f=>f.length>=6)
  .map(f=>({boot:+f[0],up:+f[1],epoch:+f[2],lvl:f[3],cat:f[4],msg:f.slice(5).join(' ')}));}
async function loadLog(){
  try{const r=await fetch('/api/log',{cache:'no-store'});logRows=parseLog(await r.text());renderLog();}
  catch(e){$('logBox').innerHTML='<p class="help">Could not load the log.</p>';}}
function renderLog(){
  const f=$('logFilter').value,box=$('logBox');
  const count=(fn)=>logRows.filter(fn).length;
  const stats=[
    ['Restarts',count(e=>e.cat==='boot'&&e.msg.startsWith('Boot #')),''],
    ['Crashes / power dips',count(e=>e.cat==='boot'&&(e.lvl==='E'||e.msg.includes('Power on'))),'bad'],
    ['Hotspot turned off',count(e=>e.msg.startsWith('Hotspot off')),''],
    ['Hotspot channel changes',count(e=>e.msg.startsWith('Hotspot channel')),'warn'],
    ['Home Wi-Fi drops',count(e=>e.msg.startsWith('Home Wi-Fi lost')),'warn'],
    ['Warnings',count(e=>e.lvl==='W'),'warn']];
  $('logStats').innerHTML=stats.map(([k,v,c])=>`<div class="stat ${v?c:''}"><b>${v}</b>${k}</div>`).join('');
  const rows=logRows.filter(e=>f==='all'||(f==='net'&&(e.cat==='wifi'||e.cat==='ap'))||
    (f==='warn'&&e.lvl!=='I')||(f==='boot'&&e.cat==='boot')).slice().reverse();
  box.innerHTML='';
  if(!rows.length){box.innerHTML='<p class="help">No events yet.</p>';return;}
  const frag=document.createDocumentFragment();
  rows.forEach(e=>{const r=document.createElement('div');r.className='lr '+e.lvl+(e.cat==='boot'?' boot':'');
    const t=document.createElement('time');t.textContent=fmtTime(e);
    const g=document.createElement('span');g.className='tag';g.textContent=e.cat;
    const m=document.createElement('span');m.className='msg';m.textContent=e.msg;
    r.append(t,g,m);frag.appendChild(r);});
  box.appendChild(frag);}
function downloadLog(){
  const lines=logRows.map(e=>{const d=entryTime(e);
    const t=d?`${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`
             :`boot#${e.boot} +${upStr(e.up)}`;
    return `${t}\t${e.lvl}\t${e.cat}\t${e.msg}`;});
  const head=st?`YUCCA Tank Water Level log · firmware ${st.fw} · ${st.board} · downloaded ${new Date().toString()}\n\n`:'';
  const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([head+lines.join('\n')+'\n'],{type:'text/plain'}));
  const n=new Date();a.download=`yucca-tank-log-${n.getFullYear()}${pad(n.getMonth()+1)}${pad(n.getDate())}-${pad(n.getHours())}${pad(n.getMinutes())}.txt`;
  document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},500);}
async function clearLog(){if(!await ask('Clear the connectivity log? This cannot be undone.'))return;
  await post('/api/log/clear');loadLog();}

poll();setInterval(()=>{if(!document.hidden)poll();},1000);
loadLog();setInterval(()=>{if(!document.hidden&&$('logAuto').checked)loadLog();},5000);
</script>
</body>
</html>
)rawliteral";
