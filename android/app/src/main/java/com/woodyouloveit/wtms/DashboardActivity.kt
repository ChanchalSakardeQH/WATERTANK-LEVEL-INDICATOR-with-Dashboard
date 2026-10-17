/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.Manifest
import android.annotation.SuppressLint
import android.content.ActivityNotFoundException
import android.content.ClipData
import android.content.ContentValues
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.provider.Settings
import android.view.View
import android.webkit.ValueCallback
import android.webkit.WebChromeClient
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.content.FileProvider
import androidx.lifecycle.lifecycleScope
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout
import kotlinx.coroutines.launch
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.progressindicator.LinearProgressIndicator
import java.io.File

class DashboardActivity : AppCompatActivity() {

    companion object {
        const val EXTRA_ADDRESS = "address"
        const val EXTRA_TITLE = "title"
        const val EXTRA_RELEASE_HOTSPOT = "release_hotspot"   // leave the tank hotspot when this screen closes
        const val EXTRA_DEVICE_ID = "device_id"               // saved tank, for its admin session
    }

    private lateinit var web: WebView
    private lateinit var progress: LinearProgressIndicator
    private lateinit var errorPanel: LinearLayout
    private lateinit var errorText: TextView
    private lateinit var baseUrl: String
    private var baseHost = ""
    private var boundToWifi = false
    private var deviceId: String? = null
    private var adminToken: String? = null      // admin session from the app's own login
    private var syncToken = false               // hand the session to the dashboard page once it has loaded
    private lateinit var toolbarRef: MaterialToolbar

    private var fileCallback: ValueCallback<Array<Uri>>? = null
    private val pickFile = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        fileCallback?.onReceiveValue(WebChromeClient.FileChooserParams.parseResult(result.resultCode, result.data))
        fileCallback = null
    }

    // Android 8-9 need the storage permission to write to Downloads
    private var pendingSave: Triple<String, String, ByteArray>? = null
    private val askStorage = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        val p = pendingSave; pendingSave = null
        if (granted && p != null) saveToDownloads(p.first, p.second, p.third)
        else if (p != null) toast("Storage permission is needed to save files")
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        val address = intent.getStringExtra(EXTRA_ADDRESS) ?: run { finish(); return }
        baseUrl = StatusClient.baseUrl(address)
        baseHost = Uri.parse(baseUrl).host ?: ""

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        toolbarRef = toolbar
        deviceId = intent.getStringExtra(EXTRA_DEVICE_ID)
        toolbar.title = intent.getStringExtra(EXTRA_TITLE) ?: getString(R.string.app_name)
        toolbar.subtitle = address
        toolbar.setNavigationIcon(R.drawable.ic_back)
        toolbar.setNavigationOnClickListener { finish() }
        toolbar.inflateMenu(R.menu.dashboard_menu)
        toolbar.menu.findItem(R.id.action_reload)?.icon?.setTint(ContextCompat.getColor(this, R.color.white))
        toolbar.setOnMenuItemClickListener {
            when (it.itemId) {
                R.id.action_reload -> { load(); true }
                R.id.action_admin -> { if (adminToken != null) adminLogout() else adminLogin(); true }
                R.id.action_firmware -> {
                    startActivity(android.content.Intent(this, FirmwareActivity::class.java)
                        .putExtra(FirmwareActivity.EXTRA_ADDRESS, intent.getStringExtra(EXTRA_ADDRESS))
                        .putExtra(FirmwareActivity.EXTRA_TITLE, intent.getStringExtra(EXTRA_TITLE)))
                    true
                }
                R.id.action_browser -> { openExternal(Uri.parse(baseUrl)); true }
                else -> false
            }
        }

        progress = findViewById(R.id.progress)
        errorPanel = findViewById(R.id.errorPanel)
        errorText = findViewById(R.id.errorText)
        findViewById<MaterialButton>(R.id.retry).setOnClickListener { load() }
        findViewById<MaterialButton>(R.id.wifiSettings).setOnClickListener {
            try { startActivity(Intent(Settings.ACTION_WIFI_SETTINGS)) } catch (_: ActivityNotFoundException) {}
        }

        // Tank hotspot or home Wi-Fi: keep this screen on that network even if it has no internet
        boundToWifi = NetUtil.bindProcessFor(this, baseHost)

        web = findViewById(R.id.web)
        web.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true           // the dashboard keeps the admin session and unit choice here
            allowFileAccess = false
            allowContentAccess = false
            setSupportMultipleWindows(false)
            userAgentString = "$userAgentString WTMSApp/${BuildConfig.VERSION_NAME}"
        }
        web.addJavascriptInterface(AppBridge(this), "WTMSApp")
        web.webViewClient = Client()
        web.webChromeClient = Chrome()

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() { if (web.canGoBack()) web.goBack() else finish() }
        })

        // A saved admin session is only used if the tank still accepts it
        val saved = deviceId?.let { AuthStore.get(this, it) }
        if (saved == null) {
            if (savedInstanceState != null) web.restoreState(savedInstanceState) else load()
        } else lifecycleScope.launch {
            val api = TankApi(this@DashboardActivity, address).also { it.useToken(saved) }
            if (api.isAdmin()) { adminToken = saved; syncToken = true }
            else deviceId?.let { AuthStore.clear(this@DashboardActivity, it) }
            updateAdminMenu()
            load()
        }
        updateAdminMenu()
    }

    private fun updateAdminMenu() {
        toolbarRef.menu.findItem(R.id.action_admin)?.title = getString(if (adminToken != null) R.string.admin_logout else R.string.admin_login)
        toolbarRef.subtitle = (intent.getStringExtra(EXTRA_ADDRESS) ?: "") + if (adminToken != null) " · admin" else ""
    }

    /** Puts the app's admin session into the dashboard page (its own login uses the same storage). */
    private fun pushTokenToPage(token: String?) {
        val js = if (token != null)
            "(function(){try{if(localStorage.getItem('wtmsAuth')!=='$token'){localStorage.setItem('wtmsAuth','$token');location.reload();}}catch(e){}})()"
        else "(function(){try{localStorage.removeItem('wtmsAuth');location.reload();}catch(e){}})()"
        web.evaluateJavascript(js, null)
    }

    private fun passwordField(hint: String): Pair<TextInputLayout, TextInputEditText> {
        val til = TextInputLayout(this).apply { this.hint = hint; endIconMode = TextInputLayout.END_ICON_PASSWORD_TOGGLE }
        val edit = TextInputEditText(til.context).apply {
            inputType = android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        }
        til.addView(edit)
        return til to edit
    }

    private fun dialogBox(vararg views: View): LinearLayout {
        val pad = (20 * resources.displayMetrics.density).toInt()
        return LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL; setPadding(pad, pad / 2, pad, 0)
            views.forEach { addView(it) }
        }
    }

    private fun adminLogin() {
        val (til, edit) = passwordField("Tank admin password")
        MaterialAlertDialogBuilder(this)
            .setTitle("Admin login")
            .setMessage("Unlocks settings, calibration, sensors, Wi-Fi, logs and firmware updates for this tank.")
            .setView(dialogBox(til))
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Log in") { _, _ ->
                val pw = edit.text?.toString().orEmpty()
                val address = intent.getStringExtra(EXTRA_ADDRESS) ?: return@setPositiveButton
                lifecycleScope.launch {
                    val api = TankApi(this@DashboardActivity, address)
                    try {
                        api.login(pw)
                        if (api.mustChangePassword) newPassword(api) else loggedIn(api.token)
                    } catch (e: Exception) { toast(e.message ?: "Login failed") }
                }
            }
            .show()
    }

    /** A tank with the default password: choose a new one before continuing. */
    private fun newPassword(api: TankApi) {
        val (t1, e1) = passwordField("New admin password (at least 6 characters)")
        val (t2, e2) = passwordField("Repeat the new password")
        MaterialAlertDialogBuilder(this)
            .setTitle("Choose an admin password")
            .setMessage("This tank still uses the default password.")
            .setView(dialogBox(t1, t2))
            .setCancelable(false)
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Save") { _, _ ->
                val p1 = e1.text?.toString().orEmpty(); val p2 = e2.text?.toString().orEmpty()
                when {
                    p1.length < 6 -> { toast("Use at least 6 characters"); newPassword(api) }
                    p1 != p2 -> { toast("The two passwords don't match"); newPassword(api) }
                    p1 == "admin" -> { toast("Choose a password other than \"admin\""); newPassword(api) }
                    else -> lifecycleScope.launch {
                        try { api.changePassword(p1); loggedIn(api.token) } catch (e: Exception) { toast(e.message ?: "Could not save the password") }
                    }
                }
            }
            .show()
    }

    private fun loggedIn(token: String) {
        adminToken = token
        deviceId?.let { AuthStore.set(this, it, token) }
        updateAdminMenu()
        pushTokenToPage(token)
        toast("Logged in as admin")
    }

    private fun adminLogout() {
        val t = adminToken ?: return
        val address = intent.getStringExtra(EXTRA_ADDRESS) ?: return
        lifecycleScope.launch {
            try { TankApi(this@DashboardActivity, address).also { it.useToken(t) }.logout() } catch (_: Exception) {}
            adminToken = null
            deviceId?.let { AuthStore.clear(this@DashboardActivity, it) }
            updateAdminMenu()
            pushTokenToPage(null)
            toast("Logged out")
        }
    }

    private fun load() {
        errorPanel.visibility = View.GONE
        web.loadUrl("$baseUrl/")
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        web.saveState(outState)
    }

    override fun onDestroy() {
        if (boundToWifi) NetUtil.unbindProcess(this)
        if (intent.getBooleanExtra(EXTRA_RELEASE_HOTSPOT, false)) HotspotLink.release(this)
        if (::web.isInitialized) { web.stopLoading(); web.destroy() }
        super.onDestroy()
    }

    private inner class Client : WebViewClient() {
        override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
            val uri = request.url
            val scheme = uri.scheme?.lowercase() ?: return true
            // Pages of this device stay in the app; everything else goes to the right app
            if ((scheme == "http" || scheme == "https") && uri.host == baseHost) return false
            openExternal(uri)
            return true
        }

        override fun onPageStarted(view: WebView, url: String?, favicon: Bitmap?) { progress.visibility = View.VISIBLE }
        override fun onPageFinished(view: WebView, url: String?) {
            progress.visibility = View.GONE
            if (syncToken) { syncToken = false; adminToken?.let { pushTokenToPage(it) } }
        }

        override fun onReceivedError(view: WebView, request: WebResourceRequest, error: WebResourceError) {
            if (!request.isForMainFrame) return
            progress.visibility = View.GONE
            errorText.text = buildString {
                append("Can't reach the tank at ").append(baseHost).append(".\n\n")
                append("• On the device hotspot: connect this phone to the WTMS Wi-Fi first.\n")
                append("• On home Wi-Fi: check the device is powered and on the same network.\n\n")
                append("(").append(error.description).append(")")
            }
            errorPanel.visibility = View.VISIBLE
        }
    }

    private inner class Chrome : WebChromeClient() {
        override fun onShowFileChooser(view: WebView, callback: ValueCallback<Array<Uri>>, params: FileChooserParams): Boolean {
            fileCallback?.onReceiveValue(null)
            fileCallback = callback
            return try { pickFile.launch(params.createIntent()); true }
            catch (e: ActivityNotFoundException) { fileCallback = null; toast("No file picker available"); false }
        }
    }

    // ---------------------------------------------------------------- links to other apps

    private fun openExternal(uri: Uri) {
        val i = if (uri.scheme.equals("intent", true)) {
            try { Intent.parseUri(uri.toString(), Intent.URI_INTENT_SCHEME) } catch (e: Exception) { return }
        } else Intent(Intent.ACTION_VIEW, uri)
        try { startActivity(i) }
        catch (e: ActivityNotFoundException) {
            toast(if (uri.scheme == "whatsapp") "WhatsApp is not installed" else "No app can open this link")
        }
    }

    // ---------------------------------------------------------------- files (called from AppBridge)

    fun saveToDownloads(name: String, mime: String, bytes: ByteArray) {
        val safe = safeName(name)
        val type = mime.ifBlank { "application/octet-stream" }
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                val values = ContentValues().apply {
                    put(MediaStore.MediaColumns.DISPLAY_NAME, safe)
                    put(MediaStore.MediaColumns.MIME_TYPE, type)
                    put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/WTMS")
                }
                val uri = contentResolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values)
                    ?: throw IllegalStateException("Downloads is not available")
                contentResolver.openOutputStream(uri)?.use { it.write(bytes) }
            } else {
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                    pendingSave = Triple(name, mime, bytes)
                    askStorage.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    return
                }
                @Suppress("DEPRECATION")
                val dir = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "WTMS")
                dir.mkdirs()
                File(dir, safe).writeBytes(bytes)
            }
            toast("Saved to Downloads/WTMS/$safe")
        } catch (e: Exception) {
            toast("Could not save the file: ${e.message}")
        }
    }

    fun shareContent(target: String, name: String, mime: String, bytes: ByteArray?, text: String, subject: String) {
        val send = Intent(Intent.ACTION_SEND)
        if (bytes != null && name.isNotBlank()) {
            val dir = File(cacheDir, "shared").apply { mkdirs(); listFiles()?.forEach { it.delete() } }
            val file = File(dir, safeName(name)).apply { writeBytes(bytes) }
            val uri = FileProvider.getUriForFile(this, "$packageName.files", file)
            send.type = mime.ifBlank { "application/octet-stream" }
            send.putExtra(Intent.EXTRA_STREAM, uri)
            send.clipData = ClipData.newRawUri(file.name, uri)
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        } else {
            send.type = "text/plain"
        }
        send.putExtra(Intent.EXTRA_TEXT, text)
        send.putExtra(Intent.EXTRA_SUBJECT, subject)

        when (target) {
            "whatsapp" -> {
                for (pkg in listOf("com.whatsapp", "com.whatsapp.w4b")) {
                    try { startActivity(Intent(send).setPackage(pkg)); return } catch (_: ActivityNotFoundException) {}
                }
                toast("WhatsApp is not installed")
                startActivity(Intent.createChooser(send, "Share"))
            }
            "email" -> {
                // Only email apps: ACTION_SEND restricted by a mailto: selector
                val email = Intent(send).apply { selector = Intent(Intent.ACTION_SENDTO, Uri.parse("mailto:")) }
                try { startActivity(email) } catch (_: ActivityNotFoundException) { startActivity(Intent.createChooser(send, "Send email")) }
            }
            else -> startActivity(Intent.createChooser(send, "Share"))
        }
    }

    private fun safeName(name: String): String =
        name.replace(Regex("[^A-Za-z0-9._-]"), "_").take(120).ifBlank { "wtms-file" }

    private fun toast(msg: String) = Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
}
