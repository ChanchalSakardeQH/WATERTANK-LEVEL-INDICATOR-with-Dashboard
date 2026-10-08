/*
  Water Tanks Monitor System - ultrasonic sensor diagnostic
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
  AJ-SR04M / JSN-SR04T sensor diagnostic  (ESP32 or ESP8266 NodeMCU)

  Uses the same pins as the main sketch:
    ESP8266: TRIG = D5 (GPIO14) -> sensor RX,  ECHO = D6 (GPIO12) <- sensor TX
    ESP32  : TRIG = D15         -> sensor RX,  ECHO = D2          <- sensor TX

  Point the probe at a wall 30cm - 1m away, open Serial Monitor at 115200,
  and read the VERDICT printed after each round.
*/

#include <Arduino.h>

#if defined(ESP32)
  #define BOARD_NAME "ESP32"
  #define TRIG_PIN 15   // D15
  #define ECHO_PIN 2    // D2
  HardwareSerial SensorSerial(2);
#elif defined(ESP8266)
  #include <SoftwareSerial.h>
  #define BOARD_NAME "ESP8266 NodeMCU"
  #define TRIG_PIN 14   // D5
  #define ECHO_PIN 12   // D6
  SoftwareSerial SensorSerial(ECHO_PIN, TRIG_PIN);   // RX, TX (from ESP side)
#else
  #error "Select an ESP32 or ESP8266 board"
#endif

// Results of one round
bool idleHigh;
int  edges;
int  workingWidth;        // trigger width that gave an echo, 0 = none
bool passiveFrames;       // sensor sends data on its own (auto serial mode)
bool requestFrames;       // sensor answers 0x01 (serial on request mode)

void resetPins() {
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
}

// ---------- 1. Idle level and activity on the ECHO line ----------
void testIdle() {
  resetPins();
  delay(100);
  int last = digitalRead(ECHO_PIN);
  idleHigh = (last == HIGH);
  edges = 0;
  unsigned long start = millis();
  while (millis() - start < 1000) {
    int v = digitalRead(ECHO_PIN);
    if (v != last) { edges++; last = v; }
    yield();
  }
  Serial.printf("[1] ECHO idle level: %s, changes in 1s without trigger: %d\n",
                idleHigh ? "HIGH" : "LOW", edges);
}

// ---------- 2. Trigger / echo pulse mode with different trigger widths ----------
void testPulse() {
  const int widths[] = {10, 20, 50, 100, 500, 1000};
  workingWidth = 0;
  resetPins();
  for (int w : widths) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(w);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long us = pulseIn(ECHO_PIN, HIGH, 60000);
    if (us > 0) {
      Serial.printf("[2] trigger %4d us -> echo %6lu us = %.1f cm\n", w, us, us * 0.0343 / 2.0);
      if (!workingWidth) workingWidth = w;
    } else {
      Serial.printf("[2] trigger %4d us -> no echo\n", w);
    }
    delay(100);
  }
}

// ---------- 3. Serial (UART 9600) modes ----------
int readBytes(uint8_t *buf, int maxLen, unsigned long ms) {
  int n = 0;
  unsigned long start = millis();
  while (millis() - start < ms) {
    while (SensorSerial.available() && n < maxLen) buf[n++] = SensorSerial.read();
    yield();
  }
  return n;
}

bool printFrames(const char *label, uint8_t *buf, int n) {
  Serial.printf("[3] %s: %d bytes", label, n);
  for (int i = 0; i < n && i < 16; i++) Serial.printf(" %02X", buf[i]);
  Serial.println();
  bool found = false;
  for (int i = 0; i + 3 < n; i++) {
    if (buf[i] == 0xFF && (uint8_t)(buf[i] + buf[i + 1] + buf[i + 2]) == buf[i + 3]) {
      int mm = buf[i + 1] * 256 + buf[i + 2];
      Serial.printf("    valid frame: distance %.1f cm\n", mm / 10.0);
      found = true;
    }
  }
  return found;
}

void testSerial() {
#if defined(ESP32)
  SensorSerial.begin(9600, SERIAL_8N1, ECHO_PIN, TRIG_PIN);
#else
  SensorSerial.begin(9600);
#endif
  delay(50);
  while (SensorSerial.available()) SensorSerial.read();

  uint8_t buf[64];
  int n = readBytes(buf, sizeof(buf), 400);                 // listen only
  passiveFrames = printFrames("listening", buf, n);

  SensorSerial.write((uint8_t)0x01);                        // request a reading
  n = readBytes(buf, sizeof(buf), 200);
  requestFrames = printFrames("after sending 0x01", buf, n);

  SensorSerial.end();
  resetPins();
}

// ---------- Verdict ----------
void verdict() {
  Serial.println("---------------- VERDICT ----------------");
  if (workingWidth) {
    Serial.printf("Pulse mode WORKS with a %d us trigger.\n", workingWidth);
    if (workingWidth > 20)
      Serial.printf("-> In the main sketch change delayMicroseconds(20) to delayMicroseconds(%d).\n", workingWidth);
    else
      Serial.println("-> The main sketch should work. Re-flash it.");
  } else if (passiveFrames) {
    Serial.println("Sensor is in AUTOMATIC SERIAL mode (a resistor is fitted at R19).");
    Serial.println("-> Tell Claude: the main sketch needs serial-mode reading.");
  } else if (requestFrames) {
    Serial.println("Sensor is in SERIAL ON REQUEST mode (a resistor is fitted at R19).");
    Serial.println("-> Tell Claude: the main sketch needs serial-mode reading.");
  } else if (idleHigh) {
    Serial.println("ECHO line sits HIGH but no data or echo.");
    Serial.println("-> Likely RX/TX swapped: swap the D5 and D6 wires and run again.");
  } else {
    Serial.println("No response at all.");
    Serial.println("-> 1. Swap the D5/D6 wires and run again.");
    Serial.println("-> 2. Check sensor GND is connected to NodeMCU GND.");
    Serial.println("-> 3. Sensor may not work at 3.3V: power it from 5V WITH a divider on TX.");
  }
  Serial.println("-----------------------------------------\n");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n=== Sensor diagnostic on %s ===\n", BOARD_NAME);
  Serial.println("Point the probe at a wall 30cm - 1m away.\n");
}

void loop() {
  testIdle();
  testPulse();
  testSerial();
  verdict();
  delay(3000);
}
