/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.wifi.WifiConfiguration
import android.net.wifi.WifiManager
import android.net.wifi.WifiNetworkSpecifier
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.PatternMatcher
import androidx.annotation.RequiresApi

/**
 * Connects the phone to a tank hotspot (Wi-Fi names starting with "WTMS").
 *
 * Android 10+: Android shows its own list of matching hotspots; the connection is for this app only,
 * so other apps keep using mobile data, and it ends when [release] is called.
 * Android 8-9: the app lists the hotspots itself (needs location permission to scan) and joins the chosen one.
 */
object HotspotLink {
    const val PREFIX = "WTMS"
    const val ADDRESS = "192.168.4.1"

    /** The tank hotspot network while connected, used for every request to [ADDRESS]. */
    @Volatile var network: Network? = null
        private set

    private var callback: ConnectivityManager.NetworkCallback? = null
    private var legacyNetId = -1
    private val main = Handler(Looper.getMainLooper())

    /** Android 10+: show the system picker and connect. */
    @RequiresApi(Build.VERSION_CODES.Q)
    fun connect(ctx: Context, onResult: (ok: Boolean, message: String) -> Unit) {
        release(ctx)
        val cm = ctx.getSystemService(ConnectivityManager::class.java) ?: return onResult(false, "Wi-Fi is not available")
        val spec = WifiNetworkSpecifier.Builder()
            .setSsidPattern(PatternMatcher(PREFIX, PatternMatcher.PATTERN_PREFIX))
            .build()
        val request = NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .setNetworkSpecifier(spec)
            .build()
        var answered = false
        val cb = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(n: Network) {
                network = n
                main.post { if (!answered) { answered = true; onResult(true, "") } }
            }
            override fun onUnavailable() {
                main.post { if (!answered) { answered = true; onResult(false, "No tank hotspot was chosen or found nearby") } }
            }
            override fun onLost(n: Network) { if (network == n) network = null }
        }
        callback = cb
        try { cm.requestNetwork(request, cb) }
        catch (e: Exception) { callback = null; onResult(false, e.message ?: "Could not ask Android to connect") }
    }

    /** Android 8-9: tank hotspots in range (needs location permission and location turned on). */
    @Suppress("DEPRECATION")
    fun scanLegacy(ctx: Context, onResult: (List<String>) -> Unit) {
        val wm = ctx.applicationContext.getSystemService(WifiManager::class.java) ?: return onResult(emptyList())
        wm.startScan()
        main.postDelayed({
            val names = try { wm.scanResults.map { it.SSID }.filter { it.startsWith(PREFIX) }.distinct() } catch (e: SecurityException) { emptyList() }
            onResult(names)
        }, 4000)
    }

    /** Android 8-9: join an open tank hotspot and wait until the phone is on it. */
    @Suppress("DEPRECATION")
    fun connectLegacy(ctx: Context, ssid: String, onResult: (ok: Boolean, message: String) -> Unit) {
        val wm = ctx.applicationContext.getSystemService(WifiManager::class.java) ?: return onResult(false, "Wi-Fi is not available")
        val conf = WifiConfiguration().apply {
            SSID = "\"$ssid\""
            allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE)        // tank hotspots are open networks
        }
        val id = wm.addNetwork(conf)
        if (id == -1) return onResult(false, "Android refused to add $ssid")
        legacyNetId = id
        wm.disconnect(); wm.enableNetwork(id, true); wm.reconnect()
        var tries = 0
        val check = object : Runnable {
            override fun run() {
                val current = wm.connectionInfo?.ssid?.trim('"')
                val net = NetUtil.wifiNetwork(ctx)
                if (current == ssid && net != null) { network = net; onResult(true, "") }
                else if (++tries > 20) onResult(false, "Could not join $ssid. Try again closer to the tank.")
                else main.postDelayed(this, 1000)
            }
        }
        main.postDelayed(check, 1500)
    }

    /** Leave the tank hotspot; the phone goes back to its normal Wi-Fi or mobile data. */
    @Suppress("DEPRECATION")
    fun release(ctx: Context) {
        callback?.let { cb ->
            try { ctx.getSystemService(ConnectivityManager::class.java)?.unregisterNetworkCallback(cb) } catch (_: Exception) {}
        }
        callback = null
        if (legacyNetId != -1) {
            ctx.applicationContext.getSystemService(WifiManager::class.java)?.let { wm ->
                wm.removeNetwork(legacyNetId); wm.reconnect()
            }
            legacyNetId = -1
        }
        network = null
    }
}
