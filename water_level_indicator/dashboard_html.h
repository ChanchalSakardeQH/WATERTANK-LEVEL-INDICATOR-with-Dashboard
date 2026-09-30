/*
  Water Tanks Monitor System - dashboard web page (HTML, CSS, JavaScript), served from flash
  woodyouloveit.com
  Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the license below.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  SPDX-License-Identifier: GPL-3.0-or-later
*/
// Dashboard web page, served from flash. Works offline (no external files).
#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<!--
  Water Tanks Monitor System dashboard - woodyouloveit.com
  Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 (or later).
  Source: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard
-->
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0f766e">
<title>Water Tanks Monitor System | woodyouloveit.com</title>
<meta name="author" content="Chanchal Sakarde, woodyouloveit.com">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23e7004e' d='M16 29 3.5 16.5A7.5 7.5 0 0 1 16 6.2 7.5 7.5 0 0 1 28.5 16.5Z'/%3E%3C/svg%3E">
<style>
:root{--bg:#f1f5f9;--card:#fff;--text:#0f172a;--muted:#64748b;--line:#e2e8f0;--accent:#0f766e;--accent2:#14b8a6;
--danger:#dc2626;--warn:#d97706;--ok:#16a34a;--input:#f8fafc;--shadow:0 1px 3px rgba(15,23,42,.08),0 4px 16px rgba(15,23,42,.05)}
@media (prefers-color-scheme:dark){:root{--bg:#0b1220;--card:#131c2e;--text:#e2e8f0;--muted:#94a3b8;--line:#243047;
--input:#0f1726;--shadow:0 1px 3px rgba(0,0,0,.4)}}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;-webkit-text-size-adjust:100%}
body{padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left)}
/* brand */
.brandbar{display:flex;align-items:center;justify-content:space-between;gap:12px;background:#fff;padding:10px 20px;border-bottom:1px solid #e2e8f0}
.brandbar img{height:30px;width:auto;display:block}
.brandbar .site{color:#0f172a;font-weight:700;font-size:13px;text-decoration:none;white-space:nowrap}
.brandbar .site:hover{color:#e7004e}
@media (max-width:380px){.brandbar img{height:24px}.brandbar .site{font-size:12px}}
footer{line-height:1.8}footer a{color:inherit;font-weight:600}
.copy{font-weight:600;color:var(--text)}
.pdfrow{display:flex;flex-wrap:wrap;align-items:center;gap:10px 16px;margin-top:14px}
.pdfrow .btn{width:auto;padding:12px 20px}.pdfrow .help{margin:0;flex:1;min-width:200px}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:16px 20px;background:var(--accent);color:#fff}
header h1{font-size:18px;margin:0;font-weight:600;line-height:1.2}
header small{display:block;opacity:.8;font-weight:400;font-size:12px}
.chip{display:inline-flex;align-items:center;gap:6px;padding:5px 12px;border-radius:999px;font-size:13px;font-weight:600;background:rgba(255,255,255,.18);white-space:nowrap}
.chip i{width:8px;height:8px;border-radius:50%;background:#a7f3d0}
.chip.bad i{background:#fca5a5}.chip.warn i{background:#fcd34d}
main{max-width:1100px;margin:0 auto;padding:16px;display:grid;gap:16px;grid-template-columns:1fr}
@media (min-width:860px){main{padding:24px}#tabOverview{grid-template-columns:380px 1fr;grid-template-areas:'ban ban' 'level glance';align-items:start}
#tabAnalytics{grid-template-columns:1fr 1fr;grid-template-areas:'akpi akpi' 'achart achart' 'amotor ause' 'ahour anight' 'aset aset';align-items:start}
#aKpi{grid-area:akpi}#aChart{grid-area:achart}#aMotor{grid-area:amotor}#aUse{grid-area:ause}#aHour{grid-area:ahour}#aNight{grid-area:anight}#aSet{grid-area:aset}
#banners{grid-area:ban}
#tabOverview:has(#banners[hidden]){grid-template-areas:'level glance'}#cLevel{grid-area:level}#cGlance{grid-area:glance}
#tabSetup{grid-template-columns:1fr 1fr;grid-template-areas:'prof cal' 'site cal' 'site disp' 'power disp' 'power dev' 'wifi wifi';align-items:start}
#cProfile{grid-area:prof}#cPower{grid-area:power}#cCal{grid-area:cal}#cSite{grid-area:site}#cDisp{grid-area:disp}#cWifi{grid-area:wifi}#cDev{grid-area:dev}}
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
.row3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}.row3[hidden],.row2[hidden]{display:none}
.sel{width:100%;padding:11px 10px;font-size:16px;border:1px solid var(--line);border-radius:10px;background:var(--input);color:var(--text)}
h3.sub{font-size:14px;margin:20px 0 6px;color:var(--text)}
.slide{display:flex;align-items:center;gap:14px;margin:10px 0 6px}.slide input{flex:1;height:32px}.slide b{min-width:96px;text-align:right;font-size:18px}
.calprev{font-size:13px;line-height:1.5;padding:10px 12px;border-radius:10px;background:var(--input);margin:6px 0 10px}
.calprev.bad{background:rgba(220,38,38,.12);color:var(--danger)}.calprev.warn{background:rgba(217,119,6,.12);color:var(--warn)}
.ro{padding:11px 12px;border:1px dashed var(--line);border-radius:10px;font-size:16px}
.help a,.card p a{color:var(--accent);font-weight:600}
.litres{font-size:15px;color:var(--muted);margin:-2px 0 8px;font-weight:600}
.gstats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:14px}.gstats .stat b{font-size:20px}
.seg[id=segUsage]{margin-bottom:0}
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
.tabs{display:flex;gap:4px;background:var(--accent);padding:0 12px;overflow-x:auto}
.tabs button{appearance:none;border:none;background:transparent;color:rgba(255,255,255,.75);font-size:15px;font-weight:600;padding:10px 14px 12px;cursor:pointer;border-bottom:3px solid transparent;white-space:nowrap}
.tabs button.on{color:#fff;border-bottom-color:#fff}
.tab[hidden]{display:none!important}#banners[hidden]{display:none!important}
.kpis{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px}
.kpi{background:var(--input);border:1px solid var(--line);border-radius:12px;padding:12px}
.kpi span{display:block;font-size:12px;color:var(--muted);font-weight:600;letter-spacing:.02em}
.kpi b{display:block;font-size:20px;margin:4px 0 2px}.kpi small{font-size:12px;color:var(--muted)}
.kpi.bad b{color:var(--danger)}.kpi.warn b{color:var(--warn)}.kpi.ok b{color:var(--ok)}.kpi.live b{color:var(--accent)}
.chart{width:100%;min-height:60px;position:relative}
.chart svg{display:block;width:100%;height:auto;overflow:visible}
.chart .grid{stroke:var(--line);stroke-width:1}.chart .axis{fill:var(--muted);font-size:11px}
.chart .tip{position:absolute;top:0;pointer-events:none;background:var(--text);color:var(--card);font-size:12px;padding:4px 8px;border-radius:6px;white-space:nowrap;display:none;transform:translateX(-50%)}
.seg{display:inline-flex;border:1px solid var(--line);border-radius:10px;overflow:hidden;margin-bottom:10px}
.seg button{border:none;background:var(--input);color:var(--text);padding:8px 14px;font-size:14px;cursor:pointer}
.seg button.on{background:var(--accent);color:#fff}
.legend{display:flex;flex-wrap:wrap;gap:14px;font-size:12px;color:var(--muted);margin-top:6px}
.legend i{display:inline-block;width:12px;height:12px;border-radius:3px;vertical-align:-2px;margin-right:5px}
table.nt{width:100%;border-collapse:collapse;font-size:14px}
.nt th{text-align:left;color:var(--muted);font-weight:600;font-size:12px;padding:6px 4px;border-bottom:1px solid var(--line)}
.nt td{padding:8px 4px;border-bottom:1px solid var(--line)}.nt td:last-child{text-align:right}
.pill{display:inline-block;padding:2px 10px;border-radius:999px;font-size:12px;font-weight:700}
.pill.ok{background:rgba(22,163,74,.14);color:var(--ok)}.pill.warn{background:rgba(217,119,6,.14);color:var(--warn)}
.pill.bad{background:rgba(220,38,38,.14);color:var(--danger)}.pill.na{background:var(--input);color:var(--muted)}
.fl{font-size:13px;margin-top:12px;border-top:1px solid var(--line)}.fl div{padding:7px 0;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;gap:10px}
.fl div span:last-child{color:var(--muted);text-align:right}
select.inp{width:100%;padding:11px 12px;font-size:16px;border:1px solid var(--line);border-radius:10px;background:var(--input);color:var(--text)}
footer{text-align:center;color:var(--muted);font-size:12px;padding:8px 0 24px}
</style>
</head>
<body>
<div class="brandbar">
  <a href="https://woodyouloveit.com" target="_blank" rel="noopener"><img id="brandLogo" alt="woodyouloveit.com" src="data:image/png;base64,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"></a>
  <a class="site" href="https://woodyouloveit.com" target="_blank" rel="noopener">woodyouloveit.com</a>
</div>
<header>
  <h1><span id="hTitle">Water Tanks Monitor System</span><small id="hSub">woodyouloveit.com</small></h1>
  <div class="chip" id="chip"><i></i><span id="chipText">Connecting…</span></div>
</header>
<nav class="tabs" id="tabs">
  <button data-tab="overview" class="on">Overview</button>
  <button data-tab="analytics">Analytics</button>
  <button data-tab="setup">Setup</button>
  <button data-tab="log">Log</button>
</nav>

<main id="tabOverview" class="tab">
  <div id="banners" style="display:grid;gap:12px">
  <div class="banner" id="siteBanner">Name this tank: add the society or organisation, building and tank in <a href="#cSite">Setup → Site details</a>. The names appear on the dashboard and in reports.</div>
  <div class="banner" id="setupBanner">Not connected to your home Wi-Fi yet. <a href="#cWifi">Set up Wi-Fi</a> in Setup to use the dashboard from your home network.</div>
  </div>

  <section class="card" id="cLevel">
    <h2>WATER LEVEL</h2>
    <div class="hero">
      <div class="tank"><div class="water" id="water"></div></div>
      <div class="leds" id="leds" title="LED strip preview"></div>
      <div style="flex:1;min-width:0">
        <div class="big"><span id="pct">--</span><small>%</small></div>
        <div class="litres" id="litres"></div>
        <div class="kv">
          <span>Distance</span><b id="dist">-- cm</b>
          <span>Empty at</span><b id="kvEmpty">-- cm</b>
          <span>Full at</span><b id="kvFull">-- cm</b>
        </div>
      </div>
    </div>
    <div class="alert bad" id="alertFault">Sensor fault: no echo. Check the sensor wiring and that the probe faces the water.</div>
    <div class="alert warn" id="alertLow">Low water level.</div>
    <div class="alert info" id="alertFill"></div>
    <div class="alert bad" id="alertLeak"></div>
  </section>

  <section class="card" id="cGlance">
    <h2>TANK AT A GLANCE</h2>
    <div class="gstats">
      <div class="stat"><b id="gWater">--</b>water now</div>
      <div class="stat"><b id="gFree">--</b>space left</div>
      <div class="stat"><b id="gCap">--</b>capacity</div>
    </div>
    <div class="kv" style="margin-top:0">
      <span>Site</span><b id="gSite">--</b>
      <span>Usage</span><b id="gUsage">--</b>
      <span>Tank</span><b id="gTank">--</b>
      <span>Water depth when full</span><b id="gDepth">--</b>
      <span>Calibrated range</span><b id="gRange">--</b>
      <span>Sensor to full water line</span><b id="gGap">--</b>
      <span>1 cm of water</span><b id="gRes">--</b>
    </div>
    <div class="alert warn" id="gWarn"></div>
    <p class="help" style="margin-top:12px"><a href="#setup">Change tank, usage and calibration in Setup</a></p>
  </section>
</main>

<main id="tabSetup" class="tab" hidden>
  <section class="card" id="cProfile">
    <h2>USAGE &amp; TANK</h2>
    <div class="field"><label>Usage</label>
      <div class="seg" id="segUsage"><button data-v="0" class="on">Domestic</button><button data-v="1">Commercial</button></div></div>
    <div class="row2">
      <div class="field"><label for="inLoc">Tank location</label><select id="inLoc" class="sel">
        <option value="0">Overhead (roof)</option><option value="1">Loft / bathroom</option>
        <option value="2">Underground sump</option><option value="3">Other</option></select></div>
      <div class="field"><label for="inShape">Tank shape</label><select id="inShape" class="sel">
        <option value="0">Rectangular (box, loft)</option><option value="1">Vertical cylinder</option></select></div>
    </div>
    <div class="field"><label for="inPreset">Tank size</label><select id="inPreset" class="sel"></select></div>
    <div class="row3" id="dimRect">
      <div class="field"><label for="inLen">Length (mm)</label><input type="number" id="inLen" min="0" max="20000" inputmode="numeric"></div>
      <div class="field"><label for="inWid">Width (mm)</label><input type="number" id="inWid" min="0" max="20000" inputmode="numeric"></div>
      <div class="field"><label for="inHgt">Height (mm)</label><input type="number" id="inHgt" min="0" max="20000" inputmode="numeric"></div>
    </div>
    <div class="row2" id="dimCyl" hidden>
      <div class="field"><label for="inDia">Diameter (mm)</label><input type="number" id="inDia" min="0" max="20000" inputmode="numeric"></div>
      <div class="field"><label for="inHgtC">Height (mm)</label><input type="number" id="inHgtC" min="0" max="20000" inputmode="numeric"></div>
    </div>
    <div class="row2">
      <div class="field"><label for="inDepth">Water depth when full (mm)</label><input type="number" id="inDepth" min="0" max="20000" inputmode="numeric">
        <p class="help">Tank bottom to the overflow pipe: the highest the water gets.</p></div>
      <div class="field"><label for="inCapP">Capacity (litres)</label><input type="number" id="inCapP" min="0" max="1000000" inputmode="numeric">
        <p class="help" id="capHint"></p></div>
    </div>
    <div class="alert info" id="profTip"></div>
    <div class="btns" style="margin-top:14px">
      <button class="btn" onclick="saveProfile()">Save tank</button>
      <button class="btn sec" onclick="applyRecommended()">Recommended settings</button>
    </div>
    <p class="help" id="recHelp"></p>
  </section>

  <section class="card" id="cCal">
    <h2>CALIBRATION</h2>
    <div class="live"><span>Live sensor reading</span><b id="liveDist">-- cm</b></div>

    <h3 class="sub">1 · How full is the tank right now?</h3>
    <p class="help">Slide to the current water level. With the water depth from <a href="#cProfile">Usage &amp; tank</a>,
      the empty and full points are worked out from the live reading. No need to empty or fill the tank.</p>
    <div class="slide"><input type="range" id="inNow" min="0" max="100" step="1" value="50"><b id="nowVal">50%</b></div>
    <div class="calprev" id="prevSlider"></div>
    <button class="btn" id="btnSlider" onclick="calSlider()">Save calibration from current level</button>

    <h3 class="sub">2 · From measurements</h3>
    <div class="row2">
      <div class="field"><label for="inGap">Sensor face to full water line (cm)</label><input type="number" id="inGap" step="0.5" min="15" max="400" inputmode="decimal"></div>
      <div class="field"><label>Water depth when full</label><div class="ro" id="depthShow">--</div></div>
    </div>
    <div class="calprev" id="prevMeas"></div>
    <button class="btn sec" id="btnMeas" onclick="calMeasured()">Save calibration from measurements</button>

    <h3 class="sub">3 · Tank is empty or full right now</h3>
    <div class="btns">
      <button class="btn sec" id="btnEmpty" onclick="calNow('empty')">Tank is EMPTY now</button>
      <button class="btn sec" id="btnFull" onclick="calNow('full')">Tank is FULL now</button>
    </div>
    <div class="row2" style="margin-top:4px">
      <div class="field"><label for="inEmpty">Empty distance (cm)</label><input type="number" id="inEmpty" step="0.5" min="20" max="450" inputmode="decimal"></div>
      <div class="field"><label for="inFull">Full distance (cm)</label><input type="number" id="inFull" step="0.5" min="15" max="440" inputmode="decimal"></div>
    </div>
    <button class="btn sec" onclick="calManual()">Save distances</button>
    <p class="help">Now: empty at <b id="calNowE">--</b>, full at <b id="calNowF">--</b> from the sensor.
      The sensor can't measure closer than about 20–25 cm, so the full water line must be at least that far below it.</p>
  </section>

  <section class="card" id="cSite">
    <h2>SITE DETAILS</h2>
    <div class="field"><label for="inOrg">Society / organisation</label><input type="text" id="inOrg" maxlength="48" placeholder="e.g. NYATI" autocomplete="organization"></div>
    <div class="row2">
      <div class="field"><label for="inBld">Building</label><input type="text" id="inBld" maxlength="32" placeholder="e.g. Tower A"></div>
      <div class="field"><label for="inTank">Tank</label><input type="text" id="inTank" maxlength="32" placeholder="e.g. Overhead Tank 1"></div>
    </div>
    <div class="kv" style="margin-top:0">
      <span>Hotspot name</span><b id="sAp">--</b>
      <span>Web address</span><b id="sHost">--</b>
    </div>
    <div class="alert info" id="sRestart"></div>
    <div class="btns" style="margin-top:14px">
      <button class="btn" onclick="saveSite()">Save site details</button>
      <button class="btn sec" id="btnSiteRestart" onclick="restartDev()">Restart device</button>
    </div>
    <p class="help">Used on this dashboard, in PDF reports and downloaded logs. The hotspot name and web address are made from the building and tank
      names, so every tank in the society gets its own. They change after a restart.</p>
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

  <section class="card" id="cPower">
    <h2>POWER &amp; RADIO</h2>
    <div class="toggle" style="border-top:none"><span>Power saving (production mode)<br><span class="help">CPU at 80 MHz (ESP32), sensor read twice a second,
      LED strip refreshed only when it changes, Wi-Fi power saving while the hotspot is off. Recommended for installed devices.</span></span>
      <label class="sw"><input type="checkbox" id="inEco"><i></i></label></div>
    <div class="field" style="margin-top:10px"><label for="inApMode">Hotspot</label>
      <select id="inApMode" class="sel"><option value="0">Automatic: on when home Wi-Fi is unavailable</option>
        <option value="2">On demand: press BOOT to turn on, off after 10 min idle</option><option value="1">Always on</option></select>
      <p class="help" id="apModeHelp"></p></div>
    <div class="field"><label>Wi-Fi transmit power</label>
      <div class="seg" id="segTx" style="margin-bottom:4px"><button data-v="2">Low</button><button data-v="0">Medium</button><button data-v="1">High</button></div>
      <p class="help">Lower power means less heat and less interference. Use High only if the signal to the router or phone is weak.</p></div>
    <div class="kv" style="margin-top:0">
      <span>Mode now</span><b id="pMode">--</b>
      <span>CPU</span><b id="pCpu">--</b>
      <span>Transmit power</span><b id="pTx">--</b>
      <span>Sensor reads</span><b id="pRate">--</b>
    </div>
    <div style="margin-top:14px"><button class="btn" onclick="savePower()">Save power settings</button></div>
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
        <p class="help" style="margin-top:12px">The hotspot <b class="apName">--</b> lets you reach this page at <b>http://192.168.4.1</b> when the device isn't on your home Wi-Fi.
          If it's off, press the <b>BOOT</b> button on the board to turn it on.</p>
        <p class="help">Hotspot mode: <b id="wApMode">--</b>. <a href="#cPower">Change in Power &amp; radio</a>.</p>
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
</main>

<main id="tabAnalytics" class="tab" hidden>
  <section class="card" id="aKpi">
    <h2>TANK ANALYTICS</h2>
    <div class="alert info" id="aMsg"></div>
    <div class="kpis" id="kpis"></div>
    <div class="pdfrow">
      <button class="btn" id="btnPdf" onclick="exportPdf()">Download PDF report</button>
      <p class="help">Last 7 days with charts, fills and night checks, branded with the woodyouloveit.com logo.
        On the hotspot's pop-up page, open the dashboard in your normal browser first so the download is allowed.</p>
    </div>
  </section>
  <section class="card" id="aChart">
    <h2>LEVEL HISTORY</h2>
    <div class="seg" id="rangeSeg"><button data-h="24" class="on">24 h</button><button data-h="72">3 days</button><button data-h="168">7 days</button><button data-h="336">14 days</button></div>
    <div class="chart" id="levelChart"></div>
    <div class="legend"><span><i style="background:rgba(22,163,74,.22)"></i>Filling (motor on)</span><span><i style="background:rgba(100,116,139,.16)"></i>Night check window</span></div>
  </section>
  <section class="card" id="aMotor">
    <h2>MOTOR RUN TIME PER DAY</h2>
    <div class="chart" id="motorChart"></div>
    <div class="fl" id="fillList"></div>
  </section>
  <section class="card" id="aUse">
    <h2>WATER USED PER DAY</h2>
    <div class="chart" id="useChart"></div>
    <p class="help">Counted from level drops outside fills. Small changes below the sensor noise are ignored.</p>
  </section>
  <section class="card" id="aHour">
    <h2>USAGE BY HOUR OF DAY</h2>
    <div class="chart" id="hourChart"></div>
    <p class="help" id="hourHelp">Average per day. The busiest hours are highlighted.</p>
  </section>
  <section class="card" id="aNight">
    <h2>NIGHT LEAK CHECK</h2>
    <div id="nightTable"></div>
    <p class="help" id="nightHelp"></p>
  </section>
  <section class="card" id="aSet">
    <h2>ANALYTICS SETTINGS</h2>
    <div class="row2">
      <div class="field"><label for="inCap">Tank capacity (litres)</label><input type="number" id="inCap" min="0" max="1000000" step="50" inputmode="numeric" placeholder="e.g. 1000">
        <p class="help">Optional. Shows amounts in litres (assumes straight tank walls). 0 = show %.</p></div>
      <div class="field"><label for="inFillCm">Fill detection: minimum rise (cm/min)</label><input type="number" id="inFillCm" min="0.1" max="20" step="0.1" inputmode="decimal">
        <p class="help">Lower it if slow fills are missed, raise it if you see false fills. Default 0.5.</p></div>
    </div>
    <div class="row2">
      <div class="field"><label for="inNs">Night check from</label><select class="inp" id="inNs"></select></div>
      <div class="field"><label for="inNe">to</label><select class="inp" id="inNe"></select></div>
    </div>
    <div class="field"><label for="inLeak">Possible leak if the level drops more than (cm) during the night window</label><input type="number" id="inLeak" min="0.2" max="50" step="0.1" inputmode="decimal">
      <p class="help">Pick a quiet time when nobody uses water. Default 01:00–05:00 and 1.5 cm.</p></div>
    <div class="btns">
      <button class="btn" onclick="saveAnalytics()">Save analytics settings</button>
      <button class="btn danger" onclick="clearHistory()">Clear history</button>
    </div>
  </section>
</main>

<main id="tabLog" class="tab" hidden>
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
<footer>
  <div><span id="fSite">Water Tanks Monitor System</span> · <span id="host"></span></div>
  <div class="copy">© 2026 Chanchal Sakarde. All Rights Reserved. · <a href="https://woodyouloveit.com" target="_blank" rel="noopener">woodyouloveit.com</a></div>
  <div>Open source under the <a href="https://www.gnu.org/licenses/gpl-3.0.html" target="_blank" rel="noopener">GNU GPL v3</a> ·
    <a href="https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard" target="_blank" rel="noopener">Source code</a></div>
</footer>
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
  renderSite();if(!siteLoaded)fillSite();
  renderGlance();renderPower();if(!profLoaded)fillProfile();else if(tab==='setup')calPreview();
  $('banners').hidden=!document.querySelector('#banners .banner.show');
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
  const fa=$('alertFill');
  if(s.filling)setAlert(fa,'info',`Filling (motor on) · rising ${s.fillRate.toFixed(1)} cm/min`+(s.fillEta>=0?` · full in about ${dur(s.fillEta)}`:''));
  else setAlert(fa,'','');
  if(!formLoaded)fillForm();
}

// ---- Usage & tank profile ----
// Loft tank sizes (rated litres; outer length x width x height in mm)
const PRESETS=[null,
  {cap:150,l:710,w:710,h:400},{cap:225,l:1035,w:725,h:385},{cap:270,l:1100,w:735,h:425},
  {cap:400,l:1120,w:875,h:420},{cap:500,l:1450,w:915,h:445},{cap:1000,l:1650,w:1080,h:685}];
const USAGE=['Domestic','Commercial'],LOCS=['Overhead (roof)','Loft / bathroom','Underground sump','Other'];
let usageSel=0,profLoaded=false;
const iv=id=>{const v=parseFloat($(id).value);return isFinite(v)?v:0;};
function presetLabel(i){const p=PRESETS[i];return `Loft tank ${p.cap} L · ${p.l} × ${p.w} × ${p.h} mm`;}
function buildPresets(){const sel=$('inPreset'),shape=+$('inShape').value,cur=sel.value;
  sel.innerHTML='<option value="0">Custom size</option>'+(shape===0?PRESETS.slice(1).map((p,i)=>`<option value="${i+1}">${presetLabel(i+1)}</option>`).join(''):'');
  sel.value=[...sel.options].some(o=>o.value===cur)?cur:'0';}
function tankHeight(){return +$('inShape').value?iv('inHgtC'):iv('inHgt');}
function capEstimate(){const d=iv('inDepth');if(!d)return 0;
  return +$('inShape').value?Math.PI*Math.pow(iv('inDia')/2,2)*d/1e6*0.95:iv('inLen')*iv('inWid')*d/1e6*0.85;}
function profileUI(){
  const shape=+$('inShape').value,loc=+$('inLoc').value;
  $('dimRect').hidden=shape!==0;$('dimCyl').hidden=shape!==1;
  $('inPreset').disabled=shape!==0;
  const est=capEstimate();
  $('capHint').textContent=est?`Estimated from the size: about ${Math.round(est)} L. Enter the rated capacity if you know it.`:'Rated capacity from the tank label or catalogue.';
  const tips={1:'Loft tanks are shallow. The sensor cannot measure closer than 20–25 cm, so mount it on a stand pipe (at least 25 cm tall and 75 mm wide) over the manhole, not flat on the lid.',
    2:'Underground sump: keep the probe away from the inlet pipe and walls. The low water alarm helps protect the pump from running dry.',
    0:'Overhead tank: mount the probe at the centre of the lid or manhole, pointing straight down, at least 25 cm above the full water line.'};
  setAlert($('profTip'),tips[loc]?'info':'',tips[loc]||'');
  const r=recommended();
  $('recHelp').textContent=`Recommended for ${USAGE[usageSel].toLowerCase()} use, ${LOCS[loc].toLowerCase()}: night leak check ${hh(r.nightStart)}–${hh(r.nightEnd)}, `+
    `low water alarm ${r.lowAlarm}%, leak limit ${r.leakCm} cm.`;
  $('depthShow').textContent=iv('inDepth')?iv('inDepth')+' mm':'Set it in Usage & tank';
  calPreview();
}
function recommended(){const loc=+$('inLoc').value,depth=iv('inDepth')||spanMm();
  return {nightStart:usageSel?22:1,nightEnd:usageSel?6:5,lowAlarm:loc===2?30:usageSel?25:15,
    leakCm:Math.max(1,Math.round(depth*0.02/5)*0.5)};}
function fillProfile(){if(profLoaded||!st.profile)return;profLoaded=true;const p=st.profile;
  usageSel=p.usage;[...$('segUsage').children].forEach(b=>b.classList.toggle('on',+b.dataset.v===usageSel));
  $('inLoc').value=p.location;$('inShape').value=p.shape;buildPresets();$('inPreset').value=String(p.preset);
  if(!$('inPreset').value)$('inPreset').value='0';
  const z=v=>v?v:'';
  $('inLen').value=z(p.len);$('inWid').value=z(p.wid);$('inHgt').value=z(p.hgt);$('inDia').value=z(p.dia);$('inHgtC').value=z(p.hgt);
  $('inDepth').value=z(p.depth);$('inCapP').value=z(st.capacity);
  $('inGap').value=st.full.toFixed(1);$('inNow').value=Math.round(st.valid?st.level:50);$('nowVal').textContent=$('inNow').value+'%';
  profileUI();}
[...$('segUsage').children].forEach(b=>b.onclick=()=>{usageSel=+b.dataset.v;
  [...$('segUsage').children].forEach(x=>x.classList.toggle('on',x===b));profileUI();});
$('inShape').onchange=()=>{buildPresets();profileUI();};
$('inLoc').onchange=()=>{if(+$('inLoc').value===1&&+$('inShape').value!==0){$('inShape').value='0';buildPresets();}profileUI();};
$('inPreset').onchange=()=>{const p=PRESETS[+$('inPreset').value];
  if(p){$('inLen').value=p.l;$('inWid').value=p.w;$('inHgt').value=p.h;$('inCapP').value=p.cap;$('inDepth').value=p.h-40;}profileUI();};
['inLen','inWid','inHgt','inDia','inHgtC','inDepth'].forEach(id=>$(id).addEventListener('input',()=>{
  if(['inLen','inWid','inHgt'].includes(id))$('inPreset').value='0';profileUI();}));
async function saveProfile(){
  const shape=+$('inShape').value,h=tankHeight(),d=iv('inDepth');
  if(d&&h&&d>h){toast('Water depth cannot be more than the tank height',true);return;}
  const cap=Math.round(iv('inCapP')||capEstimate());
  const j=await post('/api/profile',{usage:usageSel,location:$('inLoc').value,shape,preset:shape?0:$('inPreset').value,
    len:iv('inLen'),wid:iv('inWid'),hgt:h,dia:iv('inDia'),depth:d});
  if(!j.ok)return;
  if(cap!==st.capacity)await post('/api/settings',{capacity:cap});
  $('inCapP').value=cap||'';profLoaded=false;formLoaded=false;await poll();
  if(d&&Math.abs(d-spanMm())>15)toast('Tank saved. Calibrate below so the level matches the new water depth.');
}
async function applyRecommended(){const r=recommended();
  if(!await ask(`Apply recommended settings for ${USAGE[usageSel].toLowerCase()} use? Night leak check ${hh(r.nightStart)}–${hh(r.nightEnd)}, `+
    `low water alarm ${r.lowAlarm}%, leak limit ${r.leakCm} cm.`))return;
  const j=await post('/api/settings',{nightStart:r.nightStart,nightEnd:r.nightEnd,lowAlarm:r.lowAlarm,leakCm:r.leakCm});
  if(j.ok){await post('/api/profile',{usage:usageSel,location:$('inLoc').value});formLoaded=false;profLoaded=false;poll();}}

// ---- Calibration from the slider or from measurements ----
// E = sensor->water when empty, F = sensor->water when full (cm); depth D = E - F
function calCheck(E,F){
  if(F<15)return ['bad',`The full water line would be only ${F.toFixed(1)} cm below the sensor. It can't measure closer than about 20 cm: raise the sensor on a stand pipe.`];
  if(F<22)return ['warn',`The full water line is ${F.toFixed(1)} cm below the sensor, at the edge of its blind zone. Readings near full may jump. 25 cm or more is safer.`];
  if(E>450)return ['bad','The empty point would be more than 450 cm away, beyond the sensor range.'];
  return ['',''];}
function calText(E,F){const D=E-F,l=st.capacity>0?st.capacity/(D*10):0;
  return `Empty at <b>${E.toFixed(1)} cm</b> · full at <b>${F.toFixed(1)} cm</b> from the sensor · water depth ${(D*10).toFixed(0)} mm`+
    (l?` · 1 cm ≈ ${(l*10).toFixed(1)} L`:'');}
function calPreview(){
  const D=iv('inDepth')/10,p=+$('inNow').value/100,box=$('prevSlider'),box2=$('prevMeas');
  $('nowVal').textContent=$('inNow').value+'%'+(st&&st.capacity?` · ${Math.round(st.capacity*p)} L`:'');
  if(!D){box.className='calprev warn';box.textContent='Enter the water depth when full in Usage & tank first.';$('btnSlider').disabled=true;}
  else if(!st||!st.valid){box.className='calprev warn';box.textContent='Waiting for a valid sensor reading.';$('btnSlider').disabled=true;}
  else{const E=st.distance+p*D,F=E-D,[c,m]=calCheck(E,F);box.className='calprev '+c;box.innerHTML=calText(E,F)+(m?'<br>'+m:'');$('btnSlider').disabled=c==='bad';}
  const G=iv('inGap');
  if(!D||!G){box2.className='calprev';box2.textContent='Measure from the sensor face down to the overflow (full) line.';$('btnMeas').disabled=true;}
  else{const E=G+D,[c,m]=calCheck(E,G);box2.className='calprev '+c;box2.innerHTML=calText(E,G)+(m?'<br>'+m:'');$('btnMeas').disabled=c==='bad';}
}
$('inNow').oninput=calPreview;$('inGap').oninput=calPreview;
async function saveCal(E,F,how){
  if(!await ask(`Save calibration ${how}? Empty at ${E.toFixed(1)} cm, full at ${F.toFixed(1)} cm from the sensor.`))return;
  const j=await post('/api/calibrate',{empty:E.toFixed(1),full:F.toFixed(1)});
  if(j.ok){await post('/api/profile',{gap:Math.round(F*10),depth:Math.round((E-F)*10)});formLoaded=false;profLoaded=false;poll();}}
function calSlider(){const D=iv('inDepth')/10,p=+$('inNow').value/100,E=st.distance+p*D;saveCal(E,E-D,`with the tank ${$('inNow').value}% full`);}
function calMeasured(){const D=iv('inDepth')/10,G=iv('inGap');saveCal(G+D,G,'from measurements');}

// ---- Overview: litres and tank at a glance ----
function renderGlance(){
  const s=st,p=s.profile||{},cap=s.capacity,lvl=s.valid?s.level:null,depth=(s.empty-s.full)*10;
  const L=v=>Math.round(v).toLocaleString()+' L';
  $('litres').textContent=cap&&lvl!==null?`≈ ${L(cap*lvl/100)} of ${L(cap)}`:'';
  $('gWater').textContent=lvl===null?'--':cap?L(cap*lvl/100):Math.round(lvl)+'%';
  $('gFree').textContent=lvl===null?'--':cap?L(cap*(100-lvl)/100):Math.round(100-lvl)+'%';
  $('gCap').textContent=cap?L(cap):'not set';
  $('gSite').textContent=siteLine()||'not named';
  $('gUsage').textContent=`${USAGE[p.usage||0]}, ${(LOCS[p.location||0]||'').toLowerCase()}`;
  const dims=p.shape?(p.dia?`cylinder Ø ${p.dia} × ${p.hgt} mm`:'vertical cylinder'):(p.len?`${p.len} × ${p.wid} × ${p.hgt} mm`:'rectangular');
  $('gTank').textContent=(p.preset&&PRESETS[p.preset]?`Loft ${PRESETS[p.preset].cap} L, `:'')+dims;
  $('gDepth').textContent=p.depth?p.depth+' mm':'not set';
  $('gRange').textContent=`${s.full.toFixed(1)} → ${s.empty.toFixed(1)} cm (${depth.toFixed(0)} mm)`;
  $('gGap').textContent=s.full.toFixed(1)+' cm';
  $('gRes').textContent=cap?`≈ ${(cap/depth*10).toFixed(1)} L (${(1000/depth).toFixed(1)}%)`:`${(1000/depth).toFixed(1)}% of the tank`;
  const warn=[];
  if(!p.depth&&Math.abs(s.empty-120)<0.05&&Math.abs(s.full-25)<0.05)warn.push('Not calibrated yet: the level uses default distances. Go to Setup → Calibration.');
  if(s.full<20)warn.push(`The full water line is ${s.full.toFixed(1)} cm from the sensor, inside its blind zone. Raise the sensor on a stand pipe.`);
  if(p.depth&&Math.abs(p.depth-depth)>15)warn.push(`Calibration (${depth.toFixed(0)} mm) doesn't match the tank's water depth (${p.depth} mm). Re-calibrate in Setup.`);
  setAlert($('gWarn'),warn.length?'warn':'',warn.join('<br>'));
}

// ---- Site identity ----
const siteSet=()=>st&&st.site&&(st.site.org||st.site.building||st.site.tank);
function siteLine(sep){const x=st&&st.site;if(!x)return '';return [x.org,x.building,x.tank].filter(Boolean).join(sep||' · ');}
function fileTag(){const x=st&&st.site;if(!x)return '';const t=[x.building,x.tank].filter(Boolean).join('-')
  .toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');return t?t+'-':'';}
function tankTitle(){const x=st.site;return [x.building,x.tank].filter(Boolean).join(' · ');}
function renderSite(){
  const x=st.site,w=st.wifi,t=tankTitle();
  $('hTitle').textContent=t||'Water Tanks Monitor System';
  $('hSub').textContent=t?(x.org?x.org+' · ':'')+'Water Tanks Monitor System':(x.org||'woodyouloveit.com');
  document.title=(t?t+' | ':'')+'Water Tanks Monitor System';
  $('fSite').textContent=siteLine()||'Water Tanks Monitor System';
  $('siteBanner').classList.toggle('show',!siteSet());
  document.querySelectorAll('.apName').forEach(e=>e.textContent=w.apName);
  const renamed=x.nextAp!==w.apName||x.nextHost!==w.host;
  $('sAp').textContent=renamed?`${w.apName} → ${x.nextAp}`:w.apName;
  $('sHost').textContent=renamed?`${w.host}.local → ${x.nextHost}.local`:`${w.host}.local`;
  setAlert($('sRestart'),renamed?'info':'',renamed?'Restart the device to use the new hotspot name and web address.':'');
  $('btnSiteRestart').style.display=renamed?'':'none';
}
async function saveSite(){
  const j=await post('/api/site',{org:$('inOrg').value,building:$('inBld').value,tank:$('inTank').value});
  if(j.ok){siteLoaded=false;await poll();}
}
let siteLoaded=false;
function fillSite(){if(siteLoaded||!st.site)return;siteLoaded=true;
  $('inOrg').value=st.site.org;$('inBld').value=st.site.building;$('inTank').value=st.site.tank;}

function fillForm(){const s=st;formLoaded=true;
  $('inEmpty').value=s.empty;$('inFull').value=s.full;
  $('inLeds').value=s.leds;$('inBr').value=s.brightness;$('brVal').textContent=s.brightness;
  $('inLow').value=s.lowAlarm;$('lowVal').textContent=s.lowAlarm;
  $('inRev').checked=s.reversed;$('inCbl').checked=s.colorByLevel;$('inTrig').value=s.trigUs;updateAmps();
  if(!$('inSsid').value&&s.wifi.saved)$('inSsid').value=s.wifi.saved;
  fillPower();fillAnalyticsForm();fillSite();
  $('calNowE').textContent=s.empty.toFixed(1)+' cm';$('calNowF').textContent=s.full.toFixed(1)+' cm';}
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

// ---- Power & radio ----
const AP_MODES=['Automatic','Always on','On demand (BOOT button)'],TX_LBL=['Medium, 13 dBm','High, 19.5 dBm','Low, 8.5 dBm'];
let txSel=0;
function fillPower(){const s=st;$('inEco').checked=s.perfMode===0;$('inApMode').value=String(s.apMode);txSel=s.txLevel;
  [...$('segTx').children].forEach(b=>b.classList.toggle('on',+b.dataset.v===txSel));powerUI();}
function powerUI(){const m=+$('inApMode').value,eco=$('inEco').checked;
  $('apModeHelp').textContent=[
    'On while the device has no home Wi-Fi or cannot reach it. Off 30 s after home Wi-Fi connects.',
    'Always on. Uses the most power and keeps the board warmest.',
    'Off unless needed: press the BOOT button on the board to turn it on. It turns off after 10 minutes without devices. '+
    'With no home Wi-Fi saved it is still on at start-up for setup.'][m];
  $('pRate').textContent=eco?'2 per second, LEDs refreshed on change':'14 per second, LEDs 10 times a second';}
function renderPower(){const s=st;
  $('pCpu').textContent=s.cpuMhz+' MHz';$('pTx').textContent=TX_LBL[s.txLevel];
  $('pMode').textContent=s.perfMode?'Performance':'Power saving';
  $('wApMode').textContent=AP_MODES[s.apMode]+(s.wifi.ap?', on now':', off now');}
[...$('segTx').children].forEach(b=>b.onclick=()=>{txSel=+b.dataset.v;[...$('segTx').children].forEach(x=>x.classList.toggle('on',x===b));});
$('inApMode').onchange=powerUI;$('inEco').onchange=powerUI;
async function savePower(){
  const m=+$('inApMode').value;
  if(m===2&&!st.wifi.saved&&!await ask('No home Wi-Fi is saved. In on-demand mode the hotspot turns off after 10 minutes without devices, '+
    'and the dashboard is then only reachable after pressing the BOOT button. Continue?'))return;
  const j=await post('/api/settings',{perf:$('inEco').checked?0:1,apMode:m,tx:txSel});
  if(j.ok){formLoaded=false;poll();}}

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
  const head=(st?`Water Tanks Monitor System log · ${siteLine()||'site not named'}\nfirmware ${st.fw} · ${st.board} · ${st.wifi.host}.local · downloaded ${new Date().toString()}\n`:'')+
    `woodyouloveit.com · © 2026 Chanchal Sakarde. All Rights Reserved.\n\n`;
  const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([head+lines.join('\n')+'\n'],{type:'text/plain'}));
  const n=new Date();a.download=`wtms-log-${fileTag()}${n.getFullYear()}${pad(n.getMonth()+1)}${pad(n.getDate())}-${pad(n.getHours())}${pad(n.getMinutes())}.txt`;
  document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},500);}
async function clearLog(){if(!await ask('Clear the connectivity log? This cannot be undone.'))return;
  await post('/api/log/clear');loadLog();}

// ================= Tabs =================
let tab='overview';
const TABS={overview:'tabOverview',analytics:'tabAnalytics',setup:'tabSetup',log:'tabLog'};
function showTab(t){
  // "#cWifi" etc. open the tab that holds that card and scroll to it
  let target=null;
  if(!TABS[t]){const el=t&&document.getElementById(t),m=el&&el.closest('main.tab');
    if(m){target=el;t=Object.keys(TABS).find(k=>TABS[k]===m.id);}}
  tab=TABS[t]?t:'overview';
  document.querySelectorAll('.tabs button').forEach(b=>b.classList.toggle('on',b.dataset.tab===tab));
  for(const k in TABS)$(TABS[k]).hidden=tab!==k;
  if(target)setTimeout(()=>target.scrollIntoView({behavior:'smooth',block:'start'}),50);
  if(tab==='analytics')loadAnalytics();if(tab==='log')loadLog();}
document.querySelectorAll('.tabs button').forEach(b=>b.onclick=()=>{location.hash=b.dataset.tab;});
window.addEventListener('hashchange',()=>showTab(location.hash.slice(1)));

// ================= Analytics =================
let hist=[],fills=[],aLoaded=false,rangeH=24,ana=null;
const DAY=86400,hh=h=>pad(h)+':00';
const nowT=()=>st&&st.epoch?st.epoch:Math.floor(Date.now()/1000);
const spanMm=()=>(st.empty-st.full)*10;
const pctOf=mm=>Math.min(100,Math.max(0,(st.empty*10-mm)/spanMm()*100));
const lpm=()=>st.capacity>0?st.capacity/spanMm():0;
function amt(mm,signed){const s=signed&&mm>0?'+':'';const l=lpm();
  return l?s+Math.round(mm*l).toLocaleString()+' L':s+(mm/spanMm()*100).toFixed(1)+'%';}
function dur(min){min=Math.round(min);return min<60?min+' min':Math.floor(min/60)+' h '+pad(min%60)+' min';}
const dayKey=t=>{const d=new Date(t*1000);return d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate());};
const dayLbl=(t,long)=>new Date(t*1000).toLocaleDateString([], long?{weekday:'short',day:'2-digit',month:'short'}:{weekday:'short'});
const tLbl=t=>new Date(t*1000).toLocaleString([], {weekday:'short',hour:'2-digit',minute:'2-digit',hour12:false});
function startOfDay(t){const d=new Date(t*1000);d.setHours(0,0,0,0);return d.getTime()/1000;}
function median(a){if(!a.length)return null;const s=a.slice().sort((x,y)=>x-y);return s[s.length>>1];}

async function loadAnalytics(){
  try{
    const [hb,fb]=await Promise.all([fetch('/api/history',{cache:'no-store'}).then(r=>r.arrayBuffer()),
                                     fetch('/api/fills',{cache:'no-store'}).then(r=>r.arrayBuffer())]);
    const dv=new DataView(hb);hist=[];
    for(let o=0;o+8<=hb.byteLength;o+=8){const mm=dv.getUint16(o+4,true);
      hist.push({t:dv.getUint32(o,true),mm:mm===65535?null:mm,fill:dv.getUint16(o+6,true)&1});}
    hist.sort((a,b)=>a.t-b.t);
    const fv=new DataView(fb);fills=[];
    for(let o=0;o+12<=fb.byteLength;o+=12)fills.push({s:fv.getUint32(o,true),e:fv.getUint32(o+4,true),smm:fv.getUint16(o+8,true),emm:fv.getUint16(o+10,true)});
    fills.sort((a,b)=>a.s-b.s);aLoaded=true;
  }catch(e){aLoaded=false;}
  if(st)renderAnalytics();
}

// Water used: follow the level with a small dead band so sensor noise is not counted
function usageEvents(){
  const DB=3,out=[];let ref=null,prevT=0;
  for(const r of hist){
    if(r.mm===null){continue;}
    if(r.fill||ref===null||r.t-prevT>900){ref=r.mm;prevT=r.t;continue;}
    prevT=r.t;
    if(r.mm>ref+DB){out.push({t:r.t,mm:r.mm-DB-ref});ref=r.mm-DB;}
    else if(r.mm<ref-DB)ref=r.mm+DB;
  }
  return out;
}
function allFills(){const f=fills.map(x=>({...x,live:false}));
  if(st.filling&&st.fillStart)f.push({s:st.fillStart,e:nowT(),smm:null,emm:null,live:true});return f;}

function nightChecks(){
  const res=[],now=nowT(),hours=((st.nightEnd-st.nightStart+24)%24)||24,leak=st.leakCm*10;
  for(let k=0;k<7;k++){
    const d=new Date(now*1000);d.setDate(d.getDate()-k);d.setHours(st.nightEnd,0,0,0);
    const end=d.getTime()/1000,start=end-hours*3600;if(end>now)continue;
    const rec=hist.filter(r=>r.t>=start&&r.t<=end),valid=rec.filter(r=>r.mm!==null);
    const row={end,start,hours,drop:null,status:'na',text:'No data'};
    if(valid.length<hours*30*0.6){res.push(row);continue;}
    if(rec.some(r=>r.fill)||fills.some(f=>f.s<end&&f.e>start)){row.text='Tank was filling';res.push(row);continue;}
    const a=median(valid.slice(0,5).map(r=>r.mm)),b=median(valid.slice(-5).map(r=>r.mm));
    row.drop=b-a;
    // A leak drops slowly and steadily; someone using water drops it in one step.
    // Each hour is measured from the end of the previous one, so a drop right on the hour is not missed.
    let maxHour=0,prev=median(valid.slice(0,3).map(r=>r.mm));
    for(let h=0;h<hours;h++){const s=valid.filter(r=>r.t>=start+h*3600&&r.t<start+(h+1)*3600);
      if(s.length<3)continue;const endH=median(s.slice(-3).map(r=>r.mm));maxHour=Math.max(maxHour,endH-prev);prev=endH;}
    if(row.drop>=leak&&hours>=3&&maxHour>=0.7*row.drop){row.status='warn';row.text='Water used (one drop)';}
    else if(row.drop>=leak){row.status='bad';row.text='Possible leak';}
    else if(row.drop>=leak/2){row.status='warn';row.text='Watch';}
    else{row.status='ok';row.text='OK';}
    res.push(row);
  }
  return res;
}

// ---------- charts (plain SVG, no libraries) ----------
const NS='http://www.w3.org/2000/svg';
function el(tag,attrs,parent){const e=document.createElementNS(NS,tag);for(const k in attrs)e.setAttribute(k,attrs[k]);if(parent)parent.appendChild(e);return e;}
function css(v){return getComputedStyle(document.documentElement).getPropertyValue(v).trim();}

function levelChart(box){
  box.innerHTML='';const W=Math.max(300,box.clientWidth),H=Math.round(Math.min(280,Math.max(190,W*0.32)));
  const L=36,Rr=10,T=10,B=26,end=nowT(),start=end-rangeH*3600;
  const x=t=>L+(t-start)/(end-start)*(W-L-Rr),y=p=>T+(100-p)/100*(H-T-B);
  const svg=el('svg',{viewBox:`0 0 ${W} ${H}`},box);
  // night bands
  for(let d=startOfDay(start)-DAY;d<=end;d+=DAY){
    let s=d+st.nightStart*3600,e=d+st.nightEnd*3600;if(e<=s)e+=DAY;
    const a=Math.max(s,start),b=Math.min(e,end);if(b>a)el('rect',{x:x(a),y:T,width:x(b)-x(a),height:H-T-B,fill:'rgba(100,116,139,.16)'},svg);}
  // fill bands
  for(const f of allFills()){const a=Math.max(f.s,start),b=Math.min(f.e,end);
    if(b>a)el('rect',{x:x(a),y:T,width:Math.max(2,x(b)-x(a)),height:H-T-B,fill:'rgba(22,163,74,.22)'},svg);}
  // grid
  for(const p of [0,25,50,75,100]){el('line',{x1:L,x2:W-Rr,y1:y(p),y2:y(p),class:'grid'},svg);
    el('text',{x:L-6,y:y(p)+4,'text-anchor':'end',class:'axis'},svg).textContent=p+'%';}
  // x ticks
  const step=rangeH<=24?(W<500?6:3)*3600:DAY;
  let t0=rangeH<=24?Math.ceil(start/step)*step:startOfDay(start)+DAY;
  if(rangeH<=24){const d=new Date(start*1000);d.setMinutes(0,0,0);t0=d.getTime()/1000;while(t0<start||new Date(t0*1000).getHours()%(step/3600))t0+=3600;}
  for(let t=t0;t<=end;t+=step){el('line',{x1:x(t),x2:x(t),y1:T,y2:H-B,class:'grid','stroke-dasharray':'2 3'},svg);
    el('text',{x:x(t),y:H-8,'text-anchor':'middle',class:'axis'},svg).textContent=rangeH<=24?hh(new Date(t*1000).getHours()):dayLbl(t);}
  // line + area, broken at gaps
  const pts=hist.filter(r=>r.t>=start-300&&r.t<=end);let seg=[],segs=[];
  for(const r of pts){if(r.mm===null||(seg.length&&r.t-seg[seg.length-1].t>600)){if(seg.length)segs.push(seg);seg=[];}if(r.mm!==null)seg.push(r);}
  if(seg.length)segs.push(seg);
  const acc=css('--accent');
  for(const s of segs){const d=s.map((r,i)=>(i?'L':'M')+x(r.t).toFixed(1)+','+y(pctOf(r.mm)).toFixed(1)).join('');
    el('path',{d:d+`L${x(s[s.length-1].t).toFixed(1)},${y(0)}L${x(s[0].t).toFixed(1)},${y(0)}Z`,fill:acc,opacity:.12},svg);
    el('path',{d,fill:'none',stroke:acc,'stroke-width':2,'stroke-linejoin':'round'},svg);}
  if(!pts.length){el('text',{x:W/2,y:H/2,'text-anchor':'middle',class:'axis'},svg).textContent='No history in this range yet';}
  // hover / touch tooltip
  const tip=document.createElement('div');tip.className='tip';box.appendChild(tip);
  const cur=el('line',{y1:T,y2:H-B,stroke:css('--muted'),'stroke-width':1,visibility:'hidden'},svg);
  const move=ev=>{const rc=svg.getBoundingClientRect(),px=(ev.clientX-rc.left)*W/rc.width,t=start+(px-L)/(W-L-Rr)*(end-start);
    let best=null;for(const r of pts)if(r.mm!==null&&(!best||Math.abs(r.t-t)<Math.abs(best.t-t)))best=r;
    if(!best||Math.abs(best.t-t)>1800){tip.style.display='none';cur.setAttribute('visibility','hidden');return;}
    const X=x(best.t);cur.setAttribute('x1',X);cur.setAttribute('x2',X);cur.setAttribute('visibility','visible');
    tip.textContent=`${tLbl(best.t)} · ${pctOf(best.mm).toFixed(0)}%${best.fill?' · filling':''}`;
    tip.style.left=(X/W*100)+'%';tip.style.display='block';};
  svg.addEventListener('pointermove',move);svg.addEventListener('pointerdown',move);
  svg.addEventListener('pointerleave',()=>{tip.style.display='none';cur.setAttribute('visibility','hidden');});
}

function barChart(box,labels,values,fmt,hl,short){
  box.innerHTML='';const W=Math.max(280,box.clientWidth),H=190,L=8,Rr=8,T=18,B=24;
  const svg=el('svg',{viewBox:`0 0 ${W} ${H}`},box),max=Math.max(...values,0);
  if(!max){el('text',{x:W/2,y:H/2,'text-anchor':'middle',class:'axis'},svg).textContent='Not enough data yet';return;}
  const n=values.length,bw=(W-L-Rr)/n,acc=css('--accent'),dim=css('--line'),f=short&&bw<60?short:fmt;
  el('line',{x1:L,x2:W-Rr,y1:H-B,y2:H-B,class:'grid'},svg);
  values.forEach((v,i)=>{const h=v/max*(H-T-B),X=L+i*bw+bw*0.15,w=bw*0.7;
    el('rect',{x:X,y:H-B-h,width:w,height:Math.max(v?1:0,h),rx:3,fill:hl&&!hl.has(i)?dim:acc},svg);
    if(n<=10&&v)el('text',{x:X+w/2,y:H-B-h-5,'text-anchor':'middle',class:'axis'},svg).textContent=f(v);
    if(n<=10||i%3===0)el('text',{x:X+w/2,y:H-7,'text-anchor':'middle',class:'axis'},svg).textContent=labels[i];});
}

function renderAnalytics(){
  const msg=$('aMsg');
  if(st.logStore!=='flash'){setAlert(msg,'bad','History needs flash storage. On ESP8266 select Tools → Flash Size → 4MB (FS:2MB) and upload again.');}
  else if(!st.epoch){setAlert(msg,'info','Waiting for the clock. History is recorded once the device knows the time: connect it to home Wi-Fi, or keep this page open (your browser sets the clock).');}
  else if(!aLoaded){setAlert(msg,'bad','Could not load history from the device.');}
  else if(hist.length<5){setAlert(msg,'info','Collecting data. A point is recorded every 2 minutes; fill, usage and night statistics appear after a few hours.');}
  else setAlert(msg,'','');
  if(!aLoaded)return;
  const now=nowT(),today=startOfDay(now),use=usageEvents(),af=allFills(),nights=nightChecks();
  // ---- daily buckets, last 7 days ----
  const days=[];for(let k=6;k>=0;k--)days.push(today-k*DAY);
  const motor=days.map(d=>af.filter(f=>f.s>=d&&f.s<d+DAY).reduce((a,f)=>a+(f.e-f.s)/60,0));
  const used=days.map(d=>use.filter(u=>u.t>=d&&u.t<d+DAY).reduce((a,u)=>a+u.mm,0));
  const dl=days.map(d=>dayLbl(d));
  barChart($('motorChart'),dl,motor,v=>dur(v),null,v=>Math.round(v)+'m');
  const l=lpm(),kL=v=>{const x=v*l;return x>=1000?(x/1000).toFixed(1)+'kL':Math.round(x)+'L';};
  barChart($('useChart'),dl,used,v=>l?Math.round(v*l)+'L':(v/spanMm()*100).toFixed(0)+'%',null,v=>l?kL(v):(v/spanMm()*100).toFixed(0)+'%');
  // ---- hour of day ----
  const dayCount=new Set(hist.filter(r=>r.mm!==null).map(r=>dayKey(r.t))).size||1;
  const hours=Array(24).fill(0);use.forEach(u=>hours[new Date(u.t*1000).getHours()]+=u.mm);
  const avgH=hours.map(v=>v/dayCount),top=avgH.map((v,i)=>[v,i]).sort((a,b)=>b[0]-a[0]).filter(x=>x[0]>0).slice(0,3).map(x=>x[1]);
  barChart($('hourChart'),avgH.map((_,i)=>pad(i)),avgH,v=>amt(v),new Set(top));
  const totH=avgH.reduce((a,b)=>a+b,0);
  $('hourHelp').textContent=top.length?`Average per day over ${dayCount} day${dayCount>1?'s':''}. Busiest: `+top.map(i=>`${hh(i)}–${hh((i+1)%24)} (${Math.round(avgH[i]/totH*100)}%)`).join(', ')+'.':'Average per day. Appears once water use has been recorded.';
  // ---- fills ----
  const done=fills.filter(f=>f.e-f.s>=180);
  const rate=done.length?done.reduce((a,f)=>a+(f.smm-f.emm),0)/done.reduce((a,f)=>a+(f.e-f.s)/60,0):0;
  $('fillList').innerHTML='';
  done.slice(-5).reverse().forEach(f=>{const d=document.createElement('div');
    d.innerHTML=`<span>${new Date(f.s*1000).toLocaleString([], {weekday:'short',day:'2-digit',month:'short',hour:'2-digit',minute:'2-digit',hour12:false})}</span><span>${dur((f.e-f.s)/60)} · ${amt(f.smm-f.emm,true)}</span>`;
    $('fillList').appendChild(d);});
  if(!done.length)$('fillList').innerHTML='<div><span class="help">No fills recorded yet.</span><span></span></div>';
  // ---- night table ----
  const leakRow=nights.find(n=>n.status!=='na'&&n.text!=='Tank was filling');
  $('nightTable').innerHTML='<table class="nt"><thead><tr><th>Night</th><th>Change</th><th>Per hour</th><th>Result</th></tr></thead><tbody>'+
    nights.map(n=>`<tr><td>${dayLbl(n.end,true)}</td><td>${n.drop===null?'–':(n.drop>0?'−':'+')+amt(Math.abs(n.drop))}</td>`+
      `<td>${n.drop===null?'–':amt(Math.abs(n.drop)/n.hours)}</td><td><span class="pill ${n.status}">${n.text}</span></td></tr>`).join('')+'</tbody></table>';
  $('nightHelp').textContent=`Level change between ${hh(st.nightStart)} and ${hh(st.nightEnd)} each night. `+
    `"Possible leak" when it drops ${st.leakCm} cm${l?' ('+Math.round(st.leakCm*10*l)+' L)':''} or more, slowly and steadily, with no fill running. `+
    `"Water used (one drop)" when most of it happened within one hour, like a tap or flush. "Watch" from half of the limit.`;
  // ---- KPIs ----
  const k=[];const last=done[done.length-1];
  if(st.filling)k.push(['live','NOW','Filling',st.fillEta>=0?`full in ~${dur(st.fillEta)} · ${st.fillRate.toFixed(1)} cm/min`:`${st.fillRate.toFixed(1)} cm/min`]);
  else k.push(['','NOW',st.valid?Math.round(st.level)+'%':'–','motor off']);
  k.push(['','LAST FILL',last?amt(last.smm-last.emm,true):'–',last?`${tLbl(last.s)} · ${dur((last.e-last.s)/60)}`:'none recorded yet']);
  k.push(['','EMPTY → FULL TAKES',rate>0?'≈ '+dur(spanMm()/rate):'–',rate>0?`average of ${done.length} fill${done.length>1?'s':''}`:'needs one full fill']);
  k.push(['','MOTOR TODAY',dur(motor[6]),`${af.filter(f=>f.s>=today).length} fill(s) · 7-day avg ${dur(motor.reduce((a,b)=>a+b,0)/7)}`]);
  k.push(['','USED TODAY',amt(used[6]),`yesterday ${amt(used[5])}`]);
  k.push(['','BUSIEST HOUR',top.length?hh(top[0])+'–'+hh((top[0]+1)%24):'–',top.length?`${amt(avgH[top[0]])} per day`:'not enough data']);
  if(leakRow)k.push([leakRow.status,'LAST NIGHT',leakRow.text,leakRow.drop===null?'':`${leakRow.drop>0?'−':'+'}${amt(Math.abs(leakRow.drop))} between ${hh(st.nightStart)}–${hh(st.nightEnd)}`]);
  else k.push(['','LAST NIGHT','–','no complete night yet']);
  ana={now,today,days,dl,motor,used,avgH,top,dayCount,done,nights,rate,k,af,use};
  $('kpis').innerHTML=k.map(([c,a,b,s])=>`<div class="kpi ${c}"><span>${a}</span><b>${b}</b><small>${s}</small></div>`).join('');
  if(tab==='analytics')levelChart($('levelChart'));
  // overview leak banner
  const lb=$('alertLeak');
  if(leakRow&&leakRow.status==='bad'&&now-leakRow.end<DAY)setAlert(lb,'bad',`Possible leak last night: level dropped ${amt(leakRow.drop)} between ${hh(st.nightStart)} and ${hh(st.nightEnd)}. <a href="#analytics">See analytics</a>`);
  else setAlert(lb,'','');
}
document.querySelectorAll('#rangeSeg button').forEach(b=>b.onclick=()=>{rangeH=+b.dataset.h;
  document.querySelectorAll('#rangeSeg button').forEach(x=>x.classList.toggle('on',x===b));levelChart($('levelChart'));});
let rsz;window.addEventListener('resize',()=>{clearTimeout(rsz);rsz=setTimeout(()=>{if(aLoaded&&st)renderAnalytics();},200);});

for(let i=0;i<24;i++){$('inNs').add(new Option(hh(i),i));$('inNe').add(new Option(hh(i),i));}
function fillAnalyticsForm(){$('inCap').value=st.capacity;$('inFillCm').value=st.fillCm;$('inNs').value=st.nightStart;$('inNe').value=st.nightEnd;$('inLeak').value=st.leakCm;}
async function saveAnalytics(){
  if($('inNs').value===$('inNe').value){toast('Night window start and end must differ',true);return;}
  const j=await post('/api/settings',{capacity:$('inCap').value||0,fillCm:$('inFillCm').value,nightStart:$('inNs').value,nightEnd:$('inNe').value,leakCm:$('inLeak').value});
  if(j.ok){formLoaded=false;await poll();renderAnalytics();}}
async function clearHistory(){if(!await ask('Clear all tank history and recorded fills? This cannot be undone.'))return;
  await post('/api/history/clear');loadAnalytics();}

// ================= PDF report (written directly in the browser, no libraries) =================
const BRAND={site:'woodyouloveit.com',url:'https://woodyouloveit.com',owner:'Chanchal Sakarde',year:'2026',
  repo:'github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard'};
// Helvetica / Helvetica-Bold character widths for ASCII 32..126 (standard AFM metrics)
const HW=[278,278,355,556,556,889,667,191,333,333,389,584,278,333,278,278,556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,333,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,556,556,333,500,278,556,500,722,500,500,500,334,260,334,584];
const HBW=[278,333,474,556,556,889,722,238,333,333,389,584,278,333,278,278,556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,333,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,611,611,389,556,333,611,556,778,556,556,500,389,280,389,584];
const HX={0x96:556,0x97:1000,0xa9:737,0xb7:278,0xb0:400,0xd7:584,0x95:350,0x85:1000};
const WA={'\u2013':'\x96','\u2014':'\x97','\u00b7':'\xb7','\u00a9':'\xa9','\u00b0':'\xb0','\u00d7':'\xd7','\u2022':'\x95',
  '\u2026':'\x85','\u2018':"'",'\u2019':"'",'\u201c':'"','\u201d':'"','\u2248':'~','\u2192':'->','\u2212':'-','\u00a0':' '};
function wa(s){let o='';for(const ch of String(s)){const c=ch.codePointAt(0);
  if(WA[ch]!==undefined)o+=WA[ch];else if(c<256)o+=ch;else if(c<0x1f000)o+='?';}return o;}
function tw(s,size,bold){const t=bold?HBW:HW;let w=0;
  for(let i=0;i<s.length;i++){const c=s.charCodeAt(i);w+=c>=32&&c<=126?t[c-32]:(HX[c]||556);}return w*size/1000;}

class Pdf{
  constructor(){this.W=595.28;this.H=841.89;this.pages=[];this.img=null;}
  page(){this.c=[];this.pages.push(this.c);}
  o(s){this.c.push(s);}
  col(hex){const n=parseInt(hex.slice(1),16);return [(n>>16)&255,(n>>8)&255,n&255].map(v=>(v/255).toFixed(3)).join(' ');}
  Y(y){return (this.H-y).toFixed(2);}
  rect(x,y,w,h,fill,stroke,lw){
    if(fill)this.o(this.col(fill)+' rg');if(stroke)this.o(this.col(stroke)+' RG '+(lw||0.6)+' w');
    this.o(`${x.toFixed(2)} ${(this.H-y-h).toFixed(2)} ${w.toFixed(2)} ${h.toFixed(2)} re ${fill&&stroke?'B':fill?'f':'S'}`);}
  line(x1,y1,x2,y2,hex,lw,dash){
    this.o(`${this.col(hex)} RG ${lw||0.6} w ${dash?'[2 2] 0 d':''} ${x1.toFixed(2)} ${this.Y(y1)} m ${x2.toFixed(2)} ${this.Y(y2)} l S${dash?' [] 0 d':''}`);}
  poly(pts,hex,lw,fill,baseY){
    if(pts.length<2)return;
    const seg=pts.map((p,i)=>`${p[0].toFixed(2)} ${this.Y(p[1])} ${i?'l':'m'}`).join(' ');
    if(fill)this.o(`${this.col(fill)} rg ${seg} ${pts[pts.length-1][0].toFixed(2)} ${this.Y(baseY)} l ${pts[0][0].toFixed(2)} ${this.Y(baseY)} l h f`);
    this.o(`${this.col(hex)} RG ${lw} w 1 j ${seg} S`);}
  text(x,y,s,size,opt={}){
    const t=wa(s),w=tw(t,size,opt.bold);
    if(opt.align==='right')x-=w;else if(opt.align==='center')x-=w/2;
    const esc=t.replace(/\\/g,'\\\\').replace(/\(/g,'\\(').replace(/\)/g,'\\)');
    this.o(`${this.col(opt.color||'#0f172a')} rg BT /${opt.bold?'F2':'F1'} ${size} Tf ${x.toFixed(2)} ${this.Y(y)} Td (${esc}) Tj ET`);
    return w;}
  wrap(x,y,s,size,maxW,lh,opt={}){
    let line='';
    for(const wd of wa(s).split(' ')){const test=line?line+' '+wd:wd;
      if(tw(test,size,opt.bold)>maxW&&line){this.text(x,y,line,size,opt);y+=lh;line=wd;}else line=test;}
    if(line){this.text(x,y,line,size,opt);y+=lh;}
    return y;}
  image(x,y,w,h){this.o(`q ${w.toFixed(2)} 0 0 ${h.toFixed(2)} ${x.toFixed(2)} ${(this.H-y-h).toFixed(2)} cm /Im1 Do Q`);}
  blob(info){
    const enc=s=>{const a=new Uint8Array(s.length);for(let i=0;i<s.length;i++)a[i]=s.charCodeAt(i)&255;return a;};
    const pstr=s=>'('+wa(s).replace(/\\/g,'\\\\').replace(/\(/g,'\\(').replace(/\)/g,'\\)')+')';
    const parts=[],offs=[],objs=[];let len=0;
    const add=x=>{const b=typeof x==='string'?enc(x):x;parts.push(b);len+=b.length;};
    const nP=this.pages.length,first=6,infoN=first+nP*2;
    objs[1]='<< /Type /Catalog /Pages 2 0 R >>';
    objs[2]=`<< /Type /Pages /Kids [${this.pages.map((_,i)=>(first+i*2)+' 0 R').join(' ')}] /Count ${nP} >>`;
    objs[3]='<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>';
    objs[4]='<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>';
    objs[5]=this.img?{head:`<< /Type /XObject /Subtype /Image /Width ${this.img.w} /Height ${this.img.h} /ColorSpace /DeviceRGB `+
      `/BitsPerComponent 8 /Filter /DCTDecode /Length ${this.img.data.length} >>\nstream\n`,bin:this.img.data,tail:'\nendstream'}:'<< >>';
    const res=`<< /Font << /F1 3 0 R /F2 4 0 R >>${this.img?' /XObject << /Im1 5 0 R >>':''} >>`;
    this.pages.forEach((c,i)=>{const n=first+i*2,body=c.join('\n');
      objs[n]=`<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ${this.W} ${this.H}] /Resources ${res} /Contents ${n+1} 0 R >>`;
      objs[n+1]={head:`<< /Length ${body.length} >>\nstream\n`,bin:enc(body),tail:'\nendstream'};});
    objs[infoN]=`<< /Title ${pstr(info.title)} /Author ${pstr(info.author)} /Subject ${pstr(info.subject)} /Creator ${pstr(info.creator)} /Producer ${pstr(info.creator)} >>`;
    add('%PDF-1.4\n%\xe2\xe3\xcf\xd3\n');
    for(let n=1;n<objs.length;n++){offs[n]=len;add(n+' 0 obj\n');const o=objs[n];
      if(typeof o==='string')add(o);else{add(o.head);add(o.bin);add(o.tail);}add('\nendobj\n');}
    const xref=len;let x=`xref\n0 ${objs.length}\n0000000000 65535 f \n`;
    for(let n=1;n<objs.length;n++)x+=String(offs[n]).padStart(10,'0')+' 00000 n \n';
    add(x+`trailer\n<< /Size ${objs.length} /Root 1 0 R /Info ${infoN} 0 R >>\nstartxref\n${xref}\n%%EOF\n`);
    return new Blob(parts,{type:'application/pdf'});}
}

async function logoJpeg(){
  const img=$('brandLogo');if(!img)return null;
  if(!img.complete)await new Promise(r=>{img.onload=r;img.onerror=r;});
  const s=2,c=document.createElement('canvas');c.width=img.naturalWidth*s;c.height=img.naturalHeight*s;
  const g=c.getContext('2d');g.fillStyle='#ffffff';g.fillRect(0,0,c.width,c.height);g.drawImage(img,0,0,c.width,c.height);
  const bin=atob(c.toDataURL('image/jpeg',0.92).split(',')[1]),a=new Uint8Array(bin.length);
  for(let i=0;i<bin.length;i++)a[i]=bin.charCodeAt(i);
  return {w:c.width,h:c.height,data:a};
}

function buildReport(logo){
  const P=new Pdf(),M=40,CW=P.W-2*M,A=ana,C={acc:'#0f766e',txt:'#0f172a',mut:'#64748b',line:'#e2e8f0',panel:'#f4f7fa',
    red:'#dc2626',warn:'#d97706',ok:'#16a34a',brand:'#e7004e',fillBand:'#d3f1de',nightBand:'#e9edf2',area:'#dbeeec'};
  P.img=logo;
  const now=nowT(),fmtD=t=>new Date(t*1000).toLocaleDateString([], {day:'2-digit',month:'short',year:'numeric'});
  const fmtDT=t=>new Date(t*1000).toLocaleString([], {weekday:'short',day:'2-digit',month:'short',year:'numeric',hour:'2-digit',minute:'2-digit',hour12:false});
  const l=lpm();
  let y=0;
  const S=st.site||{},tankT=[S.building,S.tank].filter(Boolean).join(' - ');
  const header=()=>{P.page();
    if(logo){const h=26,w=h*logo.w/logo.h;P.image(M,28,w,h);}else P.text(M,48,BRAND.site,16,{bold:true});
    P.text(P.W-M,40,'Tank Analytics Report',15,{bold:true,align:'right'});
    P.text(P.W-M,53,tankT||'Water Tanks Monitor System',8.5,{align:'right',color:C.txt,bold:!!tankT});
    P.text(P.W-M,65,BRAND.site,8.5,{align:'right',color:C.brand,bold:true});
    P.line(M,73,P.W-M,73,C.brand,1.2);y=90;};
  const section=(t,x,w)=>{P.text(x??M,y,t.toUpperCase(),8.5,{bold:true,color:C.mut});P.line(x??M,y+4,(x??M)+(w??CW),y+4,C.line,0.6);};
  const need=h=>{if(y+h>780)header();};

  // ---- chart helpers ----
  const bars=(x,top,w,h,labels,values,fmt,hl,every)=>{
    const max=Math.max(...values,0),B=14,T=12,base=top+h-B;
    P.line(x,base,x+w,base,C.line,0.6);
    if(!max){P.text(x+w/2,top+h/2,'Not enough data yet',8,{align:'center',color:C.mut});return;}
    const n=values.length,bw=w/n;
    values.forEach((v,i)=>{const bh=v/max*(h-B-T),bx=x+i*bw+bw*0.18,ww=bw*0.64;
      if(v)P.rect(bx,base-bh,ww,Math.max(0.6,bh),hl&&!hl.has(i)?'#cbd5e1':C.acc);
      if(n<=10&&v)P.text(bx+ww/2,base-bh-3,fmt(v),6.5,{align:'center',color:C.txt});
      if(!every||i%every===0)P.text(bx+ww/2,base+9,labels[i],6.5,{align:'center',color:C.mut});});};

  const levelChart=(x,top,w,h,hours)=>{
    const L=26,B=14,end=now,start=end-hours*3600,ph=h-B,X=t=>x+L+(t-start)/(end-start)*(w-L),Yp=p=>top+(100-p)/100*ph;
    for(let d=startOfDay(start)-DAY;d<=end;d+=DAY){let s=d+st.nightStart*3600,e=d+st.nightEnd*3600;if(e<=s)e+=DAY;
      const a=Math.max(s,start),b=Math.min(e,end);if(b>a)P.rect(X(a),top,X(b)-X(a),ph,C.nightBand);}
    for(const f of A.af){const a=Math.max(f.s,start),b=Math.min(f.e,end);if(b>a)P.rect(X(a),top,Math.max(1,X(b)-X(a)),ph,C.fillBand);}
    for(const p of [0,25,50,75,100]){P.line(x+L,Yp(p),x+w,Yp(p),C.line,0.5);P.text(x+L-4,Yp(p)+2.5,p+'%',6.5,{align:'right',color:C.mut});}
    for(let t=startOfDay(start)+DAY;t<=end;t+=DAY){P.line(X(t),top,X(t),top+ph,C.line,0.5,true);
      P.text(X(t)+2,top+ph+10,new Date(t*1000).toLocaleDateString([], {weekday:'short',day:'2-digit'}),6.5,{color:C.mut});}
    // downsample to ~1 point per 1.5 pt, break at gaps
    const pts=hist.filter(r=>r.t>=start&&r.t<=end&&r.mm!==null),step=(end-start)/((w-L)/1.5);
    let seg=[],last=null,bucket=null,acc=[];
    const flush=()=>{if(acc.length){const t=acc.reduce((a,r)=>a+r.t,0)/acc.length,v=acc.reduce((a,r)=>a+pctOf(r.mm),0)/acc.length;seg.push([X(t),Yp(v)]);acc=[];}};
    const draw=()=>{flush();if(seg.length>1)P.poly(seg,C.acc,1,C.area,top+ph);seg=[];};
    for(const r of pts){if(last&&r.t-last>600)draw();const b=Math.floor((r.t-start)/step);if(b!==bucket){flush();bucket=b;}acc.push(r);last=r.t;}
    draw();
    if(!pts.length)P.text(x+w/2,top+ph/2,'No history in this period yet',8,{align:'center',color:C.mut});
    // legend
    P.rect(x+L,top+h+4,8,6,C.fillBand);P.text(x+L+11,top+h+9.5,'Filling (motor on)',6.5,{color:C.mut});
    P.rect(x+L+80,top+h+4,8,6,C.nightBand);P.text(x+L+91,top+h+9.5,`Night check window ${hh(st.nightStart)}-${hh(st.nightEnd)}`,6.5,{color:C.mut});};

  const table=(x,w,cols,rows,colors)=>{ // cols: [title, widthFraction, align]
    const rh=15;P.rect(x,y,w,rh,C.panel);let cx=x;
    cols.forEach(([t,f,al])=>{const cw=w*f;P.text(al==='right'?cx+cw-6:cx+6,y+10,t,7,{bold:true,color:C.mut,align:al});cx+=cw;});
    y+=rh;
    rows.forEach((r,ri)=>{need(rh);cx=x;
      r.forEach((v,i)=>{const [,f,al]=cols[i],cw=w*f;P.text(al==='right'?cx+cw-6:cx+6,y+10,v,7.5,{align:al,color:(colors&&colors[ri]&&colors[ri][i])||C.txt,bold:!!(colors&&colors[ri]&&colors[ri][i])});cx+=cw;});
      P.line(x,y+rh,x+w,y+rh,C.line,0.5);y+=rh;});};

  // ================= Page 1 =================
  header();
  const period=`${fmtD(now-7*DAY)} - ${fmtD(now)}`,ns='Not set';
  // site title
  P.text(M,y+4,S.org||'Society / organisation not set',14,{bold:true,color:S.org?C.txt:C.mut});
  P.text(M,y+18,`Building: ${S.building||ns}   \u00b7   Tank: ${S.tank||ns}`,9,{color:C.txt});
  y+=28;
  P.rect(M,y,CW,85,C.panel);
  const info=[['Society',S.org||ns],['Building',S.building||ns],['Tank name',S.tank||ns],['Generated',fmtDT(now)],
    ['Period','Last 7 days, '+period],['Device',`${st.board}, firmware ${st.fw}`],
    ['Usage',`${USAGE[(st.profile||{}).usage||0]}, ${(LOCS[(st.profile||{}).location||0]||'').toLowerCase()}`],
    ['Tank',$('gTank').textContent+(st.capacity&&!(st.profile||{}).preset?', '+st.capacity.toLocaleString()+' L':'')],
    ['Calibration',`empty at ${st.empty.toFixed(1)} cm, full at ${st.full.toFixed(1)} cm, water depth ${((st.empty-st.full)*10).toFixed(0)} mm`]];
  info.forEach(([k,v],i)=>{const cx=M+10+(i%2)*(CW/2),cy=y+15+Math.floor(i/2)*15;
    P.text(cx,cy,k,7,{bold:true,color:C.mut});P.text(cx+56,cy,v,7.5);});
  y+=101;
  section('Summary');y+=12;
  const kw=(CW-3*8)/4,kh=54;
  A.k.forEach(([cls,label,val,sub],i)=>{const cx=M+(i%4)*(kw+8),cy=y+Math.floor(i/4)*(kh+8);
    P.rect(cx,cy,kw,kh,'#ffffff',C.line,0.6);
    P.text(cx+8,cy+13,label,6.5,{bold:true,color:C.mut});
    P.text(cx+8,cy+29,val,12.5,{bold:true,color:cls==='bad'?C.red:cls==='warn'?C.warn:cls==='ok'?C.ok:cls==='live'?C.acc:C.txt});
    P.wrap(cx+8,cy+40,sub,6.5,kw-14,8,{color:C.mut});});
  y+=Math.ceil(A.k.length/4)*(kh+8)+10;
  section('Level history, last 7 days');y+=12;levelChart(M,y,CW,135,168);y+=165;
  const half=(CW-20)/2;
  section('Motor run time per day',M,half);section('Water used per day',M+half+20,half);y+=10;
  bars(M,y,half,120,A.dl,A.motor,v=>Math.round(v)+'m');
  bars(M+half+20,y,half,120,A.dl,A.used,v=>l?Math.round(v*l)+' L':(v/spanMm()*100).toFixed(0)+'%');
  y+=132;
  P.text(M,y,`Total ${dur(A.motor.reduce((a,b)=>a+b,0))} in ${A.af.filter(f=>f.s>=now-7*DAY).length} fills`,7,{color:C.mut});
  P.text(M+half+20,y,`Total ${amt(A.used.reduce((a,b)=>a+b,0))}, average ${amt(A.used.reduce((a,b)=>a+b,0)/7)} per day`,7,{color:C.mut});

  // ================= Page 2 =================
  header();
  section('Usage by hour of day');y+=10;
  bars(M,y,CW,130,A.avgH.map((_,i)=>pad(i)),A.avgH,v=>amt(v),new Set(A.top),2);y+=138;
  const totH=A.avgH.reduce((a,b)=>a+b,0);
  y=P.wrap(M,y,A.top.length?`Average per day over ${A.dayCount} day${A.dayCount>1?'s':''}. Busiest hours: `+
    A.top.map(i=>`${hh(i)}-${hh((i+1)%24)} (${Math.round(A.avgH[i]/totH*100)}%)`).join(', ')+'.':'Not enough usage recorded yet.',7.5,CW,10,{color:C.mut})+10;

  section('Night leak check');y+=8;
  const nc={bad:C.red,warn:C.warn,ok:C.ok};
  table(M,CW,[['Night',0.3],['Change',0.2,'right'],['Per hour',0.2,'right'],['Result',0.3]],
    A.nights.map(n=>[dayLbl(n.end,true),n.drop===null?'-':(n.drop>0?'-':'+')+amt(Math.abs(n.drop)),n.drop===null?'-':amt(Math.abs(n.drop)/n.hours),n.text]),
    A.nights.map(n=>[null,null,null,nc[n.status]||null]));
  y=P.wrap(M,y+8,`Level change between ${hh(st.nightStart)} and ${hh(st.nightEnd)}. Possible leak: a steady drop of ${st.leakCm} cm${l?' ('+Math.round(st.leakCm*10*l)+' L)':''} or more with no fill running. Water used (one drop): most of the drop within one hour, like a tap or flush.`,7,CW,9,{color:C.mut})+12;

  need(60);section('Recent fills (motor runs)');y+=8;
  const fl=A.done.slice(-10).reverse();
  if(fl.length)table(M,CW,[['Started',0.38],['Duration',0.2,'right'],['Added',0.2,'right'],['Rise speed',0.22,'right']],
    fl.map(f=>[fmtDT(f.s),dur((f.e-f.s)/60),amt(f.smm-f.emm,true),((f.smm-f.emm)/10/((f.e-f.s)/60)).toFixed(2)+' cm/min']));
  else{P.text(M,y+10,'No fills recorded yet.',8,{color:C.mut});y+=16;}
  y+=14;
  need(90);section('How these numbers are calculated');y+=14;
  ['Level is measured by the ultrasonic sensor and recorded every 2 minutes.',
   `Filling (motor on): the level rises faster than ${st.fillCm} cm/min. Fills under 3 minutes or 3 cm are ignored. Every fill is counted as the motor.`,
   'Water used: level drops outside fills, ignoring changes smaller than the sensor noise.',
   l?`Litres use the tank capacity of ${st.capacity.toLocaleString()} L and assume straight tank walls.`:'Set the tank capacity in Analytics settings to see litres instead of %.']
   .forEach(t=>{P.text(M,y,'\u2022',8,{color:C.acc});y=P.wrap(M+10,y,t,7.5,CW-10,10,{color:C.txt})+2;});

  // ---- footers on every page ----
  P.pages.forEach((c,i)=>{P.c=c;
    P.line(M,800,P.W-M,800,C.line,0.6);
    P.text(M,812,`\u00a9 ${BRAND.year} ${BRAND.owner}. All Rights Reserved.`,7,{color:C.txt});
    P.text(P.W/2,812,BRAND.site,7,{align:'center',color:C.brand,bold:true});
    P.text(P.W-M,812,`Page ${i+1} of ${P.pages.length}`,7,{align:'right',color:C.mut});
    P.text(P.W/2,824,`Water Tanks Monitor System, firmware ${st.fw}. Open source under GPL-3.0: ${BRAND.repo}`,6,{align:'center',color:C.mut});});
  return P.blob({title:'Tank Analytics Report'+(tankT?' - '+tankT:''),author:BRAND.owner,subject:`${siteLine(' / ')||'Water Tanks Monitor System'}, ${period}`,creator:BRAND.site});
}

async function exportPdf(){
  const b=$('btnPdf');b.disabled=true;b.textContent='Building report…';
  try{
    if(!aLoaded)await loadAnalytics();
    if(!ana||!st){toast('No analytics data yet',true);return;}
    const blob=buildReport(await logoJpeg()),n=new Date();
    const a=document.createElement('a');a.href=URL.createObjectURL(blob);
    a.download=`wtms-report-${fileTag()}${n.getFullYear()}${pad(n.getMonth()+1)}${pad(n.getDate())}.pdf`;
    document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},1500);
    toast('PDF report downloaded');
  }catch(e){console.error(e);toast('Could not build the report',true);}
  finally{b.disabled=false;b.textContent='Download PDF report';}
}

poll().then(()=>{showTab(location.hash.slice(1));setTimeout(loadAnalytics,3000);});
(function tick(){setTimeout(async()=>{if(!document.hidden)await poll();tick();},st&&st.perfMode===0?2000:1000);})();
setInterval(()=>{if(!document.hidden&&tab==='log'&&$('logAuto').checked)loadLog();},5000);
setInterval(()=>{if(!document.hidden&&(tab==='analytics'||tab==='overview'))loadAnalytics();},120000);
</script>
</body>
</html>
)rawliteral";
