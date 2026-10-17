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
import java.net.URLEncoder
import java.security.MessageDigest

/** A firmware image: from the app itself (bundled) or from a file on the phone. */
data class Firmware(val board: String, val version: String, val name: String, val bytes: ByteArray, val bundled: Boolean)

object FirmwareFiles {
    private val MARKER = Regex("Water Tanks Monitor System (\\d+\\.\\d+\\.\\d+) \\| woodyouloveit\\.com \\| board (esp32|esp8266)")

    /** Board and version from the marker every Water Tanks Monitor firmware carries; null if it is not one. */
    fun identify(bytes: ByteArray): Pair<String, String>? {
        if (bytes.size < 1024 || bytes[0] != 0xE9.toByte()) return null          // ESP application image
        val m = MARKER.find(String(bytes, Charsets.ISO_8859_1)) ?: return null
        return m.groupValues[2] to m.groupValues[1]
    }

    fun sha256(bytes: ByteArray): String =
        MessageDigest.getInstance("SHA-256").digest(bytes).joinToString("") { "%02x".format(it) }

    /** Firmware packed into the app at build time (assets/firmware/manifest.json), checked against its checksum. */
    fun bundled(ctx: Context): List<Firmware> = try {
        val arr = JSONArray(ctx.assets.open("firmware/manifest.json").bufferedReader().use { it.readText() })
        (0 until arr.length()).mapNotNull { i ->
            val o = arr.getJSONObject(i)
            val bytes = ctx.assets.open("firmware/" + o.getString("file")).use { it.readBytes() }
            if (sha256(bytes) != o.optString("sha256")) null
            else Firmware(o.getString("board"), o.getString("version"), o.getString("file"), bytes, true)
        }
    } catch (e: Exception) { emptyList() }

    fun compareVersions(a: String, b: String): Int {
        val x = a.split('.').map { it.toIntOrNull() ?: 0 }
        val y = b.split('.').map { it.toIntOrNull() ?: 0 }
        for (i in 0 until maxOf(x.size, y.size)) {
            val d = x.getOrElse(i) { 0 } - y.getOrElse(i) { 0 }
            if (d != 0) return d
        }
        return 0
    }
}

/** Sends a firmware image to the tank's /api/ota (multipart upload, as the dashboard does). */
object OtaClient {
    suspend fun upload(ctx: Context, base: String, token: String, fw: Firmware, progress: (Int) -> Unit): JSONObject =
        withContext(Dispatchers.IO) {
            val boundary = "----wtms" + System.currentTimeMillis()
            val head = ("--$boundary\r\nContent-Disposition: form-data; name=\"firmware\"; filename=\"${fw.name}\"\r\n" +
                "Content-Type: application/octet-stream\r\n\r\n").toByteArray(Charsets.UTF_8)
            val tail = "\r\n--$boundary--\r\n".toByteArray(Charsets.UTF_8)
            val url = URL("$base/api/ota?auth=" + URLEncoder.encode(token, "UTF-8"))
            val c = NetUtil.open(ctx, url) as HttpURLConnection
            try {
                c.requestMethod = "POST"
                c.doOutput = true
                c.connectTimeout = 10_000
                c.readTimeout = 120_000                                    // the tank verifies the image before answering
                c.setRequestProperty("Content-Type", "multipart/form-data; boundary=$boundary")
                c.setFixedLengthStreamingMode(head.size + fw.bytes.size + tail.size)
                c.outputStream.use { out ->
                    out.write(head)
                    var off = 0
                    while (off < fw.bytes.size) {
                        val n = minOf(8192, fw.bytes.size - off)
                        out.write(fw.bytes, off, n)
                        off += n
                        progress(off * 100 / fw.bytes.size)
                    }
                    out.write(tail)
                }
                val code = c.responseCode
                val body = (if (code < 400) c.inputStream else c.errorStream)?.bufferedReader()?.use { it.readText() } ?: "{}"
                try { JSONObject(body) } catch (e: Exception) { JSONObject().put("ok", false).put("message", "HTTP $code") }
            } finally {
                c.disconnect()
            }
        }
}
