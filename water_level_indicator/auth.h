/*
  Water Tanks Monitor System - admin login
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

// How the login works (the password never travels in plain text):
//   stored  = SHA256(saltHex + password)                 kept in EEPROM with a random salt
//   browser asks /api/auth for the salt and a one-time nonce
//   proof   = SHA256(nonceHex + SHA256(saltHex + password))
//   device compares it with SHA256(nonceHex + storedHex) and returns a session token.
// Default password "admin"; it has to be changed at the first login.
// Holding the BOOT button for 10 s resets the password to "admin".
// Note: the dashboard uses plain HTTP, so the session token itself is not encrypted on the network.
#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// ---------------- SHA-256 (FIPS 180-4) ----------------
struct Sha256 {
  uint32_t h[8];
  uint8_t  buf[64];
  uint64_t len;
  size_t   used;
};

static const uint32_t SHA_K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static inline uint32_t shaRot(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void shaBlock(Sha256& c, const uint8_t* p) {
  uint32_t w[64];
  for (int i = 0; i < 16; i++)
    w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) | ((uint32_t)p[i * 4 + 2] << 8) | p[i * 4 + 3];
  for (int i = 16; i < 64; i++) {
    uint32_t s0 = shaRot(w[i - 15], 7) ^ shaRot(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = shaRot(w[i - 2], 17) ^ shaRot(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint32_t a = c.h[0], b = c.h[1], d2 = c.h[2], d = c.h[3], e = c.h[4], f = c.h[5], g = c.h[6], hh = c.h[7];
  for (int i = 0; i < 64; i++) {
    uint32_t S1 = shaRot(e, 6) ^ shaRot(e, 11) ^ shaRot(e, 25);
    uint32_t ch = (e & f) ^ (~e & g);
    uint32_t t1 = hh + S1 + ch + SHA_K[i] + w[i];
    uint32_t S0 = shaRot(a, 2) ^ shaRot(a, 13) ^ shaRot(a, 22);
    uint32_t mj = (a & b) ^ (a & d2) ^ (b & d2);
    uint32_t t2 = S0 + mj;
    hh = g; g = f; f = e; e = d + t1; d = d2; d2 = b; b = a; a = t1 + t2;
  }
  c.h[0] += a; c.h[1] += b; c.h[2] += d2; c.h[3] += d; c.h[4] += e; c.h[5] += f; c.h[6] += g; c.h[7] += hh;
}

static void shaInit(Sha256& c) {
  static const uint32_t H0[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  memcpy(c.h, H0, sizeof(H0));
  c.len = 0;
  c.used = 0;
}

static void shaUpdate(Sha256& c, const uint8_t* d, size_t n) {
  c.len += n;
  while (n--) {
    c.buf[c.used++] = *d++;
    if (c.used == 64) { shaBlock(c, c.buf); c.used = 0; }
  }
}

static void shaFinal(Sha256& c, uint8_t out[32]) {
  uint64_t bits = c.len * 8;
  uint8_t pad = 0x80;
  shaUpdate(c, &pad, 1);
  uint8_t zero = 0;
  while (c.used != 56) shaUpdate(c, &zero, 1);
  uint8_t lenb[8];
  for (int i = 0; i < 8; i++) lenb[i] = bits >> (56 - 8 * i);
  shaUpdate(c, lenb, 8);
  for (int i = 0; i < 8; i++) {
    out[i * 4] = c.h[i] >> 24; out[i * 4 + 1] = c.h[i] >> 16; out[i * 4 + 2] = c.h[i] >> 8; out[i * 4 + 3] = c.h[i];
  }
}

static String toHex(const uint8_t* b, size_t n) {
  static const char* hx = "0123456789abcdef";
  String s;
  s.reserve(n * 2);
  for (size_t i = 0; i < n; i++) { s += hx[b[i] >> 4]; s += hx[b[i] & 15]; }
  return s;
}

// SHA-256 of a text, as 64 lowercase hex characters
static String sha256Hex(const String& text) {
  Sha256 c;
  uint8_t out[32];
  shaInit(c);
  shaUpdate(c, (const uint8_t*)text.c_str(), text.length());
  shaFinal(c, out);
  return toHex(out, 32);
}

static bool fromHex(const String& s, uint8_t* out, size_t n) {
  if (s.length() != n * 2) return false;
  for (size_t i = 0; i < n; i++) {
    uint8_t v = 0;
    for (int k = 0; k < 2; k++) {
      char c = s[i * 2 + k];
      v <<= 4;
      if (c >= '0' && c <= '9') v |= c - '0';
      else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
      else return false;
    }
    out[i] = v;
  }
  return true;
}

static uint32_t hwRandom() {
#if defined(ESP32)
  return esp_random();
#else
  return RANDOM_REG32;
#endif
}

static String randomHex(size_t bytes) {
  uint8_t b[32];
  for (size_t i = 0; i < bytes; i += 4) {
    uint32_t r = hwRandom();
    for (size_t k = 0; k < 4 && i + k < bytes; k++) b[i + k] = r >> (8 * k);
  }
  return toHex(b, bytes);
}

// ---------------- Stored password (EEPROM offset 416) ----------------
struct AuthData {
  uint32_t magic;
  uint8_t  salt[16];
  uint8_t  hash[32];      // SHA256(saltHex + password)
  uint8_t  mustChange;    // 1 = default password, change at next login
};
const uint32_t AUTH_MAGIC = 0x5754414D;       // "WTAM"
const int      AUTH_ADDR  = 416;
AuthData auth;

const unsigned long SESSION_IDLE_MS = 12UL * 3600UL * 1000UL;   // log out after 12 h without activity
// Named AuthSession: the ESP8266 core already has a global "Session" (BearSSL::Session)
struct AuthSession { char token[33]; unsigned long last; };
AuthSession authSessions[3];
String   authNonce;
unsigned long authNonceAt = 0;
uint8_t  authFails = 0;
unsigned long authLockUntil = 0;

void authSetPassword(const String& hashHex, bool mustChange) {
  fromHex(hashHex, auth.hash, 32);
  auth.mustChange = mustChange ? 1 : 0;
  auth.magic = AUTH_MAGIC;
  EEPROM.put(AUTH_ADDR, auth);
  EEPROM.commit();
}

void authResetDefault() {
  uint8_t s[16];
  fromHex(randomHex(16), s, 16);
  memcpy(auth.salt, s, 16);
  authSetPassword(sha256Hex(toHex(auth.salt, 16) + "admin"), true);
  memset(authSessions, 0, sizeof(authSessions));
}

void authLoad() {
  EEPROM.get(AUTH_ADDR, auth);
  if (auth.magic != AUTH_MAGIC || auth.mustChange > 1) authResetDefault();
}

String authSaltHex() { return toHex(auth.salt, 16); }

// Checks the "auth" argument (form field or query) of the current request
template <class Server>
bool authIsAdmin(Server& server) {
  String t = server.arg("auth");
  if (t.length() != 32) return false;
  unsigned long now = millis();
  for (AuthSession& s : authSessions) {
    if (s.token[0] && t == s.token) {
      if (now - s.last > SESSION_IDLE_MS) { s.token[0] = 0; return false; }
      s.last = now;
      return true;
    }
  }
  return false;
}

String authNewSession() {
  int slot = 0;
  for (int i = 1; i < 3; i++)                   // reuse the oldest (or an empty) slot
    if (!authSessions[i].token[0] || authSessions[i].last < authSessions[slot].last) slot = i;
  if (!authSessions[slot].token[0]) {}
  String t = randomHex(16);
  strncpy(authSessions[slot].token, t.c_str(), 32);
  authSessions[slot].token[32] = 0;
  authSessions[slot].last = millis();
  return t;
}

void authEndSession(const String& t) {
  for (AuthSession& s : authSessions)
    if (s.token[0] && t == s.token) s.token[0] = 0;
}

// Returns "" on success, otherwise the reason
String authCheckProof(const String& proof) {
  unsigned long now = millis();
  if (authLockUntil && now < authLockUntil) return "Too many attempts. Wait a minute and try again.";
  if (!authNonce.length() || now - authNonceAt > 120000UL) return "Login expired. Try again.";
  String expected = sha256Hex(authNonce + toHex(auth.hash, 32));
  authNonce = "";                              // one attempt per nonce
  uint8_t diff = proof.length() == 64 ? 0 : 1;
  for (unsigned int i = 0; i < 64 && i < proof.length(); i++) diff |= proof[i] ^ expected[i];
  if (diff) {
    if (++authFails >= 5) { authFails = 0; authLockUntil = now + 60000UL; }
    return "Wrong password";
  }
  authFails = 0;
  return "";
}
