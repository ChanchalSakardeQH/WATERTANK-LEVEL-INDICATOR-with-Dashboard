/*
  Water Tanks Monitor System - connectivity log stored in flash
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

// Persistent event log for debugging connectivity.
// Lines are stored in flash (LittleFS) so they survive restarts and power cuts:
//   boot <TAB> uptime_s <TAB> unix_time(0=unknown) <TAB> level(I/W/E) <TAB> category <TAB> message
// Two files of ~12 KB each are kept (current + previous), about 300-400 events.
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#if defined(ESP32)
#include <esp_log.h>
#endif

#define LOG_FILE      "/log.txt"
#define LOG_OLD       "/log.old.txt"
#define LOG_MAX_BYTES 12000

bool     logFsOK       = false;
uint32_t logBoot       = 0;
uint32_t logEpochBase  = 0;     // unix time when millis() was 0; 0 = clock unknown
String   logRam;                // fallback if the flash filesystem is unavailable

uint32_t nowEpoch() { return logEpochBase ? logEpochBase + millis() / 1000 : 0; }

void setClock(uint32_t epochNow) {
  logEpochBase = epochNow - millis() / 1000;
}

bool logFsFormatted = false;     // storage was blank or unreadable and has just been formatted

void logBegin(uint32_t boot) {
  logBoot = boot;
#if defined(ESP32)
  // Blank flash (first upload, or "Erase All Flash Before Sketch Upload" enabled) is normal:
  // hide the library's red mount errors, format, and report it once in our own log.
  esp_log_level_set("esp_littlefs", ESP_LOG_NONE);
  logFsOK = LittleFS.begin(false);
  if (!logFsOK) {
    logFsOK = LittleFS.begin(true);  // format and mount
    logFsFormatted = logFsOK;
  }
  esp_log_level_set("esp_littlefs", ESP_LOG_WARN);
#else
  logFsOK = LittleFS.begin();       // ESP8266 formats automatically if needed
#endif
}

void logWrite(char level, const char* category, const String& text) {
  String msg = text;
  msg.replace('\t', ' ');
  msg.replace('\n', ' ');
  msg.replace('\r', ' ');
  String line = String(logBoot) + '\t' + String(millis() / 1000) + '\t' + String(nowEpoch()) + '\t' +
                level + '\t' + category + '\t' + msg + '\n';
  Serial.print("[log] ");
  Serial.print(line);

  if (logFsOK) {
    File f = LittleFS.open(LOG_FILE, "a");
    if (f) {
      f.print(line);
      size_t size = f.size();
      f.close();
      if (size > LOG_MAX_BYTES) {           // rotate: keep one previous file
        LittleFS.remove(LOG_OLD);
        LittleFS.rename(LOG_FILE, LOG_OLD);
      }
      return;
    }
  }
  logRam += line;
  if (logRam.length() > 4000) {
    int cut = logRam.indexOf('\n', logRam.length() - 3000);
    logRam = logRam.substring(cut + 1);
  }
}

void logClear() {
  if (logFsOK) {
    LittleFS.remove(LOG_OLD);
    LittleFS.remove(LOG_FILE);
  }
  logRam = "";
}

size_t fileSize(const char* path) {
  if (!LittleFS.exists(path)) return 0;
  File f = LittleFS.open(path, "r");
  size_t s = f ? f.size() : 0;
  if (f) f.close();
  return s;
}

// Streams files one after another (older first) without building them in RAM. Binary-safe.
template <class Server>
void streamFiles(Server& server, const char* const* files, int nFiles, const char* type, const char* downloadName) {
  size_t total = 0;
  for (int i = 0; i < nFiles; i++) total += fileSize(files[i]);
  server.sendHeader("Cache-Control", "no-store");
  if (downloadName) server.sendHeader("Content-Disposition", String("attachment; filename=\"") + downloadName + "\"");
  server.setContentLength(total);
  server.send(200, type, "");
  char buf[512];
  for (int i = 0; i < nFiles; i++) {
    if (!LittleFS.exists(files[i])) continue;
    File f = LittleFS.open(files[i], "r");
    if (!f) continue;
    while (f.available()) {
      size_t n = f.readBytes(buf, sizeof(buf));
      if (!n) break;
      server.sendContent(buf, n);
    }
    f.close();
  }
}

template <class Server>
void logStream(Server& server, bool download) {
  const char* name = download ? "wtms-log.txt" : nullptr;
  if (!logFsOK) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/plain", logRam);
    return;
  }
  const char* files[] = {LOG_OLD, LOG_FILE};
  streamFiles(server, files, 2, "text/plain", name);
}
