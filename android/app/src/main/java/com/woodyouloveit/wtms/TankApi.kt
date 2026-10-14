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
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.security.MessageDigest

/** A Wi-Fi network as seen by the tank device. */
data class TankNet(val ssid: String, val rssi: Int, val open: Boolean)

/**
 * Talks to a tank's admin API (firmware 2.9.0 and newer), the same way the dashboard does.
 * Login uses the device's challenge: the password itself is never sent.
 */
class TankApi(private val ctx: Context, address: String) {
    val base = StatusClient.baseUrl(address)
    var token = ""
        private set
    var mustChangePassword = false
        private set

    /** Reuse a session from the same tank under another address (hotspot -> home network). */
    fun useToken(t: String) { token = t }

    class ApiError(message: String) : Exception(message)

    private fun sha256(text: String): String =
        MessageDigest.getInstance("SHA-256").digest(text.toByteArray(Charsets.UTF_8)).joinToString("") { "%02x".format(it) }

    private fun conn(path: String, timeoutMs: Int): HttpURLConnection =
        (NetUtil.open(ctx, URL(base + path)) as HttpURLConnection).apply {
            connectTimeout = timeoutMs; readTimeout = timeoutMs; useCaches = false
        }

    private suspend fun get(path: String, timeoutMs: Int = 6000): JSONObject = withContext(Dispatchers.IO) {
        val c = conn(path, timeoutMs)
        try {
            val code = c.responseCode
            val body = (if (code < 400) c.inputStream else c.errorStream)?.bufferedReader()?.use { it.readText() } ?: "{}"
            if (code == 403) throw ApiError("Admin login required")
            if (code >= 400) throw ApiError("HTTP $code")
            JSONObject(body)
        } finally { c.disconnect() }
    }

    private suspend fun post(path: String, params: Map<String, String>, timeoutMs: Int = 8000): JSONObject = withContext(Dispatchers.IO) {
        val all = if (token.isNotEmpty()) params + ("auth" to token) else params
        val form = all.entries.joinToString("&") { URLEncoder.encode(it.key, "UTF-8") + "=" + URLEncoder.encode(it.value, "UTF-8") }
        val c = conn(path, timeoutMs)
        try {
            c.requestMethod = "POST"
            c.doOutput = true
            c.setRequestProperty("Content-Type", "application/x-www-form-urlencoded")
            c.outputStream.use { it.write(form.toByteArray(Charsets.UTF_8)) }
            val code = c.responseCode
            val body = (if (code < 400) c.inputStream else c.errorStream)?.bufferedReader()?.use { it.readText() } ?: "{}"
            val json = try { JSONObject(body) } catch (e: Exception) { JSONObject() }
            if (code == 403) throw ApiError("Admin login required")
            if (!json.optBoolean("ok", code < 400)) throw ApiError(json.optString("message", "HTTP $code"))
            json
        } finally { c.disconnect() }
    }

    /** Full /api/status as JSON (for the Wi-Fi state during setup). */
    suspend fun statusJson(timeoutMs: Int = 5000): JSONObject = get("/api/status" + if (token.isNotEmpty()) "?auth=$token" else "", timeoutMs)

    suspend fun login(password: String) {
        val challenge = get("/api/auth")
        val proof = sha256(challenge.getString("nonce") + sha256(challenge.getString("salt") + password))
        val r = post("/api/login", mapOf("proof" to proof))
        token = r.optString("token")
        mustChangePassword = r.optBoolean("mustChange", false)
        if (token.isEmpty()) throw ApiError(r.optString("message", "Login failed"))
    }

    suspend fun changePassword(newPassword: String) {
        val salt = get("/api/auth").getString("salt")
        post("/api/password", mapOf("hash" to sha256(salt + newPassword)))
        mustChangePassword = false
    }

    suspend fun setSite(org: String, building: String, tank: String) {
        post("/api/site", mapOf("org" to org, "building" to building, "tank" to tank))
    }

    /** Starts a scan on the tank, then waits for its result (up to about 15 s). */
    suspend fun scanNetworks(): List<TankNet> {
        get("/api/scan?start=1&auth=$token")
        repeat(15) {
            kotlinx.coroutines.delay(1000)
            val r = get("/api/scan?auth=$token")
            if (!r.optBoolean("running", false)) {
                val arr = r.optJSONArray("networks") ?: return emptyList()
                return (0 until arr.length()).mapNotNull { i ->
                    arr.optJSONObject(i)?.let { TankNet(it.optString("ssid"), it.optInt("rssi"), it.optBoolean("open")) }
                }.filter { it.ssid.isNotBlank() }
            }
        }
        throw ApiError("The tank did not finish scanning")
    }

    suspend fun connectWifi(ssid: String, password: String) {
        post("/api/wifi/connect", mapOf("ssid" to ssid, "pass" to password))
    }

    suspend fun restart() { post("/api/restart", emptyMap()) }

    suspend fun logout() { post("/api/logout", emptyMap()) }

    /** True when the current token is still an admin session on the device (sessions end on restart or after 12 h idle). */
    suspend fun isAdmin(): Boolean = try {
        statusJson(4000).optJSONObject("auth")?.optBoolean("admin", false) == true
    } catch (e: Exception) { false }
}
