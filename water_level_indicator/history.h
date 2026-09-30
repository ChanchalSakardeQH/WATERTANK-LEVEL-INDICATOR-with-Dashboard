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

// Tank history and fill (motor run) detection, stored in flash.
//
//  /hist.bin   one record every 2 minutes: time, distance (mm), flags   (8 bytes)
//              two files of 5040 records = 14 days
//  /fills.bin  one record per detected fill: start, end, start/end distance (12 bytes)
//              two files of 100 records = 200 fills
//
// Distances are stored (not %), so the dashboard can re-calculate levels after recalibration.
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "event_log.h"

#define HIST_FILE      "/hist.bin"
#define HIST_OLD       "/hist.old.bin"
#define HIST_MAX_RECS  5040
#define FILL_FILE      "/fills.bin"
#define FILL_OLD       "/fills.old.bin"
#define FILL_MAX_RECS  100
#define HIST_INVALID   0xFFFF

const unsigned long HIST_INTERVAL_MS = 120000;   // 2 minutes
const unsigned long FILL_SAMPLE_MS   = 10000;    // fill detector sample every 10 s

struct HistRec { uint32_t t; uint16_t mm; uint16_t flags; };               // flags bit0 = filling
struct FillRec { uint32_t start; uint32_t end; uint16_t startMm; uint16_t endMm; };

void appendRec(const char* path, const char* old, const void* rec, size_t size, size_t maxRecs) {
  if (!logFsOK) return;
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

// ================= Fill detector =================
// Compares the median of the 5 oldest and 5 newest samples in a 3-minute window.
// Filling starts when the water rises faster than the threshold (and by 1 cm or more) for 30 s,
// and ends when it rises slower than half the threshold for 60 s, or the tank is full.
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
} fd;

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

void endFill(const char* reason) {
  fd.filling = false;
  fd.upCount = fd.downCount = 0;
  int rise = (int)fd.startMm - (int)fd.bestMm;
  long dur = (long)fd.bestT - (long)fd.startT;
  if (rise >= 30 && dur >= 180 && fd.startT) {                 // at least 3 cm over 3 minutes
    FillRec r = { fd.startT, fd.bestT, fd.startMm, fd.bestMm };
    appendRec(FILL_FILE, FILL_OLD, &r, sizeof(r), FILL_MAX_RECS);
    logWrite('I', "tank", String("Filling stopped (") + reason + "): +" + String(rise / 10.0, 1) + " cm in " +
                          String(dur / 60.0, 1) + " min, " + String(rise / 10.0 / (dur / 60.0), 2) + " cm/min");
  } else {
    logWrite('I', "tank", String("Filling stopped (") + reason + "), too small to record");
  }
}

// validCm: current median distance (cm) or <0 if no reading
void fillTask(float distCm, uint16_t thrMmMin, float fullCm) {
  unsigned long now = millis();
  if (now - fd.lastSample < FILL_SAMPLE_MS) return;
  fd.lastSample = now;

  if (distCm < 0) {
    if (fd.filling && now - fd.lastValid > 300000) endFill("sensor fault");
    return;
  }
  fd.lastValid = now;
  uint16_t mm = (uint16_t)(distCm * 10 + 0.5);
  fd.buf[fd.head] = mm;
  fd.head = (fd.head + 1) % FillDetector::N;
  if (fd.count < FillDetector::N) { fd.count++; return; }

  const int N = FillDetector::N;
  uint16_t oldMed = med5(fd.buf, fd.head, N);                 // 5 oldest
  uint16_t newMed = med5(fd.buf, fd.head + N - 5, N);         // 5 newest
  // The two medians are centred (N-5) samples apart: 14 x 10 s = 140 s
  const float spanMin = (N - 5) * (FILL_SAMPLE_MS / 1000.0f) / 60.0f;
  fd.rateMmMin = ((float)oldMed - (float)newMed) / spanMin;

  if (!fd.filling) {
    if (fd.rateMmMin >= thrMmMin && oldMed - newMed >= 15) {   // and at least 1.5 cm real rise
      if (++fd.upCount >= 3) {
        uint32_t t = nowEpoch();
        fd.filling = true;
        fd.downCount = 0;
        fd.startT = t ? t - 90 : 0;                            // detection lags the real start by ~1-2 min
        fd.startMm = oldMed;
        fd.bestMm = newMed;
        fd.bestT = t;
        logWrite('I', "tank", "Filling started, rising " + String(fd.rateMmMin / 10.0, 2) + " cm/min");
      }
    } else {
      fd.upCount = 0;
    }
  } else {
    if (newMed < fd.bestMm) { fd.bestMm = newMed; fd.bestT = nowEpoch(); }
    bool full = newMed <= (uint16_t)(fullCm * 10 + 5);
    if (full || fd.rateMmMin < thrMmMin / 2.0f) {
      if (++fd.downCount >= (full ? 1 : 6)) endFill(full ? "tank full" : "level stopped rising");
    } else {
      fd.downCount = 0;
    }
  }
}

// Minutes until full at the current rate, -1 if unknown
float fillEtaMin(float distCm, float fullCm) {
  if (!fd.filling || fd.rateMmMin <= 0.5 || distCm < 0) return -1;
  float left = (distCm - fullCm) * 10;
  return left <= 0 ? 0 : left / fd.rateMmMin;
}

// ================= History recorder =================
struct { double sum = 0; uint16_t n = 0; unsigned long last = 0, lastSec = 0; } hacc;

void historyTask(float distCm) {
  unsigned long now = millis();
  if (now - hacc.lastSec >= 1000) {                            // average of 1 s samples
    hacc.lastSec = now;
    if (distCm >= 0) { hacc.sum += distCm; hacc.n++; }
  }
  if (now - hacc.last < HIST_INTERVAL_MS) return;
  hacc.last = now;
  uint32_t t = nowEpoch();
  if (t) {                                                     // needs a real clock
    HistRec r;
    r.t = t;
    r.mm = hacc.n ? (uint16_t)(hacc.sum / hacc.n * 10 + 0.5) : HIST_INVALID;
    r.flags = fd.filling ? 1 : 0;
    appendRec(HIST_FILE, HIST_OLD, &r, sizeof(r), HIST_MAX_RECS);
  }
  hacc.sum = 0;
  hacc.n = 0;
}
