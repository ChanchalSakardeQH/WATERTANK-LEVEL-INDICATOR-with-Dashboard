/*
  Water Tanks Monitor System - level history and fill (motor) detection
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

// Tank history and fill (motor run) detection, stored in flash. One or more sensors (tank compartments).
//
//  /hist.bin    one record every 2 minutes: time, distance (mm) per sensor, flags (bit i = sensor i filling)
//               record size 4 + 2*n + 2 bytes (8 bytes for one sensor, the same layout as before 3.0.0)
//               two rotating files sized from the flash storage (default 60 % of it, see histAllocate):
//               the oldest half is dropped when both are full, so the newest data is always kept
//  /fill2.bin   one record per detected fill: start, end, start/end distance, sensor (16 bytes)
//               two files of 100 records
//
// Distances are stored (not %), so the dashboard can re-calculate levels after recalibration.
// Changing the number of sensors clears the history (records would no longer line up).
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "event_log.h"

#define WTMS_MAX_SENSORS 4
String sensorWho(int i);                          // "<name>: " for log lines (defined in the sketch)
#define HIST_FILE      "/hist.bin"
#define HIST_OLD       "/hist.old.bin"
#define HIST_FILE_MIN   20160              // smallest file: 2,520 records of 8 bytes (3.5 days)
#define FILL_FILE      "/fill2.bin"
#define FILL_OLD       "/fill2.old.bin"
#define FILL_V1_FILE   "/fills.bin"        // before 3.0.0: 12-byte records, one sensor
#define FILL_V1_OLD    "/fills.old.bin"
#define FILL_MAX_RECS  100
#define HIST_INVALID   0xFFFF

const unsigned long HIST_INTERVAL_MS = 120000;   // 2 minutes
const unsigned long FILL_SAMPLE_MS   = 10000;    // fill detector sample every 10 s

struct FillRecV1 { uint32_t start; uint32_t end; uint16_t startMm; uint16_t endMm; };
struct FillRec   { uint32_t start; uint32_t end; uint16_t startMm; uint16_t endMm; uint8_t sensor; uint8_t pad[3]; };

uint8_t histSensors = 1;                          // sensors per history record (set from the configuration)
uint32_t histFileBytes = 40320;                   // size of each of the two history files
uint8_t  histPctUsed = 60;                        // share of the flash storage given to history

// Size the history from the storage on this chip: pct % of it (0 = automatic, 60 %),
// minus room for the log (24 KB), the fill records and headroom, split over the two rotating files
void histAllocate(uint8_t pct) {
  histPctUsed = pct ? pct : 60;
  long budget = (long)(fsTotalBytes() / 100 * histPctUsed) - 32L * 1024;
  long per = budget / 2;
  if (per < HIST_FILE_MIN) per = HIST_FILE_MIN;
  histFileBytes = (uint32_t)per;
}

void appendRec(const char* path, const char* old, const void* rec, size_t size, size_t maxRecs) {
  if (!logFsOK) return;
  // Never fill the storage: with less than 16 KB free, drop the older file first
  if (fsTotalBytes() - fsUsedBytes() < 16384 && LittleFS.exists(old)) LittleFS.remove(old);
  File f = LittleFS.open(path, "a");
  if (!f) return;
  f.write((const uint8_t*)rec, size);
  size_t total = f.size();
  f.close();
  if (total >= maxRecs * size) {
    LittleFS.remove(old);
    LittleFS.rename(path, old);
  }
}

void historyClear() {
  if (!logFsOK) return;
  LittleFS.remove(HIST_FILE); LittleFS.remove(HIST_OLD);
  LittleFS.remove(FILL_FILE); LittleFS.remove(FILL_OLD);
}

// Fill records from firmware before 3.0.0 belong to sensor 1: convert them once
void historyMigrate() {
  if (!logFsOK) return;
  const char* olds[2] = {FILL_V1_OLD, FILL_V1_FILE};
  bool any = false;
  for (const char* path : olds) {
    if (!LittleFS.exists(path)) continue;
    File f = LittleFS.open(path, "r");
    FillRecV1 v;
    while (f && f.read((uint8_t*)&v, sizeof(v)) == sizeof(v)) {
      FillRec r = { v.start, v.end, v.startMm, v.endMm, 0, {0, 0, 0} };
      appendRec(FILL_FILE, FILL_OLD, &r, sizeof(r), FILL_MAX_RECS);
      any = true;
    }
    if (f) f.close();
    LittleFS.remove(path);
  }
  if (any) logWrite('I', "sys", "Fill records converted to the multi-sensor format");
}

// ================= Fill detector (one per sensor) =================
// Compares the median of the 5 oldest and 5 newest samples in a 3-minute window.
// Filling starts when the water rises faster than the threshold (and by 1.5 cm or more) for 30 s,
// and ends when it rises slower than half the threshold for 60 s, or the compartment is full.
struct FillDetector {
  static const int N = 19;               // 19 samples x 10 s = 3 minutes
  uint16_t buf[N];
  int      count = 0, head = 0;
  bool     filling = false;
  int      upCount = 0, downCount = 0;
  uint32_t startT = 0, bestT = 0;
  uint16_t startMm = 0, bestMm = 0;
  float    rateMmMin = 0;                // current rise speed (distance decrease)
  unsigned long lastSample = 0, lastValid = 0;
};
FillDetector fd[WTMS_MAX_SENSORS];

void fillReset() { for (int i = 0; i < WTMS_MAX_SENSORS; i++) fd[i] = FillDetector(); }
bool anyFilling(int n) { for (int i = 0; i < n; i++) if (fd[i].filling) return true; return false; }

// Median of 5 consecutive samples starting at ring index `from`
static uint16_t med5(const uint16_t* buf, int from, int n) {
  uint16_t v[5];
  for (int i = 0; i < 5; i++) v[i] = buf[(from + i) % n];
  for (int i = 1; i < 5; i++) {
    uint16_t k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) { v[j + 1] = v[j]; j--; }
    v[j + 1] = k;
  }
  return v[2];
}

void endFill(int i, const char* reason) {
  const String who = sensorWho(i);
  FillDetector& f = fd[i];
  f.filling = false;
  f.upCount = f.downCount = 0;
  int rise = (int)f.startMm - (int)f.bestMm;
  long dur = (long)f.bestT - (long)f.startT;
  if (rise >= 30 && dur >= 180 && f.startT) {                 // at least 3 cm over 3 minutes
    FillRec r = { f.startT, f.bestT, f.startMm, f.bestMm, (uint8_t)i, {0, 0, 0} };
    appendRec(FILL_FILE, FILL_OLD, &r, sizeof(r), FILL_MAX_RECS);
    logWrite('I', "tank", who + String("Filling stopped (") + reason + "): +" + String(rise / 10.0, 1) + " cm in " +
                          String(dur / 60.0, 1) + " min, " + String(rise / 10.0 / (dur / 60.0), 2) + " cm/min");
  } else {
    logWrite('I', "tank", who + String("Filling stopped (") + reason + "), too small to record");
  }
}

// distCm: current median distance (cm) of sensor i, or <0 if no reading
void fillTask(int i, float distCm, uint16_t thrMmMin, float fullCm) {
  FillDetector& f = fd[i];
  unsigned long now = millis();
  if (now - f.lastSample < FILL_SAMPLE_MS) return;
  f.lastSample = now;

  if (distCm < 0) {
    if (f.filling && now - f.lastValid > 300000) endFill(i, "sensor fault");
    return;
  }
  f.lastValid = now;
  uint16_t mm = (uint16_t)(distCm * 10 + 0.5);
  f.buf[f.head] = mm;
  f.head = (f.head + 1) % FillDetector::N;
  if (f.count < FillDetector::N) { f.count++; return; }

  const int N = FillDetector::N;
  uint16_t oldMed = med5(f.buf, f.head, N);                 // 5 oldest
  uint16_t newMed = med5(f.buf, f.head + N - 5, N);         // 5 newest
  // The two medians are centred (N-5) samples apart: 14 x 10 s = 140 s
  const float spanMin = (N - 5) * (FILL_SAMPLE_MS / 1000.0f) / 60.0f;
  f.rateMmMin = ((float)oldMed - (float)newMed) / spanMin;

  if (!f.filling) {
    if (f.rateMmMin >= thrMmMin && oldMed - newMed >= 15) {  // and at least 1.5 cm real rise
      if (++f.upCount >= 3) {
        uint32_t t = nowEpoch();
        f.filling = true;
        f.downCount = 0;
        f.startT = t ? t - 90 : 0;                            // detection lags the real start by ~1-2 min
        f.startMm = oldMed;
        f.bestMm = newMed;
        f.bestT = t;
        logWrite('I', "tank", sensorWho(i) + "Filling started, rising " + String(f.rateMmMin / 10.0, 2) + " cm/min");
      }
    } else {
      f.upCount = 0;
    }
  } else {
    if (newMed < f.bestMm) { f.bestMm = newMed; f.bestT = nowEpoch(); }
    bool full = newMed <= (uint16_t)(fullCm * 10 + 5);
    if (full || f.rateMmMin < thrMmMin / 2.0f) {
      if (++f.downCount >= (full ? 1 : 6)) endFill(i, full ? "full" : "level stopped rising");
    } else {
      f.downCount = 0;
    }
  }
}

// Minutes until sensor i is full at the current rate, -1 if unknown
float fillEtaMin(int i, float distCm, float fullCm) {
  if (!fd[i].filling || fd[i].rateMmMin <= 0.5 || distCm < 0) return -1;
  float left = (distCm - fullCm) * 10;
  return left <= 0 ? 0 : left / fd[i].rateMmMin;
}

// ================= History recorder =================
struct HistAcc { double sum[WTMS_MAX_SENSORS]; uint16_t n[WTMS_MAX_SENSORS]; unsigned long last = 0, lastSec = 0; } hacc;

// dist: current distance (cm) per sensor, <0 = no reading
void historyTask(const float* dist) {
  const int n = histSensors;
  unsigned long now = millis();
  if (now - hacc.lastSec >= 1000) {                            // average of 1 s samples
    hacc.lastSec = now;
    for (int i = 0; i < n; i++) if (dist[i] >= 0) { hacc.sum[i] += dist[i]; hacc.n[i]++; }
  }
  if (now - hacc.last < HIST_INTERVAL_MS) return;
  hacc.last = now;
  uint32_t t = nowEpoch();
  if (t) {                                                     // needs a real clock
    uint8_t rec[4 + 2 * WTMS_MAX_SENSORS + 2];
    size_t size = 4 + 2 * n + 2;
    memcpy(rec, &t, 4);
    uint16_t flags = 0;
    for (int i = 0; i < n; i++) {
      uint16_t mm = hacc.n[i] ? (uint16_t)(hacc.sum[i] / hacc.n[i] * 10 + 0.5) : HIST_INVALID;
      memcpy(rec + 4 + 2 * i, &mm, 2);
      if (fd[i].filling) flags |= (1 << i);
    }
    memcpy(rec + 4 + 2 * n, &flags, 2);
    appendRec(HIST_FILE, HIST_OLD, rec, size, histFileBytes / size);
  }
  for (int i = 0; i < WTMS_MAX_SENSORS; i++) { hacc.sum[i] = 0; hacc.n[i] = 0; }
}
