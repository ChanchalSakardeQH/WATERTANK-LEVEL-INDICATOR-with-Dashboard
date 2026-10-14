/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.util.UUID

/** A saved tank device. [address] is an IP, host name or full http(s):// URL. */
data class Device(
    val id: String = UUID.randomUUID().toString(),
    var name: String,
    var address: String,
    var host: String = "",       // mDNS host name reported by the device, used to find it again after an IP change
    var remote: String = ""      // optional address for use away from home: https:// tunnel link or VPN address
)

/** Saved devices, kept in the app's private preferences. */
object DeviceStore {
    private const val PREFS = "devices"
    private const val KEY = "list"

    fun load(ctx: Context): MutableList<Device> {
        val raw = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(KEY, "[]") ?: "[]"
        val out = mutableListOf<Device>()
        val arr = try { JSONArray(raw) } catch (e: Exception) { JSONArray() }
        for (i in 0 until arr.length()) {
            val o = arr.optJSONObject(i) ?: continue
            out.add(Device(o.optString("id", UUID.randomUUID().toString()), o.optString("name"), o.optString("address"),
                o.optString("host"), o.optString("remote")))
        }
        return out
    }

    fun save(ctx: Context, list: List<Device>) {
        val arr = JSONArray()
        list.forEach {
            arr.put(JSONObject().put("id", it.id).put("name", it.name).put("address", it.address).put("host", it.host).put("remote", it.remote))
        }
        ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putString(KEY, arr.toString()).apply()
    }
}

/** One tank compartment (one ultrasonic sensor), firmware 3.0.0 and newer. */
data class Compartment(val name: String, val valid: Boolean, val level: Double, val capacity: Int, val filling: Boolean) {
    val litres: Int get() = if (capacity > 0) Math.round(capacity * level / 100.0).toInt() else 0
}

/** Admin sessions per saved tank (the device's own session token, never the password). */
object AuthStore {
    private const val PREFS = "admin_sessions"
    private fun prefs(ctx: Context) = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
    fun get(ctx: Context, deviceId: String): String? = prefs(ctx).getString(deviceId, null)
    fun set(ctx: Context, deviceId: String, token: String) { prefs(ctx).edit().putString(deviceId, token).apply() }
    fun clear(ctx: Context, deviceId: String) { prefs(ctx).edit().remove(deviceId).apply() }
    fun ids(ctx: Context): Set<String> = prefs(ctx).all.keys
}

/** The part of the device's /api/status the app uses. */
data class TankStatus(
    val reachable: Boolean,
    val error: String = "",
    val valid: Boolean = false,          // sensor answering
    val level: Double = 0.0,             // %
    val capacity: Int = 0,               // litres, 0 = not set
    val lowAlarm: Int = 10,              // %
    val filling: Boolean = false,
    val fw: String = "",
    val board: String = "",
    val org: String = "",
    val building: String = "",
    val tank: String = "",
    val host: String = "",
    val compartments: List<Compartment> = emptyList(),   // 2+ when the tank has several compartments
    val totalValid: Boolean = false                       // whole-tank level available
) {
    val title: String get() = listOf(building, tank).filter { it.isNotBlank() }.joinToString(" · ")
    val litres: Int get() = if (capacity > 0) Math.round(capacity * level / 100.0).toInt() else 0
    val multi: Boolean get() = compartments.size > 1
    val lowNames: List<String> get() =
        if (multi) compartments.filter { it.valid && it.level < lowAlarm }.map { it.name }
        else if (valid && level < lowAlarm) listOf("") else emptyList()
    val faultNames: List<String> get() =
        if (multi) compartments.filter { !it.valid }.map { it.name } else if (!valid) listOf("") else emptyList()
    val low: Boolean get() = lowNames.isNotEmpty()
    val anyFilling: Boolean get() = filling || compartments.any { it.filling }
}

/** Status plus the address that answered (local or remote). */
data class Reach(val status: TankStatus, val address: String, val viaRemote: Boolean)

object StatusClient {
    /** Try the local address first (quick), then the remote address if one is set. */
    suspend fun fetchAny(ctx: Context, d: Device, timeoutMs: Int = 6000): Reach {
        val local = fetch(ctx, d.address, if (d.remote.isNotBlank()) minOf(timeoutMs, 3000) else timeoutMs)
        if (local.reachable || d.remote.isBlank()) return Reach(local, d.address, false)
        val remote = fetch(ctx, d.remote, timeoutMs)
        return if (remote.reachable) Reach(remote, d.remote, true) else Reach(local, d.address, false)
    }

    /** "192.168.1.42" -> "http://192.168.1.42"; full URLs are kept. */
    fun baseUrl(address: String): String {
        val a = address.trim().trimEnd('/')
        return if (a.startsWith("http://", true) || a.startsWith("https://", true)) a else "http://$a"
    }

    suspend fun fetch(ctx: Context, address: String, timeoutMs: Int = 6000): TankStatus = withContext(Dispatchers.IO) {
        try {
            val conn = NetUtil.open(ctx, URL(baseUrl(address) + "/api/status")) as HttpURLConnection
            conn.connectTimeout = timeoutMs
            conn.readTimeout = timeoutMs
            conn.useCaches = false
            try {
                if (conn.responseCode != 200) return@withContext TankStatus(false, "HTTP ${conn.responseCode}")
                val body = conn.inputStream.bufferedReader().use { it.readText() }
                parse(JSONObject(body))
            } finally {
                conn.disconnect()
            }
        } catch (e: Exception) {
            TankStatus(false, e.message ?: e.javaClass.simpleName)
        }
    }

    fun parse(o: JSONObject): TankStatus {
        if (!o.has("fw") || !o.has("board")) return TankStatus(false, "Not a Water Tanks Monitor device")
        val site = o.optJSONObject("site") ?: JSONObject()
        val wifi = o.optJSONObject("wifi") ?: JSONObject()
        val comps = mutableListOf<Compartment>()
        o.optJSONArray("sensors")?.let { arr ->
            for (i in 0 until arr.length()) {
                val c = arr.optJSONObject(i) ?: continue
                comps.add(Compartment(c.optString("name"), c.optBoolean("valid"), c.optDouble("level", 0.0),
                    c.optInt("capacity", 0), c.optBoolean("filling")))
            }
        }
        val valid = o.optBoolean("valid", false)
        return TankStatus(
            reachable = true,
            valid = valid,
            level = o.optDouble("level", 0.0),
            capacity = o.optInt("capacity", 0),
            lowAlarm = o.optInt("lowAlarm", 10),
            filling = o.optBoolean("filling", false),
            fw = o.optString("fw"),
            board = o.optString("board"),
            org = site.optString("org"),
            building = site.optString("building"),
            tank = site.optString("tank"),
            host = wifi.optString("host"),
            compartments = comps,
            totalValid = o.optJSONObject("total")?.optBoolean("valid", valid) ?: valid
        )
    }
}
