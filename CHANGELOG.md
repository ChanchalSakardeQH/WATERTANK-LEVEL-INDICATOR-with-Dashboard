# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.10.1] - 2026-09-30

### Fixed
- ESP8266 build error "reference to 'Session' is ambiguous": the ESP8266 core exposes its own `BearSSL::Session` globally. The admin session type in `auth.h` is renamed `AuthSession` (and `sessions` → `authSessions`). No change in behaviour; ESP32 builds were not affected.

## [2.10.0] - 2026-09-30

### Added
- **Firmware update over Wi-Fi (OTA)**, admin only: Setup → Firmware update.
  - Upload the `.bin` from "Export Compiled Binary". The dashboard checks the ESP image header, the product and version marker, the board (ESP32 / ESP8266), the size and downgrades before uploading.
  - Upload progress is shown, the image is verified before switching, and the page waits for the restart and confirms the new version.
  - Settings, calibration, history and logs are kept. Endpoint `POST /api/ota`.
- **Update rate profiles** (Setup → Update rate):
  - Eco (**default**): sensor every 5 s, dashboard every 10 s.
  - Balanced: 2 s / 5 s. Responsive: 0.5 s / 2 s. Custom: sensor 0.2–30 s, dashboard 2–60 s.
  - The card shows readings per hour, reaction time and fault detection time.
  - Existing devices switch to Eco after updating.
- **Live mode:** while an admin has the Setup tab open, the sensor is read every 0.5 s for calibration, returning to the profile 20 s later.
- Median window adapts to the rate (5 readings at 2 s and slower, otherwise 7). "No echo" is declared after about 15 s of missed readings (3–10 readings).
- Update rate included in settings export/import and in the report's "Settings used" page.
- Start-up banner carries a board tag (`| board esp32` / `| board esp8266`) used to validate OTA files. `/api/status` adds `rate`, `boardTag`, `otaMax`.

### Changed
- Power saving no longer sets the sensor rate; the Update rate profile does. Performance mode only affects CPU, radio, LED refresh and Serial output.

### Fixed
- Switching tabs refreshes the data immediately instead of waiting for the next scheduled refresh.
- Setup reminder banners are shown to admins only.

## [2.9.0] - 2026-09-30

### Added
- **Admin and viewer access** (`auth.h`):
  - Viewers (no login) see Overview and Analytics only. Analytics settings and Clear history are shown but disabled.
  - Setup and Log tabs, all settings changes, calibration, Wi-Fi, restart and reset need an admin login. The server rejects them with 403 for viewers.
  - Challenge-response login: the password never crosses the network, and a salted SHA-256 hash is stored in EEPROM (offset 416). Up to 3 sessions, 12 h idle timeout, 1-minute lockout after 5 wrong attempts.
  - Default password `admin`, which must be changed at the first login. Hold BOOT/FLASH for 10 s to reset it (the strip flashes red).
  - Endpoints `/api/auth`, `/api/login`, `/api/logout`, `/api/password`. `/api/status` reports `auth.admin` and `auth.mustChange`.
- **Units: mm, cm or inches** for every measurement in the dashboard and PDF report. Device default (Setup → Display & sensor, `units` setting) plus a per-browser override in the Water level card.
- **Settings export and import** (JSON) in the new **Admin & backup** card. Passwords are never exported.
- **Settings ID:** a fingerprint of the configuration, shown in the Admin card, the export file and the PDF.
- PDF report: new **"Settings used for this report"** section (page 3) listing every setting behind the figures, and the Settings ID in the details table and footer.

### Changed
- PDF footer: the GitHub source line is removed. It now shows the firmware version and Settings ID.
- **Defaults** keeps the unit setting.

## [2.8.1] - 2026-09-30

### Fixed
- ESP32: no more red `esp_littlefs ... Corrupted dir pair` / `mount failed (-84)` messages when the flash storage is blank (first upload, or "Erase All Flash Before Sketch Upload" enabled). The storage is formatted quietly and the log records "Flash storage was blank and has been formatted".

### Added
- Log line on the first start (boot #1) explaining that settings are at their defaults and how to keep them across uploads.
- README: recommended Arduino IDE Tools settings for ESP32 and NodeMCU, and troubleshooting for lost settings after an upload.

## [2.8.0] - 2026-09-30

### Added
- **Power & radio** card in Setup.
- **Power saving (production mode)**, on by default: ESP32 CPU at 80 MHz (was 240 MHz), 2 sensor reads per second (was 14), LED strip refreshed at most 4 times a second and only when a pixel changed, Wi-Fi modem sleep while the hotspot is off, a 5 ms idle pause per loop pass, no per-second Serial output, dashboard refresh every 2 s. Performance mode restores the previous behaviour.
- **Wi-Fi transmit power:** Low 8.5 dBm, Medium 13 dBm (default), High 19.5 dBm (previous behaviour).
- **Hotspot modes:** Automatic, Always on, and **On demand**. The BOOT button (ESP32) or FLASH button (NodeMCU) turns the hotspot on, and it turns off after 10 minutes without devices. With no home network and no hotspot, the radio is switched off completely.
- BOOT button press, hotspot mode and power changes are recorded in the connectivity log.
- `/api/status` fields: `apMode`, `perfMode`, `txLevel`, `cpuMhz`. `/api/settings` accepts `apMode`, `perf`, `tx`.

### Changed
- "Keep hotspot always on" switch replaced by the Hotspot setting in Power & radio. The old `apAlways` setting carries over as "Always on".
- **Defaults** keeps the power, transmit power and hotspot settings.
- README and screenshots use "NYATI" as the example society.

## [2.7.0] - 2026-09-30

### Added
- **Setup** tab with all configuration: Usage & tank, Calibration, Site details, Display & sensor, Wi-Fi and Device.
- **Usage & tank** card: Domestic / Commercial, tank location (overhead, loft / bathroom, underground sump, other), shape (rectangular, vertical cylinder), dimensions in mm, water depth when full, capacity with an estimate from the size, mounting tips per location. Stored in flash (`/api/profile`, EEPROM offset 384).
- Loft tank presets: 150, 225, 270, 400, 500 and 1000 L with their outer dimensions.
- **Recommended settings** for Domestic or Commercial use and each tank location: night leak window, low water alarm, leak limit.
- Calibration from a **current-level slider** (no need to empty or fill the tank) and from **measurements** (sensor to full water line + water depth), each with a preview and blind-zone warnings.
- Overview **Tank at a glance** card: water now, space left, capacity in litres, site, usage, tank size, water depth, calibrated range, litres per cm, and setup warnings (not calibrated, blind zone, calibration not matching the tank depth).
- Litres shown under the level percentage.
- PDF report: usage, tank size and calibration in the details table.
- Links to a card (e.g. `#cWifi`) open the tab that holds it.

### Changed
- Overview tab now shows only the level and the tank summary. Calibration, site, display, Wi-Fi and device cards moved to Setup.

## [2.6.1] - 2026-09-30

### Changed
- ESP32 sensor pins moved to **D15 (TRIG, sensor RX)** and **D2 (ECHO, sensor TX)**, so the sensor uses four neighbouring pins: 3V3, GND, D15, D2. LED strip stays on G16.
- ESP32 sensor is powered from 3V3 with the echo wired directly (no divider).
- Sensor diagnostic sketch uses the same ESP32 pins.
- ESP32 wiring diagram redrawn: four straight wires from 3V3/GND/D15/D2, and a note about the strapping pins.

### Notes
- D15 and D2 are ESP32 strapping pins. TRIG must be on D15 and ECHO on D2, not swapped: the echo resting LOW keeps D2 LOW, so USB uploads keep working.

## [2.6.0] - 2026-09-30

### Changed
- Renamed the product from "YUCCA Tank Water Level" to **Water Tanks Monitor System** (WTMS). YUCCA is now just one possible building name.
- Hotspot name: "WTMS <building> <tank>", or "WTMS-XXXX" from the chip ID when not named (was "YUCCA TANK WATER LEVEL").
- Web address: `tank-<building>-<tank>.local`, or `watertank-xxxx.local` when not named (was `yucca-tank.local`). Every tank in a society gets its own.
- Download file names: `wtms-report-<building>-<tank>-<date>.pdf`, `wtms-log-...txt`, `wtms-history.bin`.

### Added
- **Site details** card: society / organisation, building and tank names, saved in flash (EEPROM offset 256, EEPROM size now 512 bytes). Existing calibration, Wi-Fi and settings are kept.
- Site names in the dashboard header, browser tab title and footer, and a banner until the tank is named.
- PDF report: site title (society, building, tank), a site details table, and building and tank in every page header, the PDF properties and the file name.
- Connectivity log download header includes the site names and web address.
- Preview of the hotspot name and web address with a restart prompt when they change.
- `/api/site` endpoint and a `site` object in `/api/status`.

## [2.5.0] - 2026-09-30

### Added
- woodyouloveit.com branding: logo bar at the top of the dashboard (built into the firmware, works offline), page title, heart favicon, and a footer with "© 2026 Chanchal Sakarde. All Rights Reserved.", the website link, the GPL-3.0 link and a source code link.
- **Download PDF report** in the Analytics tab. It's a two-page A4 report generated in the browser with no libraries, so it works offline: logo, website, period, device and tank details, summary tiles, 7-day level chart, motor time and water used per day, usage by hour, night leak check, recent fills and method notes. Every page has the copyright, website, page number and license line. PDF properties: author Chanchal Sakarde, creator woodyouloveit.com.
- GPL-3.0 license notice with copyright and `SPDX-License-Identifier: GPL-3.0-or-later` at the top of every source file (`.ino`, `.h`, diagnostic sketch, diagram generator), plus an HTML comment in the dashboard page.
- `LICENSE` file (GNU GPL v3).
- Brand and copyright line in the Serial Monitor start-up message and in the downloaded connectivity log.
- Logo and copyright line on the wiring diagrams.
- `docs/logo.png` and `docs/sample_report.pdf`.

### Changed
- README: logo, brand and copyright at the top, new PDF report, Branding and License sections.

## [2.4.0] - 2026-09-29

### Added
- Tank analytics (`history.h`): level recorded every 2 minutes in flash (14 days, two rotating files), stored as distance so recalibration also corrects history.
- Fill (motor run) detection on the device from 10-second samples: start, end, rise and speed. Fills are saved and logged ("Filling started / stopped").
- Live filling status in `/api/status`: filling, rise speed, start time, minutes until full.
- Dashboard tabs: **Overview**, **Analytics**, **Log**.
- Analytics tab: summary tiles (now/filling, last fill, empty→full time, motor today, used today, busiest hour, last night), level history chart (24 h / 3 / 7 / 14 days) with fill and night bands and hover values, motor run time per day with recent fills, water used per day, usage by hour of day, night leak check table.
- Night leak check separates a slow, steady drop ("Possible leak") from a single drop within one hour ("Water used (one drop)").
- Leak warning for last night on the Overview tab.
- Analytics settings: tank capacity in litres, fill detection threshold, night window, leak limit, clear history.
- `/api/history`, `/api/fills`, `/api/history/clear` endpoints.
- Clock re-synced from the internet every 10 minutes on home Wi-Fi.

### Changed
- Connectivity log moved to its own tab.
- **Defaults** also keeps tank capacity (it describes the hardware).

## [2.3.1] - 2026-09-29

### Fixed
- Dashboard not loading on the hotspot while the device was retrying the saved home Wi-Fi in the background. The retry makes the ESP search other channels, which stalls hotspot traffic for up to 20 s. A background retry now stops as soon as a device joins the hotspot (logged as "Background retry stopped").

## [2.3.0] - 2026-09-29

### Added
- Connectivity log stored in flash (LittleFS, `event_log.h`), kept across restarts and power cuts: about 300–400 events in two rotating files.
- Logged events: every boot with restart cause and a boot counter, ESP8266 crash details, hotspot on/off with reason, devices joining/leaving the hotspot (MAC), hotspot channel changes, home Wi-Fi connect/lost/failed with the driver's disconnect reason, network scans, calibration and setting changes, sensor faults, main-loop stalls over 2 s.
- Health line every 5 minutes: free memory (and lowest), heap fragmentation and supply voltage on ESP8266, slowest loop, hotspot clients, Wi-Fi signal and channel.
- Real time for log entries: from NTP on home Wi-Fi, or from the browser when the dashboard is opened on the hotspot.
- Dashboard **Connectivity log** card: summary counters, filters (Wi-Fi & hotspot, warnings, restarts), auto-refresh, download as text, clear.
- **Keep hotspot always on** option in Wi-Fi settings.
- Device card: boot count, free memory, supply voltage (ESP8266), log storage.
- `/api/log`, `/api/log/clear`, `/api/time` endpoints.

### Changed
- Serial output now includes every log line prefixed with `[log]`.

## [2.2.0] - 2026-09-29

### Added
- Number of LEDs is a dashboard setting (1–300, default 30), saved in flash. The strip length changes immediately, and LEDs beyond the new length are switched off.
- Dashboard LED preview matches the configured count.
- Power estimate under the LED count: worst-case current at the current brightness and a recommended 5V supply size.

### Changed
- **Defaults** keeps the LED count, since it describes the hardware.
- Settings saved by 2.1.0 or older are kept; the LED count starts at 30.

## [2.1.0] - 2026-09-29

### Changed
- Wi-Fi is now managed by the firmware itself. The WiFiManager library and its menu page are gone.
- Connecting to the hotspot "YUCCA TANK WATER LEVEL" opens the dashboard directly (captive portal on every connectivity check URL).
- Wi-Fi setup moved into the dashboard: new **Wi-Fi settings** card with network scan, connect, show/hide password and forget.
- A new network is saved only after it connects successfully. A failed attempt keeps the previous network.
- After connecting, the dashboard shows the new home address and the hotspot turns off after 30 seconds.
- Dashboard layout: Wi-Fi settings and a separate Device card. A banner links to Wi-Fi settings until a home network is set.

### Added
- Hotspot turns on automatically when home Wi-Fi is lost or unreachable. Home Wi-Fi is retried every minute while nobody is on the hotspot.
- Cause of the last restart (power on, brownout, crash, watchdog...) shown in the Device card and on Serial.
- `/api/scan`, `/api/wifi/connect`, `/api/wifi/forget` endpoints.

### Fixed
- Hotspot dropping after a few minutes: the station interface no longer keeps searching for a network in the background while the hotspot is in use, and Wi-Fi power saving is off.

### Removed
- WiFiManager dependency, `/api/wifi` endpoint and the "Change Wi-Fi" restart flow.

## [2.0.0] - 2026-09-29

### Added
- Web dashboard for desktop and mobile browsers (`dashboard_html.h`), served by the device and working offline.
- Calibration from the dashboard: one-tap "Tank is EMPTY now" / "Tank is FULL now" or manual distances.
- Display and sensor settings from the dashboard: brightness, low water alarm, strip direction, color mode, trigger pulse, factory defaults.
- Wi-Fi setup with WiFiManager: hotspot "YUCCA TANK WATER LEVEL" with a captive portal that opens automatically.
- Wi-Fi control from the dashboard: change network, forget network, restart.
- Dashboard also reachable from the setup hotspot (`/dashboard`) for tanks without home Wi-Fi.
- mDNS name `yucca-tank.local`.
- Settings saved in flash (EEPROM), including calibration.
- Top LED blinks blue while the setup hotspot is open.
- JSON API: `/api/status`, `/api/calibrate`, `/api/settings`, `/api/wifi`, `/api/restart`.

### Changed
- Sensor reading is now non-blocking (one reading every 70ms, median of the last 7) so the web server stays responsive.
- Calibration and display options moved from constants in the sketch to saved settings.
- New library dependency: WiFiManager by tzapu.

### Fixed
- AJ-SR04M returned no echo because it ignores 10-20us trigger pulses. The trigger is now 100us (`TRIG_PULSE_US`).
- Echoes longer than 450cm (the sensor's "no target" pulse) are now treated as invalid instead of an empty tank (`MAX_VALID_CM`).

### Added
- Sensor diagnostic sketch (`tools/sensor_diagnostic/`) that checks the echo line, trigger widths and serial modes.
- Wiring diagram for NodeMCU with the AJ-SR04M powered from 3.3V and no echo divider (`docs/wiring_esp8266_3v3.svg`).
- README section for the 3.3V option, including AJ-SR04M pin mapping (RX = TRIG, TX = ECHO).

## [1.1.0] - 2026-09-29

### Added
- Support for NodeMCU ESP8266 (30-pin). The board is detected automatically from the Arduino IDE selection.
- ESP8266 pin mapping: LED on D2 (GPIO4), TRIG on D5 (GPIO14), ECHO on D6 (GPIO12).
- Compile-time error if a board other than ESP32 or ESP8266 is selected.
- Detected board name printed to Serial Monitor at startup.
- Wiring diagrams for both boards (`docs/wiring_esp32.svg`, `docs/wiring_esp8266.svg`) and the script that generates them.
- README, CHANGELOG and .gitignore.

### Changed
- Sketch moved to `water_level_indicator/water_level_indicator.ino` so the Arduino IDE can open it directly.

## [1.0.0] - 2026-09-29

### Added
- Initial ESP32 version for a JSN-SR04T / AJ-SR04M waterproof sensor and a 30-LED WS2812B strip.
- Red → yellow → green gradient bar that fills from the bottom, with a dimmed top LED for smooth movement.
- Median filter (7 samples) and smoothing of the level reading.
- Low-water alarm: bottom 3 LEDs blink red below 10%.
- Sensor fault indication: bottom LED blinks blue when there is no echo.
- `COLOR_BY_LEVEL` option to color the whole bar by level.
- `STRIP_REVERSED` option for strips mounted top-down.
- Startup sweep animation.

[Unreleased]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.10.1...HEAD
[2.10.1]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.10.0...v2.10.1
[2.10.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.9.0...v2.10.0
[2.9.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.8.1...v2.9.0
[2.8.1]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.8.0...v2.8.1
[2.8.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.7.0...v2.8.0
[2.7.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.6.1...v2.7.0
[2.6.1]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.6.0...v2.6.1
[2.6.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.5.0...v2.6.0
[2.5.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.4.0...v2.5.0
[2.4.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.3.1...v2.4.0
[2.3.1]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.3.0...v2.3.1
[2.3.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.2.0...v2.3.0
[2.2.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v1.1.0...v2.0.0
[1.1.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard/releases/tag/v1.0.0
