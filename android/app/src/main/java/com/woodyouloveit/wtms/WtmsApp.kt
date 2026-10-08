/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager

class WtmsApp : Application() {
    override fun onCreate() {
        super.onCreate()
        val channel = NotificationChannel(AlertWorker.CHANNEL, getString(R.string.channel_name), NotificationManager.IMPORTANCE_DEFAULT)
            .apply { description = getString(R.string.channel_desc) }
        getSystemService(NotificationManager::class.java)?.createNotificationChannel(channel)
        AlertWorker.schedule(this)
    }
}
