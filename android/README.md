# Water Tanks Monitor — Android app

Native Android app for the Water Tanks Monitor System by [woodyouloveit.com](https://woodyouloveit.com).
© 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.

## What it does

| Feature | Details |
|---|---|
| First start | While no tank is saved: an animated save-water mascot (a smiling water drop that bounces, blinks and sends up hearts), the woodyouloveit.com logo, a pulsing "Every drop counts" and rotating water-saving tips, with *Set up my first tank* and *Find tanks on this Wi-Fi*. Still when the phone's animations are off |
| Compartments | Tanks with several sensors (firmware 3.0.0) show the whole tank plus a bar per compartment; alerts name the compartment |
| Admin login | Per tank, in the dashboard menu: logs in with the tank's own admin password (challenge, the password is never sent), asks for a new password on a tank still using the default, and unlocks Setup and Log in the dashboard. Only the session token is kept; it is checked against the tank every time and dropped after the tank restarts or 12 h without use. Viewers without the password see Overview and Analytics only |
| Automatic discovery | On Wi-Fi, when a saved tank doesn't answer or none are saved, the app searches the network by itself: tanks whose router address changed are updated, new tanks are offered with an Add button |
| Set up a new tank | Wizard: connect to the tank's hotspot (Android shows the list of WTMS Wi-Fi) → admin login, replacing the default password → names → pick the home or society router **from the tank's own scan** → the tank joins and is saved under its new address |
| Open a tank's hotspot | View any tank through its WTMS Wi-Fi without setting it up; mobile data stays on for other apps |
| Remote address | Optional per tank (Cloudflare Tunnel https:// link or a VPN/Tailscale address); used automatically when the home address doesn't answer |
| Tank list | Every saved tank with level %, litres, filling / low water / sensor fault, refreshed on open and by pulling down |
| Find tanks | Searches the Wi-Fi (mDNS) for tank devices and checks the device hotspot address 192.168.4.1; updates saved tanks whose IP changed |
| Dashboard | Opens the device's own dashboard (Overview, Analytics, Setup, Log), so every firmware feature appears in the app automatically |
| Real file sharing | PDF report, CSV data, log and settings backup go to WhatsApp, email or the share sheet **with the file attached** (needs firmware 2.12.0) |
| Downloads | Saved to Downloads/WTMS |
| Hotspot friendly | Traffic to local tank addresses is sent over Wi-Fi even when that Wi-Fi has no internet |
| Notifications | About every 15 min: low water, sensor fault, and (optional) tank not reachable for 1 hour, each once per event |

Requirements: Android 8.0 or newer. Hotspot selection uses Android's own Wi-Fi picker on Android 10+; on Android 8–9 the app lists the hotspots itself (location permission needed for Wi-Fi scanning). The app talks to tank devices on the local network (their hotspot or the home Wi-Fi),
or through a VPN / Cloudflare Tunnel address for remote access.

## Setting up a tank with the app

1. **Add tank → Set up a new tank** and press **Connect**. Android lists the nearby tank Wi-Fi (`WTMS-…`); pick one.
   No tank listed? Press the BOOT button on the board to turn its hotspot on.
2. Log in with the tank's admin password. A new tank uses `admin` and asks for your own password straight away.
3. Optionally name it (society, building, tank).
4. Pick the home or society Wi-Fi from the list the **tank** scanned, enter its password, press **Connect tank**.
5. The app waits until the tank joins, switches the phone back to its normal Wi-Fi, and saves the tank under its new address.
   If you changed the names, the tank restarts once to use them.

## Using the app away from home (internet)

The tank only serves its dashboard on the local network. To reach it from anywhere, give it a secure route and enter that as the tank's **remote address**
(long-press the tank → *Rename, address or remote address*):

| Route | What to install | Remote address to enter |
|---|---|---|
| **Tailscale** (VPN, recommended for admins) | Tailscale on an always-on home PC or Raspberry Pi as a *subnet router*, and the Tailscale app on the phone | nothing extra: the home IP works through the VPN (the app detects the VPN and routes through it) |
| **Cloudflare Tunnel** (for sharing with residents) | `cloudflared` on an always-on home PC or Raspberry Pi, a domain on Cloudflare, Cloudflare Access login | e.g. `https://yucca-tank1.woodyouloveit.com` |

The app tries the home address first and falls back to the remote one; the tank card then shows *remote*. Do not open the tank to the internet with router port forwarding: the dashboard uses plain HTTP.

## Building on Windows

From the project root, double-click **`build-android.bat`** for the menu, or run:

```bat
build-android.bat -Task Debug -Install -Launch          :: debug APK, install on the USB phone, start it
build-android.bat -Task Keystore                        :: first time only: create the release signing key
build-android.bat -Task Release                         :: asks for the new version, then builds the signed APK + AAB
build-android.bat -Task Release -VersionName 1.2.0 -VersionCode 3
build-android.bat -Task TestAab -Launch                 :: install the .aab the way Google Play does
build-android.bat -Task Doctor                          :: check and fix Java, SDK, licence, phone, key
build-android.bat -Task Debug -Yes                      :: unattended: answer yes to set-up and fix questions
build-android.bat -Task Debug -NoAutoFix                :: report problems only, change nothing
```

| Item | Where |
|---|---|
| Your Android folder | Found by itself by walking up from the project (e.g. `D:\Android` with `Sdk`, `Android Studio` or `Gradle`); `SM_ANDROID_ROOT` overrides it |
| SDK, Gradle cache, Java 21, tools | Inside that folder (`Sdk`, `Gradle`, `jdk-21`, `Tools`), or the standard Windows folders without one |
| Release key | `<Android folder>\Keys\wtms-release.jks`. A key made by the earlier script in `%USERPROFILE%\.woodyouloveit` is used if it is there: **back it up with its password**, Google Play only accepts updates signed with the same key |
| App version | `android\version.properties` (`versionName`, `versionCode`); the menu asks for a new one before a release |
| Debug build | Installs beside the release app as **WTMS Debug** (`com.woodyouloveit.wtms.debug`) |
| Outputs | `release\android\<version>\woodyouloveit-wtms-<version>-debug.apk`, `-release.apk`, `-release.aab`, `SHA256SUMS.txt`; logs in `release\logs\` |

**It sets itself up:** with your permission (or `-Yes`) it installs a missing Java 21, the Android SDK command-line tools,
the SDK licence, platform-tools, the Android platform this app compiles for (read from `compileSdk`, API 35) and build tools.
**It fixes common build failures and builds again:** licence, missing packages, SDK not found, wrong Java (Gradle 8.14.3 runs on Java 17–24),
damaged Gradle download, stuck Gradle processes, out of memory, network hiccups. Disk full, long paths and HTTPS-inspecting proxies are explained.

The project also opens directly in Android Studio (`android\` folder).

## Project layout

```
android/
├── app/src/main/java/com/woodyouloveit/wtms/
│   ├── MainActivity.kt        tank list, add, find, notifications, about
│   ├── SetupActivity.kt       new-tank wizard (hotspot → login → names → router Wi-Fi)
│   ├── HotspotLink.kt         connects the phone to a tank hotspot (system picker on Android 10+)
│   ├── TankApi.kt             tank admin API: challenge login, password, scan, Wi-Fi, names
│   ├── DashboardActivity.kt   device dashboard (WebView), Wi-Fi routing, files and sharing
│   ├── AppBridge.kt           window.WTMSApp used by the dashboard for files and sharing
│   ├── AlertWorker.kt         background checks and notifications
│   ├── Devices.kt             saved tanks and /api/status client
│   ├── Discovery.kt           mDNS discovery
│   ├── NetUtil.kt             Wi-Fi routing for local addresses
│   ├── DeviceAdapter.kt       tank list cards
│   └── WtmsApp.kt             notification channel, scheduling
├── app/build.gradle.kts       version, signing from environment variables
└── gradlew(.bat), gradle/wrapper
```
