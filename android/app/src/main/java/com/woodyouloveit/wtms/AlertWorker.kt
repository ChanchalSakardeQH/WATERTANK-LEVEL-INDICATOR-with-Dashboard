/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.Manifest
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import java.util.concurrent.TimeUnit

/** Which alerts the user wants. */
object AlertSettings {
    private const val P = "alerts"
    private fun prefs(c: Context) = c.getSharedPreferences(P, Context.MODE_PRIVATE)
    fun enabled(c: Context) = prefs(c).getBoolean("enabled", true)
    fun low(c: Context) = prefs(c).getBoolean("low", true)
    fun fault(c: Context) = prefs(c).getBoolean("fault", true)
    fun offline(c: Context) = prefs(c).getBoolean("offline", false)     // off by default: hotspot-only tanks are often out of reach
    fun save(c: Context, enabled: Boolean, low: Boolean, fault: Boolean, offline: Boolean) {
        prefs(c).edit().putBoolean("enabled", enabled).putBoolean("low", low).putBoolean("fault", fault).putBoolean("offline", offline).apply()
    }
}

/**
 * Runs about every 15 minutes (the shortest interval Android allows), checks every saved tank
 * and notifies once per event: low water, sensor fault, unreachable for 1 hour, and recovery.
 */
class AlertWorker(ctx: Context, params: WorkerParameters) : CoroutineWorker(ctx, params) {

    companion object {
        const val CHANNEL = "tank_alerts"
        private const val WORK = "tank-alerts"
        private const val OFFLINE_AFTER_MS = 60 * 60 * 1000L

        fun schedule(ctx: Context) {
            val wm = WorkManager.getInstance(ctx)
            if (!AlertSettings.enabled(ctx)) { wm.cancelUniqueWork(WORK); return }
            val req = PeriodicWorkRequestBuilder<AlertWorker>(15, TimeUnit.MINUTES)
                .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
                .build()
            wm.enqueueUniquePeriodicWork(WORK, ExistingPeriodicWorkPolicy.KEEP, req)
        }
    }

    override suspend fun doWork(): Result {
        val ctx = applicationContext
        if (!AlertSettings.enabled(ctx)) return Result.success()
        val state = ctx.getSharedPreferences("alert_state", Context.MODE_PRIVATE)
        val now = System.currentTimeMillis()

        for ((index, d) in DeviceStore.load(ctx).withIndex()) {
            val s = StatusClient.fetchAny(ctx, d, 8000).status
            val key = d.id
            val name = s.title.ifBlank { d.name.ifBlank { d.address } }
            val ed = state.edit()

            if (s.reachable) {
                ed.putLong("$key.lastOk", now)
                if (state.getBoolean("$key.offline", false)) {
                    ed.putBoolean("$key.offline", false)
                    if (AlertSettings.offline(ctx)) notify(ctx, index * 10 + 1, "$name is back online", levelText(s))
                }
                // Low water (re-armed once the level is 5 % above the alarm)
                if (s.low && !state.getBoolean("$key.low", false)) {
                    ed.putBoolean("$key.low", true)
                    if (AlertSettings.low(ctx)) notify(ctx, index * 10 + 2,
                        "Low water: $name" + if (s.multi) " (" + s.lowNames.joinToString(", ") + ")" else "", levelText(s))
                } else if (s.valid && s.level >= s.lowAlarm + 5) ed.putBoolean("$key.low", false)
                // Sensor fault
                if (!s.valid && !state.getBoolean("$key.fault", false)) {
                    ed.putBoolean("$key.fault", true)
                    if (AlertSettings.fault(ctx)) notify(ctx, index * 10 + 3, "Sensor fault: $name", "The level sensor is not answering (no echo). Check the probe and its wiring.")
                } else if (s.valid) ed.putBoolean("$key.fault", false)
            } else {
                val lastOk = state.getLong("$key.lastOk", now)
                if (!state.contains("$key.lastOk")) ed.putLong("$key.lastOk", now)
                if (now - lastOk > OFFLINE_AFTER_MS && !state.getBoolean("$key.offline", false)) {
                    ed.putBoolean("$key.offline", true)
                    if (AlertSettings.offline(ctx)) notify(ctx, index * 10 + 1, "$name is not reachable", "No answer for over an hour. Check its power and Wi-Fi.")
                }
            }
            ed.apply()
        }
        return Result.success()
    }

    private fun levelText(s: TankStatus): String = if (s.multi) s.compartments.joinToString(" · ") {
            if (it.valid) "${it.name} ${Math.round(it.level)}%" + (if (it.capacity > 0) " (${it.litres} L)" else "") else "${it.name}: no echo"
        } else levelText1(s)

    private fun levelText1(s: TankStatus): String =
        if (!s.valid) "Sensor fault (no echo)"
        else "Level ${Math.round(s.level)}%" + (if (s.capacity > 0) " (≈ ${s.litres} L of ${s.capacity} L)" else "") +
            (if (s.filling) ", filling" else "")

    private fun notify(ctx: Context, id: Int, title: String, text: String) {
        if (Build.VERSION.SDK_INT >= 33 &&
            ContextCompat.checkSelfPermission(ctx, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) return
        val open = PendingIntent.getActivity(ctx, 0, Intent(ctx, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)
        val n = NotificationCompat.Builder(ctx, CHANNEL)
            .setSmallIcon(R.drawable.ic_notification)
            .setColor(ContextCompat.getColor(ctx, R.color.brand_teal))
            .setContentTitle(title)
            .setContentText(text)
            .setStyle(NotificationCompat.BigTextStyle().bigText(text))
            .setContentIntent(open)
            .setAutoCancel(true)
            .build()
        try { NotificationManagerCompat.from(ctx).notify(id, n) } catch (_: SecurityException) {}
    }
}
