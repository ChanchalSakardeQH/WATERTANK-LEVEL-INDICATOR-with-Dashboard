// Water Tanks Monitor System - Android app module
// Copyright (C) 2026 Chanchal Sakarde - woodyouloveit.com
// SPDX-License-Identifier: GPL-3.0-or-later
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

import java.util.Properties

// Release signing comes from environment variables set by tools/build-android.ps1 (never stored in the project)
val keystoreFile: String? = System.getenv("ANDROID_KEYSTORE_FILE")

// Version: android/version.properties (the build menu updates it); -PappVersionName / -PappVersionCode override
val versionProps = Properties().apply {
    val f = rootProject.file("version.properties")
    if (f.exists()) f.inputStream().use { load(it) }
}
val appVersionName: String = (project.findProperty("appVersionName") as String?) ?: versionProps.getProperty("versionName", "1.2.0")
val appVersionCode: Int = (project.findProperty("appVersionCode") as String?)?.toIntOrNull()
    ?: versionProps.getProperty("versionCode", "3").toInt()

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

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}

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
