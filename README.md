# YUCCA Tank Water Level

A tank water level indicator using a waterproof ultrasonic sensor, a WS2812B LED strip (1–300 LEDs) and a built-in web dashboard.
The strip fills from the bottom as the water rises: **red at the bottom, fading through yellow to green at the top**.

One sketch runs on both **ESP32 DevKit V1 (30-pin)** and **NodeMCU ESP8266 (30-pin)**. The board is detected automatically from the Arduino IDE board selection.

## Features

- **Web dashboard** for desktop and mobile browsers: live level, calibration, display settings, Wi-Fi settings
- **Wi-Fi setup hotspot** "YUCCA TANK WATER LEVEL": connect and the dashboard opens automatically (captive portal)
- **Wi-Fi settings in the dashboard**: scan, connect, forget. The hotspot comes back by itself if home Wi-Fi is unreachable.
- **One-tap calibration** from the dashboard ("Tank is EMPTY now" / "Tank is FULL now") or by typing distances
- **Connectivity log** saved in flash: restarts and their cause, Wi-Fi and hotspot events, sensor faults, health checks
- Settings saved in flash, kept after restarts and power cuts
- LEDs keep showing the level while the setup hotspot is open
- Works on ESP32 and ESP8266 without code changes
- Red → yellow → green gradient bar, with the top LED partially dimmed for smooth movement
- Median filter (7 samples) plus smoothing to ignore ripples and stray echoes
- Low-water alarm: the bottom 3 LEDs blink red below 10%
- Sensor fault indication: the bottom LED blinks blue. A failed reading never shows a false "full" tank.
- Optional mode where the whole bar changes color with the level
- Serial output of distance and level for calibration

## Hardware

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32 DevKit V1 (30-pin) **or** NodeMCU ESP8266 (30-pin) | |
| 1 | JSN-SR04T or AJ-SR04M waterproof ultrasonic sensor | Driver board + probe |
| 1 | WS2812B LED strip, 30 or 60 LEDs (1–300 supported) | 5V version. Set the count in the dashboard. |
| 1 | 5V power supply: 2A for 30 LEDs, 3A for 60 LEDs | Powers the board, strip and sensor. The dashboard shows the estimate for your setup. |
| 1 | 330Ω resistor | In series with the LED data line |
| 1 | 1kΩ resistor | Echo voltage divider |
| 1 | 2kΩ resistor | Echo voltage divider |
| 1 | 1000µF capacitor, 10V or higher | Across the strip's 5V and GND |
| 2 | 15-pin female headers | Makes the board removable |
| 3 | Screw terminals (4-pin, 3-pin, 2-pin) | Sensor, LED strip, power |
| 1 | Perfboard, about 7×9 cm | |

## Wiring

### Pin summary

| Connection | ESP32 DevKit | NodeMCU ESP8266 | Notes |
|---|---|---|---|
| LED strip DIN | G16 | D2 (GPIO4) | Through the 330Ω resistor |
| Sensor TRIG | G17 | D5 (GPIO14) | Direct |
| Sensor ECHO | G18 | D6 (GPIO12) | Through the 1k/2k divider |
| +5V | VIN | VIN | From the 5V supply |
| GND | GND | GND | All grounds connected together |

### ESP32 DevKit V1 (30-pin)

![ESP32 wiring diagram](docs/wiring_esp32.svg)

### NodeMCU ESP8266 (30-pin)

![NodeMCU ESP8266 wiring diagram](docs/wiring_esp8266.svg)

### NodeMCU ESP8266 with AJ-SR04M on 3.3V (no divider)

The AJ-SR04M is rated for 3.0–5.5V. Powered from the NodeMCU's 3V3 pin, its echo output is 3.3V, so it can connect directly to D6 with no resistors. On this board, **RX = TRIG** and **TX = ECHO**. Range may be shorter than at 5V, so test at your tank's full depth.

![NodeMCU ESP8266 3.3V wiring diagram](docs/wiring_esp8266_3v3.svg)

> **Warning:** In this setup the sensor's 5V pin must go to **3V3 only**. Connecting it to the 5V supply would put 5V straight into D6.

### Why the extra parts

- **1k/2k divider on ECHO:** The sensor's echo output is 5V, but both boards are 3.3V chips. The divider brings the signal down to about 3.3V.
- **330Ω on DIN:** Reduces ringing on the data line and protects the first LED. Place it close to the strip.
- **1000µF capacitor:** Absorbs the current surge when the strip switches on. Mind the polarity: the striped side goes to GND.
- **External 5V supply:** Each LED can draw up to 60mA at full white, so 60 LEDs can need over 3A. The level display uses less (red/green only), and the dashboard shows an estimate for your LED count and brightness. Don't power the strip from the board's USB port.

> **ESP8266 note:** Avoid D3, D4 and D8 for external wiring. The board reads them at boot, and anything connected there can stop it from starting.

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add the board package for your board:
   - **ESP32:** In Boards Manager, install "esp32" by Espressif.
   - **ESP8266:** In Preferences → Additional Boards Manager URLs, add
     `http://arduino.esp8266.com/stable/package_esp8266com_index.json`,
     then install "esp8266" from Boards Manager.
3. Install **Adafruit NeoPixel** from Library Manager. Wi-Fi, web server, DNS and mDNS come with the board package.
   The WiFiManager library used by 2.0.0 is no longer needed.
4. Open `water_level_indicator/water_level_indicator.ino`. The dashboard page (`dashboard_html.h`) opens as a second tab.
5. Select the board in **Tools → Board**:
   - ESP32: **ESP32 Dev Module**
   - ESP8266: **NodeMCU 1.0 (ESP-12E Module)**, with **Flash Size: 4MB (FS:2MB OTA:~1019KB)** so the log can be saved in flash
6. Upload, then open Serial Monitor at **115200 baud**. The first line shows the firmware version and board.

## First-time Wi-Fi setup

1. Power the device. With no home Wi-Fi saved, it opens the hotspot **YUCCA TANK WATER LEVEL** and the top LED blinks blue.
2. Connect your phone or laptop to that hotspot. The dashboard opens automatically.
   If it doesn't, open a browser and go to `http://192.168.4.1`.
3. In **Wi-Fi settings**, press **Scan**, pick your network, enter the password and press **Connect**.
4. The dashboard shows the new address (`http://yucca-tank.local` or the IP). The hotspot turns off 30 seconds later.
   Switch your phone or PC back to your home Wi-Fi and open that address.

If the device can't reach your home Wi-Fi (router off, wrong password, moved too far), the hotspot turns back on automatically.
The device retries your home network every minute while nobody is connected to the hotspot.

No home Wi-Fi at the tank? Just leave it on the hotspot and use `http://192.168.4.1`.

## Dashboard

Open the dashboard from any device on the same network:

- `http://yucca-tank.local` (Windows, macOS, iOS, Linux; some Android phones don't support `.local`)
- or the device's IP address, shown in Serial Monitor and in your router's device list

| Desktop | Mobile |
|---|---|
| ![Dashboard on desktop](docs/dashboard_desktop.png) | ![Dashboard on mobile](docs/dashboard_mobile.png) |

| Card | What it does |
|---|---|
| Water level | Live level, tank graphic and LED strip preview. Shows sensor fault and low water alerts. |
| Calibration | Live sensor reading, one-tap EMPTY / FULL buttons, or type both distances |
| Display & sensor | LED brightness, low water alarm, strip direction, color mode, trigger pulse. **Defaults** restores factory settings. |
| Wi-Fi settings | Saved network, status, signal, home address and hotspot state. **Scan** lists nearby networks, **Connect** joins one (saved only if it works), **Forget Wi-Fi** erases the saved network. |
| Device | Board, firmware, uptime, cause of the last restart, boot count, free memory, supply voltage (ESP8266), **Restart device**. |
| Connectivity log | Every restart, Wi-Fi and hotspot event with time, filters, **Download** and **Clear**. See below. |

## Configuration

All of these settings can be changed from the dashboard and are saved in flash. The table shows the factory defaults.

| Setting | Default | Description |
|---|---|---|
| Number of LEDs | `30` | 1–300. Kept when you press **Defaults**. |
| Empty distance | `120` cm | Distance from the sensor to the water surface when the tank is empty |
| Full distance | `25` cm | Distance when the tank is full. Keep this at 20–25 or more (sensor blind zone). |
| LED brightness | `150` | 5–255 |
| Strip mounted top-down | off | Turn on if LED #1 is at the top of the tank |
| Whole bar one color | off | On: the whole bar is red when low and green when full |
| Low water alarm | `10` % | Level below which the bottom LEDs blink red |
| Trigger pulse | `100` | Trigger pulse length in µs. The AJ-SR04M ignores 10–20µs pulses. |

Fixed in the sketch: `MAX_VALID_CM` (450), readings above it are treated as "no target". The hotspot name (`AP_NAME`) and hostname (`HOSTNAME`) are also defined at the top of the sketch.

The pin numbers for each board are in the `#if defined(ESP32)` / `#elif defined(ESP8266)` block.

### Calibration

1. Mount the sensor at the top of the tank, pointing straight down, with no pipes or walls in its beam.
2. Open the dashboard. When the tank is empty, press **Tank is EMPTY now**. When it's full, press **Tank is FULL now**.
3. If you can't wait for the tank to empty or fill, measure the distances from the sensor face to the bottom and to the full line, type them in and press **Save distances**.

The sensor must sit at least 20–25cm above the full water line.

## How it works

```
level = (empty distance - distance) / (empty distance - full distance)    clamped to 0..1
LEDs lit = level × number of LEDs, counting from the bottom of the strip
LED color = its position on the strip: bottom red → middle yellow → top green
```

The sensor is read every 70ms and the median of the last 7 readings is used. The level is then blended with the previous value so the bar moves smoothly. Ten misses in a row show the sensor fault blink.

## Connectivity log

The device records events in flash, so they survive restarts and power cuts (about 300–400 events, oldest dropped first).
Open the **Connectivity log** card in the dashboard, or download it as a text file to share.

| Event | What it tells you |
|---|---|
| `Boot #N ... restart cause: ...` | Every start. **Power on** means power was cut or dipped; **Watchdog**, **Exception**, **Crash** or **Brownout** mean a problem (shown in red). |
| `Supply voltage` (ESP8266) | The board's 3.3V rail at boot and every 5 minutes. Below 3.0V means the supply is struggling. |
| `Hotspot on / off (reason)` | When and why the hotspot changed. "connected to home Wi-Fi" is normal. |
| `Device joined / left hotspot` | Phones and PCs connecting to the hotspot, by MAC address. |
| `Hotspot channel changed` | The ESP8266 hotspot must use the same channel as the home Wi-Fi. When it changes, devices on the hotspot are disconnected briefly. |
| `Home Wi-Fi lost / disconnect event` | With the reason, e.g. "signal lost (beacon timeout)" or "wrong password?". |
| `Health` (every 5 min) | Free memory, supply voltage, slowest main loop, hotspot clients, Wi-Fi signal and channel. |
| `Main loop was blocked` | Something stalled the firmware for more than 2 s. |
| `Sensor fault / Sensor OK` | Ultrasonic sensor stopped or started answering (at most once per 30 s). |

Times come from the internet when on home Wi-Fi, or from your browser when you open the dashboard on the hotspot.
Before that, events show as `Boot #N +h:mm:ss` (time since that start).

**Reading a "hotspot disappears" problem**
- Many `Boot #` lines with **Power on** and a low supply voltage: the board is resetting from power dips. Use a 5V supply that can handle the LED strip, not USB.
- `Hotspot off (connected to home Wi-Fi)`: expected. Turn on **Keep hotspot always on** in Wi-Fi settings if you want it to stay.
- `Hotspot channel changed`: the device is joining or retrying your home Wi-Fi on another channel.
- `Home Wi-Fi lost: signal lost`: weak signal at the tank. The device switches to the hotspot until the home Wi-Fi is back.

## LED indications

| Display | Meaning |
|---|---|
| Red → green sweep at power-up | Startup test |
| Top LED blinking blue | Hotspot "YUCCA TANK WATER LEVEL" is on |
| Bar from the bottom | Current water level |
| Bottom 3 LEDs blinking red | Water below 10% |
| Bottom LED blinking blue | No echo from the sensor. Check the wiring, the divider and the probe position. |

## Troubleshooting

| Problem | Things to check |
|---|---|
| LEDs flicker or show random colors | Common ground between the supply, board and strip; the 330Ω resistor; the capacitor. A 74AHCT125 level shifter on DIN helps with long wires. |
| Blue blink (no echo) | ECHO divider wiring; sensor powered from 5V; probe too close to the water (inside the 25cm blind zone) |
| Level jumps around | Probe not vertical, or the beam hits the tank wall or pipes |
| Only part of the strip lights up at full level | Set **Number of LEDs on the strip** in Display & sensor settings to match your strip. |
| Bar fills from the wrong end | Turn on **Strip mounted top-down** in the dashboard |
| Blue blink with an AJ-SR04M | Run `tools/sensor_diagnostic` to check the wiring and find a working trigger width |
| Dashboard doesn't open by itself on the hotspot | Open a browser and go to `http://192.168.4.1`. On Android, tap the "Sign in to network" notification. |
| Hotspot connects but the page never loads | Type `http://192.168.4.1` exactly (with `http://`). Other sites and `https://` addresses can't be redirected. On a phone, turn off mobile data while setting up. Check Serial Monitor: repeating `Boot #` lines mean the board keeps restarting. |
| Hotspot disappears | Open the **Connectivity log**. See "Reading a hotspot disappears problem" above. |
| Device card says "Log storage: Memory only" | On ESP8266, select **Tools → Flash Size → 4MB (FS:2MB OTA:~1019KB)** and upload again. |
| `yucca-tank.local` doesn't open | Use the IP address instead. Some Android phones don't support `.local` names. |
| Dashboard unreachable after changing the router | Wait about 20 seconds: the hotspot turns on by itself. Connect to it and choose the new network in Wi-Fi settings. |
| ESP8266 won't boot with the circuit connected | Make sure nothing is connected to D3, D4 or D8 |

## Project structure

```
water-level-indicator/
├── water_level_indicator/
│   ├── water_level_indicator.ino   # Sketch for ESP32 and ESP8266
│   ├── dashboard_html.h            # Web dashboard page (HTML/CSS/JS)
│   └── event_log.h                 # Connectivity log stored in flash (LittleFS)
├── docs/
│   ├── wiring_esp32.svg            # ESP32 wiring diagram
│   ├── wiring_esp8266.svg          # NodeMCU wiring diagram
│   ├── wiring_esp8266_3v3.svg      # NodeMCU, sensor on 3.3V, no divider
│   ├── wiring_*.png                # PNG versions of the diagrams
│   ├── dashboard_*.png             # Dashboard screenshots
│   └── gen_diagrams.py             # Regenerates the diagrams
├── CHANGELOG.md
├── README.md
└── .gitignore
```

