/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.net.wifi.WifiManager
import android.os.Handler
import android.os.Looper

/**
 * Finds devices that announce an HTTP service on the local network (mDNS / DNS-SD).
 * Tank devices announce "_http._tcp" under their host name (tank-<building>-<tank> or watertank-xxxx).
 * Every result is still checked with /api/status before it is offered.
 */
class Discovery(private val ctx: Context, private val onFound: (serviceName: String, ip: String) -> Unit) {
    private val nsd = ctx.getSystemService(NsdManager::class.java)
    private val main = Handler(Looper.getMainLooper())
    private var listener: NsdManager.DiscoveryListener? = null
    private val queue = ArrayDeque<NsdServiceInfo>()
    private var resolving = false
    private var lock: WifiManager.MulticastLock? = null

    fun start() {
        val wifi = ctx.applicationContext.getSystemService(WifiManager::class.java)
        lock = wifi?.createMulticastLock("wtms-discovery")?.apply { setReferenceCounted(false); acquire() }
        val l = object : NsdManager.DiscoveryListener {
            override fun onDiscoveryStarted(serviceType: String) {}
            override fun onServiceFound(info: NsdServiceInfo) { main.post { enqueue(info) } }
            override fun onServiceLost(info: NsdServiceInfo) {}
            override fun onDiscoveryStopped(serviceType: String) {}
            override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {}
            override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {}
        }
        listener = l
        try { nsd?.discoverServices("_http._tcp.", NsdManager.PROTOCOL_DNS_SD, l) } catch (e: Exception) { listener = null }
    }

    // Older Android versions resolve one service at a time
    private fun enqueue(info: NsdServiceInfo) { queue.addLast(info); next() }

    @Suppress("DEPRECATION")
    private fun next() {
        if (resolving || queue.isEmpty() || nsd == null) return
        resolving = true
        val info = queue.removeFirst()
        try {
            nsd.resolveService(info, object : NsdManager.ResolveListener {
                override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) { main.post { resolving = false; next() } }
                override fun onServiceResolved(serviceInfo: NsdServiceInfo) {
                    val ip = serviceInfo.host?.hostAddress
                    main.post {
                        if (ip != null && !ip.contains(':')) onFound(serviceInfo.serviceName, ip)
                        resolving = false; next()
                    }
                }
            })
        } catch (e: Exception) { resolving = false }
    }

    fun stop() {
        listener?.let { try { nsd?.stopServiceDiscovery(it) } catch (_: Exception) {} }
        listener = null
        queue.clear()
        try { lock?.release() } catch (_: Exception) {}
        lock = null
    }
}
