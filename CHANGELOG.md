# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
