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
import java.net.URL
import java.net.URLConnection

/**
 * Picks the network for each request to a tank:
 *  - 192.168.4.1 while the app holds a tank hotspot connection -> that hotspot ([HotspotLink])
 *  - other local addresses -> the Wi-Fi network, so a Wi-Fi without internet still works,
 *    except when a VPN (e.g. Tailscale) is on: then the VPN must carry them
 *  - everything else (https:// remote addresses) -> the phone's normal network
 */
object NetUtil {
    fun isLocalHost(host: String): Boolean {
        val h = host.lowercase()
        if (h.endsWith(".local")) return true
        val p = h.split('.').mapNotNull { it.toIntOrNull() }
        if (p.size != 4) return false
        return p[0] == 10 || (p[0] == 192 && p[1] == 168) || (p[0] == 172 && p[1] in 16..31) || (p[0] == 169 && p[1] == 254)
    }

    @Suppress("DEPRECATION")
    fun wifiNetwork(ctx: Context): Network? {
        val cm = ctx.getSystemService(ConnectivityManager::class.java) ?: return null
        return cm.allNetworks.firstOrNull { cm.getNetworkCapabilities(it)?.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) == true }
    }

    fun vpnActive(ctx: Context): Boolean {
        val cm = ctx.getSystemService(ConnectivityManager::class.java) ?: return false
        return cm.getNetworkCapabilities(cm.activeNetwork)?.hasTransport(NetworkCapabilities.TRANSPORT_VPN) == true
    }

    /** The network to use for [host], or null for the phone's default. */
    fun networkFor(ctx: Context, host: String): Network? {
        HotspotLink.network?.let { if (host == HotspotLink.ADDRESS) return it }
        if (isLocalHost(host) && !vpnActive(ctx)) return wifiNetwork(ctx)
        return null
    }

    fun open(ctx: Context, url: URL): URLConnection =
        networkFor(ctx, url.host)?.openConnection(url) ?: url.openConnection()

    /** Route the whole app (including WebView) to the right network while a dashboard is open. */
    fun bindProcessFor(ctx: Context, host: String): Boolean {
        val cm = ctx.getSystemService(ConnectivityManager::class.java) ?: return false
        val net = networkFor(ctx, host) ?: return false
        return cm.bindProcessToNetwork(net)
    }

    fun unbindProcess(ctx: Context) {
        ctx.getSystemService(ConnectivityManager::class.java)?.bindProcessToNetwork(null)
    }
}
