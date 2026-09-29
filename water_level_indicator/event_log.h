// Persistent event log for debugging connectivity.
// Lines are stored in flash (LittleFS) so they survive restarts and power cuts:
//   boot <TAB> uptime_s <TAB> unix_time(0=unknown) <TAB> level(I/W/E) <TAB> category <TAB> message
// Two files of ~12 KB each are kept (current + previous), about 300-400 events.
#pragma once
#include <Arduino.h>
#include <LittleFS.h>

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

void logBegin(uint32_t boot) {
  logBoot = boot;
#if defined(ESP32)
  logFsOK = LittleFS.begin(true);   // format on first use
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

// Streams the whole log (older file first) without building it in RAM
template <class Server>
void logStream(Server& server, bool download) {
  size_t total = logFsOK ? fileSize(LOG_OLD) + fileSize(LOG_FILE) : logRam.length();
  server.sendHeader("Cache-Control", "no-store");
  if (download) server.sendHeader("Content-Disposition", "attachment; filename=\"yucca-tank-log.txt\"");
  server.setContentLength(total);
  server.send(200, "text/plain", "");
  if (!logFsOK) {
    if (total) server.sendContent(logRam);
    return;
  }
  const char* files[] = {LOG_OLD, LOG_FILE};
  char buf[513];
  for (const char* path : files) {
    if (!LittleFS.exists(path)) continue;
    File f = LittleFS.open(path, "r");
    if (!f) continue;
    while (f.available()) {
      size_t n = f.readBytes(buf, 512);
      buf[n] = 0;
      server.sendContent(String(buf));
    }
    f.close();
  }
}
