/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.util.Base64
import android.webkit.JavascriptInterface

/**
 * Exposed to the device dashboard as window.WTMSApp (firmware 2.12.0 and newer use it).
 * Files arrive as base64 because WebView cannot download blob: URLs by itself.
 * Only pages from the tank device are ever loaded in the WebView (see DashboardActivity).
 */
class AppBridge(private val activity: DashboardActivity) {

    @JavascriptInterface
    fun version(): String = BuildConfig.VERSION_NAME

    /** Save a file to Downloads/WTMS. */
    @JavascriptInterface
    fun saveFile(name: String, mime: String, base64: String) {
        val bytes = decode(base64) ?: return
        activity.runOnUiThread { activity.saveToDownloads(name, mime, bytes) }
    }

    /**
     * Share text with an optional file attached.
     * target: "any" (share sheet), "whatsapp" or "email". Empty base64 = text only.
     */
    @JavascriptInterface
    fun share(target: String, name: String, mime: String, base64: String, text: String, subject: String) {
        val bytes = if (base64.isEmpty()) null else decode(base64)
        activity.runOnUiThread { activity.shareContent(target, name, mime, bytes, text, subject) }
    }

    private fun decode(b64: String): ByteArray? = try { Base64.decode(b64, Base64.DEFAULT) } catch (e: IllegalArgumentException) { null }
}
