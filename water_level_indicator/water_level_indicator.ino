/*
  Water Tanks Monitor System - main firmware (ESP32 / ESP8266)
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

  Source: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard
  SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
  Water Tanks Monitor System (WTMS)                        firmware 2.8.1
  ESP32 DevKit or ESP8266 NodeMCU + AJ-SR04M / JSN-SR04T + WS2812B strip (1-300 LEDs, set in the dashboard)

  - Each device is named in the dashboard: society / organisation, building, tank.
    The names appear on the dashboard and PDF report, and set the hotspot name
    ("WTMS <building> <tank>") and web address (http://tank-<building>-<tank>.local),
    so several tanks can run side by side.
  - No home Wi-Fi saved: opens the hotspot (default "WTMS-XXXX", XXXX = chip ID).
    Connect to it and the dashboard opens automatically (captive portal).
  - Wi-Fi is set up from the dashboard (Wi-Fi settings: scan, connect, forget).
  - Connected to home Wi-Fi: http://<hostname>.local or the device IP.
    The hotspot turns off 30 s after a successful connection and comes back
    automatically if the home network can't be reached.
  - Calibration, settings and Wi-Fi are saved in flash.
  - Connectivity log (restarts, Wi-Fi, hotspot, sensor) kept in flash for debugging.
  - Tank analytics: 14-day level history, fill (motor run) detection, consumption,
    night leak check. Recorded in flash, analysed in the dashboard.
    Needs a flash layout with a filesystem: Tools > Flash Size "4MB (FS:2MB ...)"
    on ESP8266, or any default partition scheme on ESP32.

  Library (Library Manager): Adafruit NeoPixel
  Wi-Fi, web server, DNS and mDNS come with the ESP32 / ESP8266 board package.
*/

#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #include <esp_system.h>
  using WebServerT = WebServer;
  #define BOARD_NAME "ESP32"
  #define WIFI_OPEN  WIFI_AUTH_OPEN
  #define LED_PIN   16   // G16 -> 330R -> strip DIN
  // Sensor on 4 neighbouring pins: 3V3, GND, D15, D2 (one 4-pin connector).
  // D15 and D2 are boot strapping pins: TRIG must be on D15 (output) and ECHO on D2,
  // because the sensor's echo rests LOW, which keeps uploading over USB working.
  #define TRIG_PIN  15   // D15 -> sensor TRIG (AJ-SR04M: RX)
  #define ECHO_PIN   2   // D2  <- sensor ECHO (AJ-SR04M: TX), 3.3V only (sensor on 3V3)
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  using WebServerT = ESP8266WebServer;
  #define BOARD_NAME "ESP8266 NodeMCU"
  #define WIFI_OPEN  ENC_TYPE_NONE
  ADC_MODE(ADC_VCC);     // lets the ESP8266 measure its own supply voltage (A0 is not used)
  #define LED_PIN    4   // D2 -> 330R -> strip DIN
  #define TRIG_PIN  14   // D5 -> sensor TRIG (AJ-SR04M: RX)
  #define ECHO_PIN  12   // D6 <- sensor ECHO (AJ-SR04M: TX)
#else
  #error "Select an ESP32 or ESP8266 board in Tools > Board"
#endif

#include <DNSServer.h>
#include <time.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "dashboard_html.h"
#include "event_log.h"
#include "history.h"

#define FW_VERSION  "2.8.1"
#define PRODUCT     "Water Tanks Monitor System"
#define DEFAULT_LEDS 30
#define MAX_LEDS     300

// ================= Timing =================
const float         MAX_VALID_CM     = 450.0;   // longer echoes mean "no target found"
const unsigned long ECHO_TIMEOUT_US  = 27000;   // ~460 cm
const unsigned long PING_FAST_MS     = 70;      // performance mode: ~14 readings per second
const unsigned long PING_ECO_MS      = 500;     // power saving: 2 readings per second
const unsigned long AP_DEMAND_IDLE   = 600000;  // on-demand hotspot: off after 10 min without devices
const int           BOOT_BTN         = 0;       // BOOT button (ESP32) / FLASH button (NodeMCU), GPIO0
const int           MEDIAN_N         = 7;       // median of the last 7 readings
const int           FAIL_LIMIT       = 10;      // consecutive misses before "sensor fault"
const unsigned long CONNECT_TIMEOUT  = 20000;   // give up a Wi-Fi attempt after 20 s
const unsigned long RETRY_INTERVAL   = 60000;   // retry home Wi-Fi every 60 s
const unsigned long AP_OFF_DELAY     = 30000;   // hotspot stays 30 s after connecting
const unsigned long HEARTBEAT_MS     = 300000;  // status line in the log every 5 min

// ================= Saved settings (EEPROM offset 0, same layout as 2.0.0) =================
struct Settings {
  uint32_t magic;
  float    distEmpty;
  float    distFull;
  float    lowAlarm;
  uint16_t trigUs;
  uint8_t  brightness;
  uint8_t  reversed;
  uint8_t  colorByLevel;
  uint8_t  apMode;        // hotspot: 0 automatic, 1 always on, 2 on demand (BOOT button)
  uint16_t numLeds;      // added in 2.2.0; older saves read as invalid -> default
  // added in 2.4.0 (analytics)
  uint32_t capacityL;    // tank capacity in litres, 0 = unknown
  uint8_t  nightStart;   // night leak check window, local hours
  uint8_t  nightEnd;
  uint16_t leakMm;       // night drop that counts as a possible leak (mm)
  uint16_t fillMmMin;    // minimum rise speed that counts as filling (mm/min)
  // added in 2.8.0 (stored in former padding bytes, which read as 0 = the production defaults)
  uint8_t  perfMode;     // 0 power saving (production), 1 performance
  uint8_t  txLevel;      // Wi-Fi transmit power: 0 medium, 1 high, 2 low
};
const uint32_t SETTINGS_MAGIC = 0x59544B32;   // "YTK2"
Settings cfg;

// ================= Saved Wi-Fi (EEPROM offset 64) =================
struct WifiCreds {
  uint32_t magic;
  char     ssid[33];
  char     pass[65];
};
const uint32_t WIFI_MAGIC = 0x59574631;       // "YWF1"
const int      WIFI_ADDR  = 64;
WifiCreds creds;

// ================= Boot counter (EEPROM offset 200) =================
struct BootInfo { uint32_t magic; uint32_t count; };
const uint32_t BOOT_MAGIC = 0x59424F54;       // "YBOT"
const int      BOOT_ADDR  = 200;
uint32_t bootCount = 0;

// ================= Site identity (EEPROM offset 256) =================
struct Identity {
  uint32_t magic;
  char     org[49];       // society / organisation
  char     building[33];
  char     tank[33];
};
const uint32_t ID_MAGIC = 0x5754494E;         // "WTIN"
const int      ID_ADDR  = 256;
Identity ident;
String apName, hostName;                       // derived from the identity at boot

String chipSuffix() {
#if defined(ESP8266)
  uint32_t id = ESP.getChipId();
#else
  uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 24);
#endif
  char b[5];
  snprintf(b, sizeof(b), "%04X", (unsigned)(id & 0xFFFF));
  return String(b);
}

// "Tower A / Tank 1" -> "tower-a-tank-1"
String slug(const String& in) {
  String o;
  for (unsigned int i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (ok) o += c;
    else if (o.length() && o[o.length() - 1] != '-') o += '-';
  }
  while (o.length() && o[o.length() - 1] == '-') o.remove(o.length() - 1);
  return o;
}

void deriveNames(String& apOut, String& hostOut) {
  String b = ident.building, t = ident.tank;
  b.trim(); t.trim();
  if (b.length() || t.length()) {
    apOut = "WTMS";
    if (b.length()) apOut += " " + b;
    if (t.length()) apOut += " " + t;
    if (apOut.length() > 32) apOut = apOut.substring(0, 32);
    apOut.trim();
    hostOut = "tank";
    String sb = slug(b), st = slug(t);
    if (sb.length()) hostOut += "-" + sb;
    if (st.length()) hostOut += "-" + st;
    if (hostOut.length() > 48) hostOut = hostOut.substring(0, 48);
    while (hostOut.endsWith("-")) hostOut.remove(hostOut.length() - 1);
  } else {
    apOut = "WTMS-" + chipSuffix();
    hostOut = "watertank-" + chipSuffix();
    hostOut.toLowerCase();
  }
}

// ================= Tank profile (EEPROM offset 384) =================
// Usage type, where the tank is, its shape and size. Used by the dashboard for
// calibration from dimensions, litres, recommended settings and the PDF report.
struct TankProfile {
  uint32_t magic;
  uint8_t  usage;         // 0 domestic, 1 commercial
  uint8_t  location;      // 0 overhead (roof), 1 loft / bathroom, 2 underground sump, 3 other
  uint8_t  shape;         // 0 rectangular, 1 vertical cylinder
  uint8_t  preset;        // 0 custom, 1.. preset index in the dashboard
  uint16_t lengthMm, widthMm, heightMm, diaMm;
  uint16_t depthMm;       // water depth when full (bottom -> full / overflow line)
  uint16_t gapMm;         // sensor face -> full water line
};
const uint32_t PROF_MAGIC = 0x5754504B;       // "WTPK"
const int      PROF_ADDR  = 384;
TankProfile prof;

void saveProfile() { prof.magic = PROF_MAGIC; EEPROM.put(PROF_ADDR, prof); EEPROM.commit(); }

void saveIdentity() { ident.magic = ID_MAGIC; EEPROM.put(ID_ADDR, ident); EEPROM.commit(); }

// Keep printable characters only, trimmed, max `size-1` bytes
void setIdField(char* dst, size_t size, String v) {
  v.trim();
  String o;
  for (unsigned int i = 0; i < v.length() && o.length() < size - 1; i++) {
    uint8_t c = v[i];
    if (c >= 0x20 && c != 0x7F && c != '"' && c != '\\') o += (char)c;
  }
  memset(dst, 0, size);
  strncpy(dst, o.c_str(), size - 1);
}

Adafruit_NeoPixel strip(DEFAULT_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);  // length set from settings
WebServerT server(80);
DNSServer  dns;

// Sensor state
float samples[MEDIAN_N];
int   sampleCount = 0, sampleIdx = 0, failStreak = 0;
float currentDistance = -1;
bool  sensorOK = false;
float smoothedLevel = -1;
bool  blinkState = false;

// Wi-Fi state
enum NetState { NET_NONE, NET_CONNECTING, NET_CONNECTED, NET_FAILED };
NetState net = NET_NONE;
bool   apActive = false, mdnsStarted = false;
String trySsid, tryPass, pendingSsid, pendingPass, netMsg;
bool   tryIsNew = false;
bool   bgRetry = false;          // current attempt is a background retry of the saved network
unsigned long connectStart = 0, lastRetry = 0, apOffAt = 0;
unsigned long pendingConnectAt = 0, pendingForgetAt = 0, restartAt = 0;
String resetReason;
bool   resetIsProblem = false;

// Diagnostics
uint32_t minHeap = 0xFFFFFFFF;
unsigned long loopMax = 0, lastLoopAt = 0;
uint8_t lastChannel = 0;
uint16_t lastStaReason = 0;
unsigned long lastStaReasonAt = 0;
bool lastSensorOK = true;

// Wi-Fi driver events arrive from another context: queue them, log them from loop()
enum { EV_AP_JOIN = 1, EV_AP_LEAVE, EV_STA_DISC };
struct NetEvt { uint8_t type; uint8_t mac[6]; uint16_t reason; };
NetEvt evQ[16];
volatile uint8_t evHead = 0, evTail = 0;
#if defined(ESP8266)
WiFiEventHandler hApJoin, hApLeave, hStaDisc;
#endif

void pushEvt(uint8_t type, const uint8_t* mac, uint16_t reason) {
  uint8_t next = (evHead + 1) % 16;
  if (next == evTail) return;                   // queue full: drop
  evQ[evHead].type = type;
  if (mac) memcpy(evQ[evHead].mac, mac, 6); else memset(evQ[evHead].mac, 0, 6);
  evQ[evHead].reason = reason;
  evHead = next;
}

String macStr(const uint8_t* m) {
  char b[18];
  snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(b);
}

const char* staReasonText(uint16_t r) {
  switch (r) {
    case 2:   return "authentication expired";
    case 3:   return "router ended the session";
    case 4:   return "association expired";
    case 8:   return "disconnected by this device";
    case 15:
    case 204: return "handshake timeout (wrong password?)";
    case 200: return "signal lost (beacon timeout)";
    case 201: return "network not found";
    case 202: return "authentication failed (wrong password?)";
    case 203: return "association failed";
    case 205: return "connection failed";
    default:  return "other";
  }
}

uint32_t supplyMv() {
#if defined(ESP8266)
  return ESP.getVcc();
#else
  return 0;
#endif
}

// ================= Storage =================
void setAnalyticsDefaults();
void setDefaults() {
  cfg.magic = SETTINGS_MAGIC;
  cfg.distEmpty = 120.0;  cfg.distFull = 25.0;  cfg.lowAlarm = 0.10;
  cfg.trigUs = 100;       cfg.brightness = 150;
  cfg.reversed = 0;       cfg.colorByLevel = 0;  cfg.apMode = 0;
  cfg.perfMode = 0;       cfg.txLevel = 0;
  cfg.numLeds = DEFAULT_LEDS;
  setAnalyticsDefaults();
}

void setAnalyticsDefaults() {
  cfg.capacityL = 0;  cfg.nightStart = 1;  cfg.nightEnd = 5;
  cfg.leakMm = 15;    cfg.fillMmMin = 5;
}

void saveSettings() { EEPROM.put(0, cfg); EEPROM.commit(); }
void saveCreds()    { EEPROM.put(WIFI_ADDR, creds); EEPROM.commit(); }
bool haveCreds()    { return creds.magic == WIFI_MAGIC && creds.ssid[0] != 0; }

void loadStorage() {
  EEPROM.begin(512);
  EEPROM.get(0, cfg);
  if (cfg.magic != SETTINGS_MAGIC || isnan(cfg.distEmpty) || isnan(cfg.distFull) ||
      isnan(cfg.lowAlarm) || cfg.distEmpty - cfg.distFull < 10) {
    setDefaults();
    saveSettings();
  }
  if (cfg.numLeds < 1 || cfg.numLeds > MAX_LEDS) {   // settings saved by 2.1.0 or older
    cfg.numLeds = DEFAULT_LEDS;
    saveSettings();
  }
  if (cfg.apMode > 2 || cfg.perfMode > 1 || cfg.txLevel > 2) {
    if (cfg.apMode > 2) cfg.apMode = 0;
    if (cfg.perfMode > 1) cfg.perfMode = 0;
    if (cfg.txLevel > 2) cfg.txLevel = 0;
    saveSettings();
  }
  if (cfg.capacityL > 1000000 || cfg.nightStart > 23 || cfg.nightEnd > 23 || cfg.nightStart == cfg.nightEnd ||
      cfg.leakMm < 2 || cfg.leakMm > 500 || cfg.fillMmMin < 1 || cfg.fillMmMin > 200) {   // saved by 2.3.x or older
    setAnalyticsDefaults();
    saveSettings();
  }

  BootInfo bi;
  EEPROM.get(BOOT_ADDR, bi);
  if (bi.magic != BOOT_MAGIC) { bi.magic = BOOT_MAGIC; bi.count = 0; }
  bi.count++;
  bootCount = bi.count;
  EEPROM.put(BOOT_ADDR, bi);
  EEPROM.commit();
  EEPROM.get(WIFI_ADDR, creds);
  if (creds.magic != WIFI_MAGIC) memset(&creds, 0, sizeof(creds));
  creds.ssid[32] = 0;
  creds.pass[64] = 0;
  EEPROM.get(ID_ADDR, ident);
  if (ident.magic != ID_MAGIC) memset(&ident, 0, sizeof(ident));
  ident.org[48] = 0; ident.building[32] = 0; ident.tank[32] = 0;
  EEPROM.get(PROF_ADDR, prof);
  if (prof.magic != PROF_MAGIC || prof.usage > 1 || prof.location > 3 || prof.shape > 1) {
    memset(&prof, 0, sizeof(prof));
  }
  deriveNames(apName, hostName);
}

// ================= Ultrasonic (non-blocking) =================
float readDistanceOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(cfg.trigUs);         // AJ-SR04M ignores 10-20 us triggers
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (us == 0) return -1;
  float cm = us * 0.0343 / 2.0;
  return cm > MAX_VALID_CM ? -1 : cm;
}

float medianOfSamples() {
  float v[MEDIAN_N];
  for (int i = 0; i < sampleCount; i++) v[i] = samples[i];
  for (int i = 1; i < sampleCount; i++) {
    float k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) { v[j + 1] = v[j]; j--; }
    v[j + 1] = k;
  }
  return v[sampleCount / 2];
}

void sensorTask() {
  static unsigned long last = 0;
  if (millis() - last < (cfg.perfMode ? PING_FAST_MS : PING_ECO_MS)) return;
  last = millis();

  float d = readDistanceOnce();
  if (d > 0) {
    samples[sampleIdx] = d;
    sampleIdx = (sampleIdx + 1) % MEDIAN_N;
    if (sampleCount < MEDIAN_N) sampleCount++;
    failStreak = 0;
  } else if (failStreak < 1000) {
    failStreak++;
  }
  if (failStreak >= FAIL_LIMIT) {
    sensorOK = false; sampleCount = 0; sampleIdx = 0; currentDistance = -1;
  } else if (sampleCount >= 3) {
    currentDistance = medianOfSamples();
    sensorOK = true;
  }
  // Log sensor fault / recovery, at most once per 30 s so a loose wire can't flood the log
  static unsigned long lastSensorLog = 0;
  static int hidden = 0;
  if (sensorOK != lastSensorOK) {
    lastSensorOK = sensorOK;
    if (millis() - lastSensorLog < 30000 && lastSensorLog) { hidden++; return; }
    lastSensorLog = millis();
    String extra = hidden ? " (" + String(hidden) + " changes in between not logged)" : String("");
    hidden = 0;
    if (sensorOK) logWrite('I', "sensor", "Sensor OK: " + String(currentDistance, 1) + " cm" + extra);
    else          logWrite('W', "sensor", "Sensor fault: no echo" + extra);
  }
}

float levelFromDistance(float d) {
  float level = (cfg.distEmpty - d) / (cfg.distEmpty - cfg.distFull);
  return constrain(level, 0.0f, 1.0f);
}

// ================= LEDs =================
uint32_t redToGreen(float t, float dim = 1.0) {
  t = constrain(t, 0.0f, 1.0f);
  uint8_t r, g;
  if (t < 0.5) { r = 255; g = (uint8_t)(t * 2 * 255); }
  else         { r = (uint8_t)((1 - t) * 2 * 255); g = 255; }
  return strip.Color(r * dim, g * dim, 0);
}

// Position of LED i along the strip: 0.0 at the bottom, 1.0 at the top
float posT(int i) { return cfg.numLeds > 1 ? (float)i / (cfg.numLeds - 1) : 1.0f; }

void setLed(int posFromBottom, uint32_t c) {
  int idx = cfg.reversed ? (cfg.numLeds - 1 - posFromBottom) : posFromBottom;
  strip.setPixelColor(idx, c);
}

void drawLevel(float level) {
  float litF = level * cfg.numLeds;
  int fullLeds = (int)litF;
  float partial = litF - fullLeds;
  for (int i = 0; i < cfg.numLeds; i++) {
    float t = cfg.colorByLevel ? level : posT(i);
    if (i < fullLeds)                          setLed(i, redToGreen(t));
    else if (i == fullLeds && partial > 0.05)  setLed(i, redToGreen(t, partial));
  }
  if (level < cfg.lowAlarm)
    for (int i = 0; i < 3 && i < cfg.numLeds; i++) setLed(i, blinkState ? strip.Color(255, 0, 0) : 0);
}

bool ledsForce = true;                  // redraw even if nothing changed (after settings changes)
void displayTask() {
  static unsigned long last = 0;
  static uint8_t lastPix[MAX_LEDS * 3];
  if (millis() - last < (cfg.perfMode ? 100UL : 250UL)) return;
  last = millis();
  blinkState = (millis() / 500) % 2;

  strip.clear();
  if (sensorOK) {
    float level = levelFromDistance(currentDistance);
    smoothedLevel = smoothedLevel < 0 ? level : smoothedLevel + (level - smoothedLevel) * 0.25;
    drawLevel(smoothedLevel);
  } else {
    setLed(0, blinkState ? strip.Color(0, 0, 255) : 0);        // sensor fault
  }
  if (apActive && !blinkState) setLed(cfg.numLeds - 1, strip.Color(0, 0, 160));  // hotspot on
  // Only send data to the strip when a pixel actually changed
  size_t n = (size_t)cfg.numLeds * 3;
  const uint8_t* px = strip.getPixels();
  if (!ledsForce && memcmp(px, lastPix, n) == 0) return;
  memcpy(lastPix, px, n);
  ledsForce = false;
  strip.show();
}

// ================= Power =================
// Power saving (default): CPU 80 MHz, fewer sensor reads and LED refreshes,
// Wi-Fi modem sleep while the hotspot is off. Transmit power is chosen separately.
const char* const TX_NAMES[] = {"medium (13 dBm)", "high (19.5 dBm)", "low (8.5 dBm)"};
void applyRadio() {
  if (WiFi.getMode() == WIFI_OFF) return;
  bool staOnly = !apActive;
#if defined(ESP32)
  static const wifi_power_t P[] = {WIFI_POWER_13dBm, WIFI_POWER_19_5dBm, WIFI_POWER_8_5dBm};
  WiFi.setTxPower(P[cfg.txLevel]);
  WiFi.setSleep(cfg.perfMode == 0 && staOnly);        // modem sleep only works without the hotspot
#else
  static const float P[] = {13.0f, 20.5f, 8.5f};
  WiFi.setOutputPower(P[cfg.txLevel]);
  WiFi.setSleepMode(cfg.perfMode == 0 && staOnly ? WIFI_MODEM_SLEEP : WIFI_NONE_SLEEP);
#endif
}
void applyPower() {
#if defined(ESP32)
  setCpuFrequencyMhz(cfg.perfMode ? 240 : 80);        // Wi-Fi works down to 80 MHz
#endif
  applyRadio();
  ledsForce = true;
}
uint32_t cpuMhz() {
#if defined(ESP32)
  return getCpuFrequencyMhz();
#else
  return ESP.getCpuFreqMHz();
#endif
}

// ================= Wi-Fi =================
const char* netStateName();
unsigned long apIdleSince = 0;
void startAP(const char* why) {
  if (apActive) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str());
  delay(100);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());         // every name -> dashboard (captive portal)
  apActive = true;
  apOffAt = 0;
  apIdleSince = millis();
  applyRadio();
  lastChannel = WiFi.channel();
  logWrite('I', "ap", String("Hotspot on (") + why + "), channel " + String((int)lastChannel));
}

void stopAP(const char* why) {
  if (!apActive) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(haveCreds() ? WIFI_STA : WIFI_OFF);    // no home network: switch the radio off completely
  apActive = false;
  apOffAt = 0;
  applyRadio();
  logWrite('I', "ap", String("Hotspot off (") + why + ")");
}

void beginConnect(const String& ssid, const String& pass, bool isNew) {
  trySsid = ssid;
  tryPass = pass;
  tryIsNew = isNew;
  if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  WiFi.begin(ssid.c_str(), pass.length() ? pass.c_str() : nullptr);
  applyRadio();
  net = NET_CONNECTING;
  connectStart = millis();
  netMsg = "Connecting to " + ssid + "...";
  logWrite('I', "wifi", String("Connecting to ") + ssid + (isNew ? " (new network from dashboard)" : ""));
}

void onConnected() {
  net = NET_CONNECTED;
  if (tryIsNew) {                               // save only networks that worked
    memset(&creds, 0, sizeof(creds));
    creds.magic = WIFI_MAGIC;
    strncpy(creds.ssid, trySsid.c_str(), 32);
    strncpy(creds.pass, tryPass.c_str(), 64);
    saveCreds();
    tryIsNew = false;
  }
  netMsg = "Connected to " + WiFi.SSID();
  if (apActive && cfg.apMode != 1) apOffAt = millis() + AP_OFF_DELAY;
  if (!mdnsStarted && MDNS.begin(hostName.c_str())) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
  configTime(0, 0, "pool.ntp.org", "time.google.com");   // real clock for the log
  logWrite('I', "wifi", "Connected to " + WiFi.SSID() + ", IP " + WiFi.localIP().toString() +
                        ", signal " + String((int)WiFi.RSSI()) + " dBm, channel " + String((int)WiFi.channel()) +
                        " (" + String((millis() - connectStart) / 1000.0, 1) + " s)");
}

void onConnectFailed() {
  WiFi.disconnect(false);                       // stop channel hopping so the hotspot stays stable
  if (tryIsNew) netMsg = "Could not connect to " + trySsid + ". Check the password and that the network is in range.";
  else          netMsg = "Home Wi-Fi " + String(creds.ssid) + " not reachable. Retrying every minute.";
  tryIsNew = false;
  net = NET_FAILED;
  lastRetry = millis();
  logWrite('W', "wifi", "Could not connect to " + trySsid + " in " + String(CONNECT_TIMEOUT / 1000) +
                        " s. Last reason: " + staReasonText(lastStaReason) + " (" + String(lastStaReason) + ")");
  if (cfg.apMode == 2 && !apActive)
    logWrite('I', "ap", "Hotspot stays off (on-demand mode). Press the BOOT button to turn it on.");
  else
    startAP("home Wi-Fi not reachable");
}

void wifiTask() {
  unsigned long now = millis();
  if (apActive) dns.processNextRequest();

  if (pendingConnectAt && now >= pendingConnectAt) {      // requested from the dashboard
    pendingConnectAt = 0;
    startAP("connecting to a new network");               // keep a way back if it fails
    beginConnect(pendingSsid, pendingPass, true);
  }
  if (pendingForgetAt && now >= pendingForgetAt) {
    pendingForgetAt = 0;
    memset(&creds, 0, sizeof(creds));
    saveCreds();
    WiFi.disconnect(false);
    net = NET_NONE;
    netMsg = "Wi-Fi forgotten. Choose a network to connect.";
    logWrite('I', "wifi", "Saved Wi-Fi forgotten from dashboard");
    startAP("Wi-Fi forgotten");
  }

  switch (net) {
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        bgRetry = false;
        onConnected();
      } else if (bgRetry && apActive && WiFi.softAPgetStationNum() > 0) {
        // Someone joined the hotspot during a background retry. The retry makes the ESP
        // search other channels, which stalls hotspot traffic: stop it and serve the user.
        bgRetry = false;
        WiFi.disconnect(false);
        net = NET_FAILED;
        lastRetry = now;
        netMsg = "Home Wi-Fi " + String(creds.ssid) + " not reachable. Retrying when nobody is on the hotspot.";
        logWrite('I', "wifi", "Background retry stopped: a device joined the hotspot");
      } else if (now - connectStart > CONNECT_TIMEOUT) {
        bgRetry = false;
        onConnectFailed();
      }
      break;
    case NET_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        logWrite('W', "wifi", String("Home Wi-Fi lost: ") + staReasonText(lastStaReason) + " (" + String(lastStaReason) + ")");
        beginConnect(creds.ssid, creds.pass, false);
      }
      break;
    case NET_FAILED:                              // retry only when nobody uses the hotspot
      if (haveCreds() && now - lastRetry > RETRY_INTERVAL && WiFi.softAPgetStationNum() == 0) {
        lastRetry = now;
        beginConnect(creds.ssid, creds.pass, false);
        bgRetry = true;
      }
      break;
    case NET_NONE:
      break;
  }

  if (apActive && apOffAt && now >= apOffAt && net == NET_CONNECTED && cfg.apMode != 1)
    stopAP("connected to home Wi-Fi");
  if (cfg.apMode == 1 && !apActive) startAP("always-on setting");
  if (apActive && cfg.apMode == 2) {                    // on demand: off after 10 min without devices
    if (WiFi.softAPgetStationNum() > 0) apIdleSince = now;
    else if (now - apIdleSince > AP_DEMAND_IDLE) stopAP("on-demand mode, no devices for 10 min");
  }

  // Channel changes kick hotspot users off (ESP8266 hotspot follows the home Wi-Fi channel)
  if (apActive) {
    uint8_t ch = WiFi.channel();
    if (ch && lastChannel && ch != lastChannel)
      logWrite('W', "ap", "Hotspot channel changed " + String(lastChannel) + " -> " + String(ch) +
                          ": devices on the hotspot are disconnected briefly");
    if (ch) lastChannel = ch;
  }
}

// BOOT / FLASH button: turns the hotspot on (on-demand mode, or any time it is off)
void buttonTask() {
  static int last = HIGH;
  static unsigned long changedAt = 0;
  static bool handled = false;
  int v = digitalRead(BOOT_BTN);
  if (v != last) { last = v; changedAt = millis(); handled = false; }
  if (v == LOW && !handled && millis() - changedAt > 50) {
    handled = true;
    if (!apActive) startAP("BOOT button pressed");
    else logWrite('I', "ap", "BOOT button: hotspot kept on for another 10 min");
    apIdleSince = millis();
    if (net == NET_CONNECTED && cfg.apMode != 1) apOffAt = millis() + AP_DEMAND_IDLE;
  }
}

// Log queued Wi-Fi driver events
void eventTask() {
  while (evTail != evHead) {
    NetEvt e = evQ[evTail];
    evTail = (evTail + 1) % 16;
    if (e.type == EV_AP_JOIN) {
      logWrite('I', "ap", "Device joined hotspot: " + macStr(e.mac) + " (" + String((int)WiFi.softAPgetStationNum()) + " connected)");
    } else if (e.type == EV_AP_LEAVE) {
      logWrite('I', "ap", "Device left hotspot: " + macStr(e.mac) + " (" + String((int)WiFi.softAPgetStationNum()) + " connected)");
    } else if (e.type == EV_STA_DISC) {
      // The driver repeats this every few seconds while retrying: log changes, or once a minute
      if (e.reason != lastStaReason || millis() - lastStaReasonAt > 60000) {
        if (net == NET_CONNECTED || e.reason != 8)
          logWrite('W', "wifi", String("Home Wi-Fi disconnect event: ") + staReasonText(e.reason) + " (" + String(e.reason) + ")");
        lastStaReasonAt = millis();
      }
      lastStaReason = e.reason;
    }
  }
}

// Periodic health line, clock sync, loop stall detection
void diagTask() {
  unsigned long now = millis();
  unsigned long dt = now - lastLoopAt;
  lastLoopAt = now;
  if (dt > loopMax) loopMax = dt;
  if (dt > 2000) logWrite('W', "sys", "Main loop was blocked for " + String(dt) + " ms");

  static unsigned long lastSlow = 0, lastBeat = 0;
  if (now - lastSlow < 1000) return;
  lastSlow = now;
  uint32_t heap = ESP.getFreeHeap();
  if (heap < minHeap) minHeap = heap;

  static unsigned long lastNtpCheck = 0;
  if (net == NET_CONNECTED && (!logEpochBase || now - lastNtpCheck > 600000)) {
    lastNtpCheck = now;
    time_t t = time(nullptr);
    if (t > 1700000000) {
      long diff = (long)t - (long)nowEpoch();
      if (!logEpochBase) { setClock((uint32_t)t); logWrite('I', "sys", "Clock synced from internet"); }
      else if (diff > 2 || diff < -2) {
        setClock((uint32_t)t);
        if (diff > 60 || diff < -60) logWrite('I', "sys", "Clock corrected by " + String(diff) + " s");
      }
    }
  }

  if (now - lastBeat >= HEARTBEAT_MS) {
    lastBeat = now;
    uint32_t mv = supplyMv();
    bool bad = heap < 8000 || loopMax > 1000 || (mv && mv < 3000);
    String m = "Health: free memory " + String(heap) + " B (lowest " + String(minHeap) + " B)";
#if defined(ESP8266)
    m += ", fragmentation " + String(ESP.getHeapFragmentation()) + "%";
#endif
    if (mv) m += ", supply " + String(mv / 1000.0, 2) + " V";
    m += ", slowest loop " + String(loopMax) + " ms";
    if (apActive) m += ", hotspot on (" + String((int)WiFi.softAPgetStationNum()) + " devices)";
    else          m += ", hotspot off";
    if (net == NET_CONNECTED) m += ", home Wi-Fi " + String((int)WiFi.RSSI()) + " dBm ch " + String((int)WiFi.channel());
    else                      m += String(", home Wi-Fi ") + netStateName();
    logWrite(bad ? 'W' : 'I', "sys", m);
    loopMax = 0;
  }
}

// ================= Web API =================
String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

void sendJson(int code, const String& j) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", j);
}

void sendResult(bool ok, const String& message) {
  sendJson(ok ? 200 : 400, String("{\"ok\":") + (ok ? "true" : "false") +
                           ",\"message\":\"" + jsonEscape(message) + "\"}");
}

void handleDashboard() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

// Captive portal: any unknown URL (phone/PC connectivity checks) goes to the dashboard
void handleNotFound() {
  IPAddress ip = apActive ? WiFi.softAPIP() : WiFi.localIP();
  server.sendHeader("Location", String("http://") + ip.toString() + "/", true);
  server.send(302, "text/plain", "");
}

const char* netStateName() {
  switch (net) {
    case NET_CONNECTING: return "connecting";
    case NET_CONNECTED:  return "connected";
    case NET_FAILED:     return "failed";
    default:             return "none";
  }
}

void handleStatus() {
  bool conn = net == NET_CONNECTED;
  unsigned long now = millis();
  String j;
  j.reserve(1200);
  j += "{\"fw\":\"" FW_VERSION "\",\"board\":\"" BOARD_NAME "\"";
  j += ",\"valid\":";        j += sensorOK ? "true" : "false";
  j += ",\"distance\":";     j += String(sensorOK ? currentDistance : 0, 1);
  j += ",\"level\":";        j += String(smoothedLevel < 0 ? 0 : smoothedLevel * 100, 1);
  j += ",\"empty\":";        j += String(cfg.distEmpty, 1);
  j += ",\"full\":";         j += String(cfg.distFull, 1);
  j += ",\"lowAlarm\":";     j += String((int)round(cfg.lowAlarm * 100));
  j += ",\"trigUs\":";       j += String(cfg.trigUs);
  j += ",\"brightness\":";   j += String(cfg.brightness);
  j += ",\"leds\":";         j += String(cfg.numLeds);
  j += ",\"reversed\":";     j += cfg.reversed ? "true" : "false";
  j += ",\"colorByLevel\":"; j += cfg.colorByLevel ? "true" : "false";
  j += ",\"uptime\":";       j += String(now / 1000);
  j += ",\"reset\":\"";      j += jsonEscape(resetReason);
  j += "\",\"boot\":";      j += String(bootCount);
  j += ",\"heap\":";         j += String(ESP.getFreeHeap());
  j += ",\"minHeap\":";      j += String(minHeap);
  j += ",\"vcc\":";          j += String(supplyMv());
  j += ",\"epoch\":";        j += String(nowEpoch());
  j += ",\"logStore\":\"";  j += logFsOK ? "flash" : "memory";
  j += "\",\"apAlways\":";  j += cfg.apMode == 1 ? "true" : "false";
  j += ",\"apMode\":";      j += String(cfg.apMode);
  j += ",\"perfMode\":";    j += String(cfg.perfMode);
  j += ",\"txLevel\":";     j += String(cfg.txLevel);
  j += ",\"cpuMhz\":";      j += String(cpuMhz());
  j += ",\"filling\":";      j += fd.filling ? "true" : "false";
  j += ",\"fillRate\":";     j += String(fd.filling ? fd.rateMmMin / 10.0 : 0.0, 2);
  j += ",\"fillStart\":";    j += String(fd.filling ? fd.startT : 0);
  j += ",\"fillEta\":";      j += String(fillEtaMin(sensorOK ? currentDistance : -1, cfg.distFull), 0);
  j += ",\"capacity\":";     j += String(cfg.capacityL);
  j += ",\"nightStart\":";   j += String(cfg.nightStart);
  j += ",\"nightEnd\":";     j += String(cfg.nightEnd);
  j += ",\"leakCm\":";       j += String(cfg.leakMm / 10.0, 1);
  j += ",\"fillCm\":";       j += String(cfg.fillMmMin / 10.0, 1);
  j += ",\"wifi\":{\"saved\":\""; j += jsonEscape(haveCreds() ? String(creds.ssid) : String(""));
  j += "\",\"state\":\"";    j += netStateName();
  j += "\",\"ssid\":\"";     j += jsonEscape(conn ? WiFi.SSID() : trySsid);
  j += "\",\"ip\":\"";       j += conn ? WiFi.localIP().toString() : String("");
  j += "\",\"rssi\":";       j += String(conn ? (int)WiFi.RSSI() : 0);
  j += ",\"msg\":\"";        j += jsonEscape(netMsg);
  j += "\",\"ap\":";         j += apActive ? "true" : "false";
  j += ",\"apName\":\""; j += jsonEscape(apName); j += "\",\"apIp\":\""; j += WiFi.softAPIP().toString();
  j += "\",\"apClients\":";  j += String(apActive ? (int)WiFi.softAPgetStationNum() : 0);
  j += ",\"apOffIn\":";      j += String(apActive && apOffAt > now ? (apOffAt - now) / 1000 + 1 : 0);
  j += ",\"host\":\""; j += hostName; j += "\"}";
  String nextAp, nextHost;
  deriveNames(nextAp, nextHost);                 // names that apply after the next restart
  j += ",\"profile\":{\"usage\":";  j += String(prof.usage);
  j += ",\"location\":";  j += String(prof.location);
  j += ",\"shape\":";     j += String(prof.shape);
  j += ",\"preset\":";    j += String(prof.preset);
  j += ",\"len\":";       j += String(prof.lengthMm);
  j += ",\"wid\":";       j += String(prof.widthMm);
  j += ",\"hgt\":";       j += String(prof.heightMm);
  j += ",\"dia\":";       j += String(prof.diaMm);
  j += ",\"depth\":";     j += String(prof.depthMm);
  j += ",\"gap\":";       j += String(prof.gapMm);
  j += "}";
  j += ",\"site\":{\"org\":\"";  j += jsonEscape(ident.org);
  j += "\",\"building\":\"";     j += jsonEscape(ident.building);
  j += "\",\"tank\":\"";         j += jsonEscape(ident.tank);
  j += "\",\"nextAp\":\"";       j += jsonEscape(nextAp);
  j += "\",\"nextHost\":\"";     j += nextHost;
  j += "\"}}";
  sendJson(200, j);
}

void handleCalibrate() {
  float e = cfg.distEmpty, f = cfg.distFull;
  String point = server.arg("point");
  if (point == "empty" || point == "full") {
    if (!sensorOK) return sendResult(false, "No valid sensor reading right now");
    if (point == "empty") e = currentDistance; else f = currentDistance;
  } else {
    if (server.hasArg("empty")) e = server.arg("empty").toFloat();
    if (server.hasArg("full"))  f = server.arg("full").toFloat();
  }
  if (f < 15)           return sendResult(false, "Full distance must be at least 15 cm (sensor blind zone)");
  if (e > MAX_VALID_CM) return sendResult(false, "Empty distance must be 450 cm or less");
  if (e - f < 10)       return sendResult(false, "Empty distance must be at least 10 cm more than Full");
  cfg.distEmpty = e;
  cfg.distFull = f;
  saveSettings();
  logWrite('I', "cfg", "Calibration saved: empty " + String(e, 1) + " cm, full " + String(f, 1) + " cm");
  smoothedLevel = -1;
  sendResult(true, "Calibration saved: empty " + String(e, 1) + " cm, full " + String(f, 1) + " cm");
}

void handleSettings() {
  bool defaults = server.hasArg("defaults");
  uint16_t oldLeds = cfg.numLeds;
  uint8_t oldAp = cfg.apMode, oldPerf = cfg.perfMode, oldTx = cfg.txLevel;
  uint32_t oldCap = cfg.capacityL;
  if (defaults) {
    setDefaults();
    cfg.numLeds = oldLeds;              // LED count describes the hardware, keep it
    cfg.apMode = oldAp;                 // Wi-Fi and power are not display settings
    cfg.perfMode = oldPerf;
    cfg.txLevel = oldTx;
    cfg.capacityL = oldCap;             // tank size is hardware too
  } else {
    if (server.hasArg("leds"))         cfg.numLeds      = constrain(server.arg("leds").toInt(), 1, MAX_LEDS);
    if (server.hasArg("capacity"))   cfg.capacityL  = constrain(server.arg("capacity").toInt(), 0, 1000000);
    if (server.hasArg("nightStart")) cfg.nightStart = constrain(server.arg("nightStart").toInt(), 0, 23);
    if (server.hasArg("nightEnd"))   cfg.nightEnd   = constrain(server.arg("nightEnd").toInt(), 0, 23);
    if (cfg.nightStart == cfg.nightEnd) cfg.nightEnd = (cfg.nightStart + 4) % 24;
    if (server.hasArg("leakCm"))     cfg.leakMm     = constrain((int)(server.arg("leakCm").toFloat() * 10 + 0.5), 2, 500);
    if (server.hasArg("fillCm"))     cfg.fillMmMin  = constrain((int)(server.arg("fillCm").toFloat() * 10 + 0.5), 1, 200);
    int apArg = server.hasArg("apMode") ? constrain(server.arg("apMode").toInt(), 0, 2)
              : server.hasArg("apAlways") ? (server.arg("apAlways").toInt() ? 1 : 0) : -1;
    if (apArg >= 0 && apArg != cfg.apMode) {
      static const char* const M[] = {"automatic", "always on", "on demand (BOOT button)"};
      cfg.apMode = apArg;
      logWrite('I', "cfg", String("Setting: hotspot ") + M[apArg]);
      if (apArg != 1 && net == NET_CONNECTED && apActive) apOffAt = millis() + 5000;
      if (apArg == 2) apIdleSince = millis();
    }
    bool power = false;
    if (server.hasArg("perf")) { uint8_t v = constrain(server.arg("perf").toInt(), 0, 1); power |= v != cfg.perfMode; cfg.perfMode = v; }
    if (server.hasArg("tx"))   { uint8_t v = constrain(server.arg("tx").toInt(), 0, 2);   power |= v != cfg.txLevel;  cfg.txLevel = v; }
    if (power) logWrite('I', "cfg", String("Setting: ") + (cfg.perfMode ? "performance mode" : "power saving") +
                                   ", transmit power " + TX_NAMES[cfg.txLevel]);
    if (server.hasArg("brightness"))   cfg.brightness   = constrain(server.arg("brightness").toInt(), 5, 255);
    if (server.hasArg("lowAlarm"))     cfg.lowAlarm     = constrain(server.arg("lowAlarm").toInt(), 0, 50) / 100.0;
    if (server.hasArg("reversed"))     cfg.reversed     = server.arg("reversed").toInt() ? 1 : 0;
    if (server.hasArg("colorByLevel")) cfg.colorByLevel = server.arg("colorByLevel").toInt() ? 1 : 0;
    if (server.hasArg("trigUs"))       cfg.trigUs       = constrain(server.arg("trigUs").toInt(), 10, 1000);
  }
  if (cfg.numLeds != oldLeds) {
    strip.clear();                      // switch off every LED of the old length first
    strip.show();
    strip.updateLength(cfg.numLeds);
  }
  strip.setBrightness(cfg.brightness);
  saveSettings();
  applyPower();
  sendResult(true, defaults ? "Settings reset to defaults" : "Settings saved");
}

void handleScan() {
  if (server.hasArg("start")) {
    if (!(WiFi.getMode() & WIFI_STA)) WiFi.mode(WIFI_AP_STA);   // scanning needs the station interface
    WiFi.scanDelete();
    WiFi.scanNetworks(true);                                     // async
    logWrite('I', "wifi", "Network scan started from dashboard");
    return sendJson(200, "{\"running\":true}");
  }
  int n = WiFi.scanComplete();
  if (n == -1) return sendJson(200, "{\"running\":true}");
  if (n < 0)   return sendJson(200, "{\"running\":false,\"networks\":[]}");

  // Strongest entry per SSID, sorted by signal
  int idx[40], m = 0;
  for (int i = 0; i < n && m < 40; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    int dup = -1;
    for (int k = 0; k < m; k++) if (WiFi.SSID(idx[k]) == s) { dup = k; break; }
    if (dup < 0) idx[m++] = i;
    else if (WiFi.RSSI(i) > WiFi.RSSI(idx[dup])) idx[dup] = i;
  }
  for (int a = 1; a < m; a++) {
    int k = idx[a], b = a - 1;
    while (b >= 0 && WiFi.RSSI(idx[b]) < WiFi.RSSI(k)) { idx[b + 1] = idx[b]; b--; }
    idx[b + 1] = k;
  }
  String j = "{\"running\":false,\"networks\":[";
  for (int a = 0; a < m; a++) {
    int i = idx[a];
    if (a) j += ",";
    j += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String((int)WiFi.RSSI(i)) +
         ",\"open\":" + (WiFi.encryptionType(i) == WIFI_OPEN ? "true" : "false") + "}";
  }
  j += "]}";
  sendJson(200, j);
}

void handleWifiConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0 || ssid.length() > 32)
    return sendResult(false, "Network name must be 1-32 characters");
  if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63))
    return sendResult(false, "Password must be 8-63 characters (leave empty for open networks)");
  pendingSsid = ssid;
  pendingPass = pass;
  pendingConnectAt = millis() + 500;           // reply first, then switch
  sendResult(true, "Connecting to " + ssid + "...");
}

void handleWifiForget() {
  pendingForgetAt = millis() + 500;
  sendResult(true, "Forgetting Wi-Fi. The hotspot stays on.");
}

void handleRestart() {
  logWrite('I', "sys", "Restart requested from dashboard");
  restartAt = millis() + 1000;
  sendResult(true, "Restarting...");
}

void handleLog()      { logStream(server, server.hasArg("download")); }

void handleHistory() {
  const char* files[] = {HIST_OLD, HIST_FILE};
  streamFiles(server, files, 2, "application/octet-stream", server.hasArg("download") ? "wtms-history.bin" : nullptr);
}
void handleFills() {
  const char* files[] = {FILL_OLD, FILL_FILE};
  streamFiles(server, files, 2, "application/octet-stream", nullptr);
}
void handleSite() {
  if (server.hasArg("org"))      setIdField(ident.org, sizeof(ident.org), server.arg("org"));
  if (server.hasArg("building")) setIdField(ident.building, sizeof(ident.building), server.arg("building"));
  if (server.hasArg("tank"))     setIdField(ident.tank, sizeof(ident.tank), server.arg("tank"));
  saveIdentity();
  String nextAp, nextHost;
  deriveNames(nextAp, nextHost);
  logWrite('I', "cfg", String("Site details saved: ") + ident.org + " / " + ident.building + " / " + ident.tank);
  bool rename = nextAp != apName || nextHost != hostName;
  sendResult(true, rename ? "Saved. Restart the device to use the new hotspot name and web address."
                          : "Site details saved");
}
void handleProfile() {
  auto num = [](const char* k, long lo, long hi, long cur) -> long {
    return server.hasArg(k) ? constrain(server.arg(k).toInt(), lo, hi) : cur;
  };
  prof.usage    = num("usage", 0, 1, prof.usage);
  prof.location = num("location", 0, 3, prof.location);
  prof.shape    = num("shape", 0, 1, prof.shape);
  prof.preset   = num("preset", 0, 50, prof.preset);
  prof.lengthMm = num("len", 0, 20000, prof.lengthMm);
  prof.widthMm  = num("wid", 0, 20000, prof.widthMm);
  prof.heightMm = num("hgt", 0, 20000, prof.heightMm);
  prof.diaMm    = num("dia", 0, 20000, prof.diaMm);
  prof.depthMm  = num("depth", 0, 20000, prof.depthMm);
  prof.gapMm    = num("gap", 0, 5000, prof.gapMm);
  saveProfile();
  static const char* const U[] = {"Domestic", "Commercial"};
  static const char* const L[] = {"overhead", "loft / bathroom", "underground sump", "other"};
  String dims = prof.shape ? "cylinder D" + String(prof.diaMm) : String(prof.lengthMm) + "x" + String(prof.widthMm);
  logWrite('I', "cfg", String("Tank profile saved: ") + U[prof.usage] + ", " + L[prof.location] + ", " + dims +
                        "x" + String(prof.heightMm) + " mm, water depth " + String(prof.depthMm) + " mm");
  sendResult(true, "Tank profile saved");
}
void handleHistoryClear() {
  historyClear();
  logWrite('I', "tank", "Tank history cleared from dashboard");
  sendResult(true, "History cleared");
}
void handleLogClear() {
  logClear();
  logWrite('I', "sys", "Log cleared from dashboard");
  sendResult(true, "Log cleared");
}
void handleTime() {                      // browser tells the device the time (works offline)
  uint32_t t = strtoul(server.arg("epoch").c_str(), nullptr, 10);
  if (!logEpochBase && t > 1700000000) {
    setClock(t);
    logWrite('I', "sys", "Clock set from browser");
  }
  sendResult(true, "ok");
}

// ================= Setup / loop =================
String getResetReason() {
#if defined(ESP8266)
  String r = ESP.getResetReason();
  resetIsProblem = r.indexOf("Watchdog") >= 0 || r.indexOf("Exception") >= 0;
  if (r == "Power On") return "Power on (power was cut or dipped)";
  return r;
#else
  esp_reset_reason_t rr = esp_reset_reason();
  resetIsProblem = rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT ||
                   rr == ESP_RST_WDT || rr == ESP_RST_BROWNOUT;
  switch (rr) {
    case ESP_RST_POWERON:  return "Power on (power was cut or dipped)";
    case ESP_RST_SW:       return "Software restart";
    case ESP_RST_PANIC:    return "Crash";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "Watchdog";
    case ESP_RST_BROWNOUT: return "Brownout (power dip)";
    case ESP_RST_EXT:      return "Reset button";
    default:               return "Other";
  }
#endif
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F(PRODUCT " " FW_VERSION " | woodyouloveit.com"));
  Serial.println(F("(C) 2026 Chanchal Sakarde. All Rights Reserved. Open source under GPL-3.0."));
  resetReason = getResetReason();
  loadStorage();
#if defined(ESP32)
  setCpuFrequencyMhz(cfg.perfMode ? 240 : 80);          // before Wi-Fi starts
#endif
  logBegin(bootCount);
  logWrite(resetIsProblem ? 'E' : 'I', "boot",
           "Boot #" + String(bootCount) + ", firmware " FW_VERSION " on " BOARD_NAME ", restart cause: " + resetReason);
#if defined(ESP8266)
  if (resetIsProblem) logWrite('E', "boot", "Crash details: " + ESP.getResetInfo());
  logWrite(supplyMv() < 3000 ? 'W' : 'I', "boot", "Supply voltage " + String(supplyMv() / 1000.0, 2) + " V");
#endif
  if (!logFsOK) logWrite('W', "sys", "Flash log unavailable: log kept in memory only. Check Tools > Partition Scheme (ESP32) or Flash Size with FS (ESP8266).");
  if (logFsFormatted) logWrite('I', "sys", "Flash storage was blank and has been formatted (normal after the first upload or 'Erase All Flash').");
  if (bootCount == 1) logWrite('I', "sys", "First start: settings are at their defaults. Keep Tools > 'Erase All Flash Before Sketch Upload' Disabled to keep them across uploads.");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  strip.begin();
  strip.setBrightness(cfg.brightness);
  strip.updateLength(cfg.numLeds);
  for (int i = 0; i < cfg.numLeds; i++) {              // startup sweep
    setLed(i, redToGreen(posT(i)));
    strip.show();
    delay(25);
  }
  strip.clear();
  strip.show();

  // ---- Wi-Fi: we manage connection ourselves, nothing stored by the SDK ----
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(50);
#if defined(ESP8266)
  WiFi.hostname(hostName);

#else
  WiFi.setHostname(hostName.c_str());
#endif
  WiFi.setAutoReconnect(false);

#if defined(ESP8266)
  hApJoin  = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& e) { pushEvt(EV_AP_JOIN, e.mac, 0); });
  hApLeave = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected& e) { pushEvt(EV_AP_LEAVE, e.mac, 0); });
  hStaDisc = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& e) { pushEvt(EV_STA_DISC, nullptr, (uint16_t)e.reason); });
#else
  WiFi.onEvent([](WiFiEvent_t ev, WiFiEventInfo_t info) {
    if (ev == ARDUINO_EVENT_WIFI_AP_STACONNECTED)         pushEvt(EV_AP_JOIN,  info.wifi_ap_staconnected.mac, 0);
    else if (ev == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) pushEvt(EV_AP_LEAVE, info.wifi_ap_stadisconnected.mac, 0);
    else if (ev == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)   pushEvt(EV_STA_DISC, nullptr, info.wifi_sta_disconnected.reason);
  });
#endif

  if (haveCreds()) {
    if (cfg.apMode == 1) startAP("always-on setting");
    else              WiFi.mode(WIFI_STA);
    beginConnect(creds.ssid, creds.pass, false);
  } else {
    startAP("no home Wi-Fi saved");
    netMsg = "Not connected to a home network yet. Choose one to connect.";
  }
  applyPower();                                         // CPU speed, transmit power, Wi-Fi sleep
  pinMode(BOOT_BTN, INPUT_PULLUP);

  server.on("/",                  HTTP_GET,  handleDashboard);
  server.on("/dashboard",         HTTP_GET,  handleDashboard);
  server.on("/api/status",        HTTP_GET,  handleStatus);
  server.on("/api/scan",          HTTP_GET,  handleScan);
  server.on("/api/calibrate",     HTTP_POST, handleCalibrate);
  server.on("/api/settings",      HTTP_POST, handleSettings);
  server.on("/api/wifi/connect",  HTTP_POST, handleWifiConnect);
  server.on("/api/wifi/forget",   HTTP_POST, handleWifiForget);
  server.on("/api/restart",       HTTP_POST, handleRestart);
  server.on("/api/log",           HTTP_GET,  handleLog);
  server.on("/api/log/clear",     HTTP_POST, handleLogClear);
  server.on("/api/time",          HTTP_POST, handleTime);
  server.on("/api/history",       HTTP_GET,  handleHistory);
  server.on("/api/fills",         HTTP_GET,  handleFills);
  server.on("/api/history/clear", HTTP_POST, handleHistoryClear);
  server.on("/api/site",          HTTP_POST, handleSite);
  server.on("/api/profile",       HTTP_POST, handleProfile);
  server.onNotFound(handleNotFound);
  server.begin();
  lastLoopAt = millis();
}

void loop() {
  diagTask();
  eventTask();
  wifiTask();
  server.handleClient();
#if defined(ESP8266)
  if (mdnsStarted) MDNS.update();
#endif
  sensorTask();
  fillTask(sensorOK ? currentDistance : -1, cfg.fillMmMin, cfg.distFull);
  historyTask(sensorOK ? currentDistance : -1);
  displayTask();
  buttonTask();

  static unsigned long lastLog = 0;                   // live values on Serial in performance mode only
  if (cfg.perfMode && millis() - lastLog > 1000) {
    lastLog = millis();
    if (sensorOK) Serial.printf("Distance: %.1f cm  Level: %.0f%%\n", currentDistance, smoothedLevel * 100);
    else          Serial.println("Sensor: no echo");
  }

  if (restartAt && millis() > restartAt) ESP.restart();
  delay(cfg.perfMode ? 1 : 5);                         // let the CPU idle between passes
}
