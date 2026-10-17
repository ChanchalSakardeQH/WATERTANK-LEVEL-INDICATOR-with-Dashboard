# Firmware for the Android app

Exported firmware placed here is packed into the Water Tanks Monitor app automatically when the app is built,
so the manufacturer can update tanks in the field from the app (Firmware update, service login).

1. Once: create the manufacturer service login with `tools\new-service-key.bat` (writes `water_level_indicator\service_secret.h`,
   which is never committed). Firmware built without it has no service login and cannot be updated over Wi-Fi.
2. Arduino IDE: open `water_level_indicator\water_level_indicator.ino`, choose the board (ESP32 Dev Module and/or NodeMCU 1.0),
   then **Sketch → Export Compiled Binary**.
3. Copy `water_level_indicator.ino.bin` from the sketch's `build\<board>\` folder into this folder (any name, e.g. `wtms-esp32.bin`).
   The app build also finds it directly in the sketch's `build` folder.
4. Build the app (`build-android.bat`). The log shows `Firmware bundled in the app: esp32 3.3.0, esp8266 3.3.0`.

The newest version per board is used; each image is identified by the version and board marker in the firmware itself and
checked with SHA-256 in the app. `.bootloader`, `.partitions` and `.merged` files are ignored.
