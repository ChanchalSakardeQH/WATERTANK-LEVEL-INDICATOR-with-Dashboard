# Water Tanks Monitor — Android app

Native Android app for the Water Tanks Monitor System by [woodyouloveit.com](https://woodyouloveit.com).
© 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.

## What it does

| Feature | Details |
|---|---|
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
build-android.bat -Task Release -VersionName 1.1.0 -VersionCode 2
build-android.bat -Task TestAab -Launch                 :: install the .aab the way Google Play does
build-android.bat -Task Doctor                          :: check (and fix) Java, SDK, licence, phone, key
build-android.bat -Task Debug -Yes                      :: unattended: answer yes to set-up and fix questions
build-android.bat -Task Debug -NoAutoFix                :: report problems only, change nothing
build-android.bat -Task Release -DryRun                 :: show what would run
```

**It sets itself up.** With your permission (or `-Yes`) the script installs what is missing:

| Missing | What it does |
|---|---|
| Java 17–23 | Uses Android Studio's own Java if present; otherwise downloads Java 17 (Eclipse Temurin, checksum-verified) to `%USERPROFILE%\.woodyouloveit\jdk-17` |
| Android SDK | Downloads Google's command-line tools, asks you to accept the SDK licence, installs platform-tools, the Android platform this app compiles for (read from `compileSdk`, now API 35) and build tools |
| SDK licence | Shows it and records your acceptance |
| adb (phone install) | Installs platform-tools |

**It fixes common build failures and tries again:** licence not accepted, missing build tools or platform, SDK not found,
wrong Java version, damaged Gradle download, stuck Gradle background processes, out of memory (raises Gradle to 4 GB),
and network hiccups. Disk full, very long paths and HTTPS-inspecting proxies are explained with what to do.
A real code error is reported with the log to send.

The Gradle version (8.13) and the Android platform are read from the project, so the script follows the project when they change.
The first build downloads Gradle and the Android libraries (a few hundred MB).

Outputs go to `release\android\<version>\` (`woodyouloveit-wtms-<version>-debug.apk`, `-release.apk`, `-release.aab`) with `SHA256SUMS.txt`, and a log to `release\logs\` (passwords never appear in it).

The release key is kept outside the project in `%USERPROFILE%\.woodyouloveit\wtms-release.jks`. **Back it up with its password**:
Google Play only accepts updates signed with the same key. `-RememberPassword` stores the password encrypted for your Windows account only.

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
