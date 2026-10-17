import java.util.Properties
import java.security.MessageDigest

// Water Tanks Monitor System - Android app module
// Copyright (C) 2026 Chanchal Sakarde - woodyouloveit.com
// SPDX-License-Identifier: GPL-3.0-or-later
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

// Release signing comes from environment variables set by tools/build-android.ps1 (never stored in the project)
val keystoreFile: String? = System.getenv("ANDROID_KEYSTORE_FILE")

// Version: android/version.properties (the build menu updates it); -PappVersionName / -PappVersionCode override
val versionProps = Properties().apply {
    val f = rootProject.file("version.properties")
    if (f.exists()) f.inputStream().use { load(it) }
}
val appVersionName: String = (project.findProperty("appVersionName") as String?) ?: versionProps.getProperty("versionName", "1.4.0")
val appVersionCode: Int = (project.findProperty("appVersionCode") as String?)?.toIntOrNull()
    ?: versionProps.getProperty("versionCode", "5").toInt()

// ---------------------------------------------------------------- firmware bundled in the app
// Every exported Water Tanks Monitor firmware found in firmware/*.bin (project root) or in the sketch's build folder
// (Arduino IDE: Sketch > Export Compiled Binary) is packed into the app with a manifest, newest version per board.
// The app identifies each image by the marker the firmware prints at start-up ("... 3.3.0 | woodyouloveit.com | board esp32").
fun versionCompare(a: String, b: String): Int {
    val x = a.split('.').map { it.toIntOrNull() ?: 0 }
    val y = b.split('.').map { it.toIntOrNull() ?: 0 }
    for (i in 0 until maxOf(x.size, y.size)) {
        val d = x.getOrElse(i) { 0 } - y.getOrElse(i) { 0 }
        if (d != 0) return d
    }
    return 0
}
val firmwareAssets = layout.buildDirectory.dir("generated/firmwareAssets")
val bundleFirmware by tasks.registering {
    description = "Packs exported Water Tanks Monitor firmware (.bin) into the app"
    val repo = rootProject.projectDir.parentFile
    val sources = fileTree(repo) { include("firmware/*.bin", "water_level_indicator/build/**/water_level_indicator.ino.bin") }
    inputs.files(sources)
    outputs.dir(firmwareAssets)
    doLast {
        val outDir = firmwareAssets.get().asFile.resolve("firmware")
        outDir.deleteRecursively()
        outDir.mkdirs()
        val marker = Regex("Water Tanks Monitor System (\\d+\\.\\d+\\.\\d+) \\| woodyouloveit\\.com \\| board (esp32|esp8266)")
        val best = mutableMapOf<String, Pair<String, java.io.File>>()          // board -> (version, file)
        for (f in sources.files) {
            val bytes = f.readBytes()
            if (bytes.size < 1024 || bytes[0] != 0xE9.toByte()) continue
            val m = marker.find(String(bytes, Charsets.ISO_8859_1)) ?: continue
            val version = m.groupValues[1]
            val board = m.groupValues[2]
            val cur = best[board]
            if (cur == null || versionCompare(version, cur.first) > 0) best[board] = version to f
        }
        val entries = best.map { (board, vf) ->
            val name = "wtms-$board-${vf.first}.bin"
            vf.second.copyTo(outDir.resolve(name), overwrite = true)
            val sha = MessageDigest.getInstance("SHA-256").digest(vf.second.readBytes()).joinToString("") { byte -> "%02x".format(byte.toInt() and 0xff) }
            "{\"board\":\"$board\",\"version\":\"${vf.first}\",\"file\":\"$name\",\"size\":${vf.second.length()},\"sha256\":\"$sha\"}"
        }
        outDir.resolve("manifest.json").writeText("[" + entries.joinToString(",") + "]")
        logger.lifecycle("Firmware bundled in the app: " +
            (if (best.isEmpty()) "none (export the firmware and copy the .bin into firmware/)" else best.entries.joinToString { "${it.key} ${it.value.first}" }))
    }
}

android {
    namespace = "com.woodyouloveit.wtms"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.woodyouloveit.wtms"
        minSdk = 26
        targetSdk = 35
        versionName = appVersionName
        versionCode = appVersionCode
        manifestPlaceholders["appLabel"] = "Water Tanks Monitor"
    }

    signingConfigs {
        if (keystoreFile != null) {
            create("release") {
                storeFile = file(keystoreFile)
                storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
                keyAlias = System.getenv("ANDROID_KEY_ALIAS")
                keyPassword = System.getenv("ANDROID_KEY_PASSWORD")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            if (keystoreFile != null) signingConfig = signingConfigs.getByName("release")
        }
        debug {
            // Installs beside the release app as "WTMS Debug" (the build script launches com.woodyouloveit.wtms.debug)
            applicationIdSuffix = ".debug"
            versionNameSuffix = "-debug"
            manifestPlaceholders["appLabel"] = "WTMS Debug"
        }
    }

    buildFeatures { buildConfig = true }

    sourceSets.getByName("main").assets.srcDir(firmwareAssets.get().asFile)

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}

tasks.named("preBuild") { dependsOn(bundleFirmware) }

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("androidx.activity:activity-ktx:1.9.3")
    implementation("com.google.android.material:material:1.12.0")
    implementation("androidx.recyclerview:recyclerview:1.3.2")
    implementation("androidx.swiperefreshlayout:swiperefreshlayout:1.1.0")
    implementation("androidx.coordinatorlayout:coordinatorlayout:1.2.0")
    implementation("androidx.webkit:webkit:1.12.1")
    implementation("androidx.work:work-runtime-ktx:2.9.1")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.7")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")
}
