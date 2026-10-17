/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.progressindicator.LinearProgressIndicator
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.json.JSONObject

/**
 * Firmware update over Wi-Fi for the manufacturer (service login). Uses the firmware bundled in the app or a
 * .bin file, checks board, size and version, uploads it, waits for the restart and checks that the tank's
 * settings came back unchanged (the update replaces only the program).
 */
class FirmwareActivity : AppCompatActivity() {

    companion object {
        const val EXTRA_ADDRESS = "address"
        const val EXTRA_TITLE = "title"
    }

    private lateinit var address: String
    private lateinit var api: TankApi
    private lateinit var device: TextView
    private lateinit var bundledText: TextView
    private lateinit var btnBundled: MaterialButton
    private lateinit var btnFile: MaterialButton
    private lateinit var progress: LinearProgressIndicator
    private lateinit var status: TextView
    private var st: JSONObject? = null
    private var bundled: Firmware? = null
    private var busy = false

    private val pickFile = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null) return@registerForActivityResult
        val bytes = try { contentResolver.openInputStream(uri)?.use { it.readBytes() } } catch (e: Exception) { null }
        if (bytes == null) { toast("Could not read the file"); return@registerForActivityResult }
        val id = FirmwareFiles.identify(bytes)
        if (id == null) {
            MaterialAlertDialogBuilder(this).setTitle("Not Water Tanks Monitor firmware")
                .setMessage("Choose water_level_indicator.ino.bin from Arduino IDE's Export Compiled Binary (not the .bootloader, .partitions or .merged file).")
                .setPositiveButton("OK", null).show()
            return@registerForActivityResult
        }
        confirmAndInstall(Firmware(id.first, id.second, "wtms-${id.first}-${id.second}.bin", bytes, false))
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_firmware)
        address = intent.getStringExtra(EXTRA_ADDRESS) ?: run { finish(); return }
        api = TankApi(this, address)
        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        toolbar.subtitle = intent.getStringExtra(EXTRA_TITLE) ?: address
        toolbar.setNavigationOnClickListener { if (!busy) finish() }
        device = findViewById(R.id.fwDevice)
        bundledText = findViewById(R.id.fwBundled)
        btnBundled = findViewById(R.id.btnBundled)
        btnFile = findViewById(R.id.btnFile)
        progress = findViewById(R.id.fwProgress)
        status = findViewById(R.id.fwStatus)
        btnBundled.setOnClickListener { bundled?.let { confirmAndInstall(it) } }
        btnFile.setOnClickListener { pickFile.launch(arrayOf("application/octet-stream", "*/*")) }
        setButtons(false)
        onBackPressedDispatcher.addCallback(this, object : androidx.activity.OnBackPressedCallback(true) {
            override fun handleOnBackPressed() { if (busy) toast("Please wait until the update is finished") else finish() }
        })
        loadDevice()
    }

    private fun setButtons(on: Boolean) {
        btnBundled.isEnabled = on && bundled != null
        btnFile.isEnabled = on
    }

    private fun loadDevice() {
        lifecycleScope.launch {
            val s = try { api.statusJson(6000) } catch (e: Exception) { null }
            if (s == null) { device.text = "The tank at $address does not answer. Connect to its Wi-Fi or the same network, then open this again."; return@launch }
            st = s
            val board = s.optString("boardTag")
            device.text = describe(s)
            bundled = FirmwareFiles.bundled(this@FirmwareActivity).firstOrNull { it.board == board }
            val b = bundled
            bundledText.text = when {
                b == null -> "This app has no firmware for ${s.optString("board")} bundled. Choose a .bin file instead."
                else -> "Bundled in this app: firmware ${b.version} for ${s.optString("board")} (${b.bytes.size / 1024} KB)." +
                    when { FirmwareFiles.compareVersions(b.version, s.optString("fw")) > 0 -> " Newer than the tank's."
                           FirmwareFiles.compareVersions(b.version, s.optString("fw")) == 0 -> " Same as the tank's."
                           else -> " Older than the tank's." }
            }
            btnBundled.text = if (b != null) "Install bundled firmware ${b.version}" else "No bundled firmware for this board"
            if (s.optJSONObject("auth")?.optBoolean("serviceAvailable", false) != true) {
                status.text = "This tank's firmware has no service login, so it cannot be updated over Wi-Fi. Update it once by USB with firmware built with the service key."
                return@launch
            }
            serviceLogin()
        }
    }

    private fun describe(s: JSONObject): String {
        val site = s.optJSONObject("site") ?: JSONObject()
        val name = listOf(site.optString("building"), site.optString("tank")).filter { it.isNotBlank() }.joinToString(" · ")
        val sensors = s.optJSONArray("sensors")?.length() ?: 1
        return "${name.ifBlank { "Tank" }}\nFirmware ${s.optString("fw")} on ${s.optString("board")} · $sensors sensor(s)\n" +
            "Space for an update: ${s.optLong("otaMax") / 1024} KB"
    }

    private fun serviceLogin() {
        val til = TextInputLayout(this).apply { hint = "Service password"; endIconMode = TextInputLayout.END_ICON_PASSWORD_TOGGLE }
        val edit = TextInputEditText(til.context).apply {
            inputType = android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        }
        til.addView(edit)
        val pad = (20 * resources.displayMetrics.density).toInt()
        val box = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; setPadding(pad, pad / 2, pad, 0); addView(til) }
        MaterialAlertDialogBuilder(this)
            .setTitle("Service login")
            .setMessage("Firmware updates are for the manufacturer only.")
            .setView(box)
            .setCancelable(false)
            .setNegativeButton("Cancel") { _, _ -> finish() }
            .setPositiveButton("Log in") { _, _ ->
                lifecycleScope.launch {
                    try {
                        api.login(edit.text?.toString().orEmpty(), "service")
                        status.text = "Logged in as service."
                        setButtons(true)
                    } catch (e: Exception) {
                        toast(e.message ?: "Login failed")
                        serviceLogin()
                    }
                }
            }
            .show()
    }

    private fun confirmAndInstall(fw: Firmware) {
        val s = st ?: return
        val board = s.optString("boardTag")
        if (fw.board != board) { toast("This firmware is for ${fw.board}, the tank is ${s.optString("board")}"); return }
        val max = s.optLong("otaMax")
        if (max > 0 && fw.bytes.size > max) {
            MaterialAlertDialogBuilder(this).setTitle("Too big for a Wi-Fi update")
                .setMessage("The firmware is ${fw.bytes.size / 1024} KB, but the tank has ${max / 1024} KB free for updates. Update this tank by USB.")
                .setPositiveButton("OK", null).show()
            return
        }
        val c = FirmwareFiles.compareVersions(fw.version, s.optString("fw"))
        val note = when { c < 0 -> "\n\nThis is an OLDER version than the tank's ${s.optString("fw")}."
                          c == 0 -> "\n\nThe tank already runs this version."
                          else -> "" }
        MaterialAlertDialogBuilder(this)
            .setTitle("Install firmware ${fw.version}?")
            .setMessage("Tank: ${s.optString("fw")} → ${fw.version} (${fw.bytes.size / 1024} KB).\n\nSettings, calibration, Wi-Fi, history and logs are kept. " +
                "Keep the phone near the tank until the update is confirmed.$note")
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Install") { _, _ -> install(fw) }
            .show()
    }

    private fun install(fw: Firmware) {
        val before = st ?: return
        busy = true
        setButtons(false)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        progress.visibility = View.VISIBLE
        progress.setProgressCompat(0, false)
        status.text = "Uploading firmware ${fw.version}…"
        lifecycleScope.launch {
            try {
                val r = OtaClient.upload(this@FirmwareActivity, api.base, api.token, fw) { p ->
                    runOnUiThread {
                        progress.setProgressCompat(p, true)
                        status.text = if (p < 100) "Uploading… $p %" else "The tank is checking and installing the firmware…"
                    }
                }
                if (!r.optBoolean("ok")) { finishWith("Update failed: ${r.optString("message")}"); return@launch }
                status.text = "Installed. Waiting for the tank to restart…"
                setIndeterminate(true)
                waitForRestart(fw.version, before)
            } catch (e: Exception) {
                finishWith("Upload failed (${e.message}). The tank keeps running its current firmware: move closer and try again.")
            }
        }
    }

    private suspend fun waitForRestart(version: String, before: JSONObject) {
        delay(4000)
        repeat(40) {
            val s = try { TankApi(this, address).statusJson(3000) } catch (e: Exception) { null }
            if (s != null && s.optString("fw") == version) {
                st = s
                device.text = describe(s)
                finishWith("Updated to $version. " + keptReport(before, s), ok = true)
                return
            }
            delay(3000)
        }
        finishWith("The tank has not come back with $version yet. Check its power and Wi-Fi, then reopen this screen.")
    }

    /** The update replaces only the program: compare what must have been kept. */
    private fun keptReport(a: JSONObject, b: JSONObject): String {
        val sa = a.optJSONObject("site") ?: JSONObject(); val sb = b.optJSONObject("site") ?: JSONObject()
        val sameSite = listOf("org", "building", "tank").all { sa.optString(it) == sb.optString(it) }
        val na = a.optJSONArray("sensors")?.length() ?: 1; val nb = b.optJSONArray("sensors")?.length() ?: 1
        val sameCal = a.optDouble("empty") == b.optDouble("empty") && a.optDouble("full") == b.optDouble("full")
        return if (sameSite && na == nb && sameCal) "Site names, $nb sensor(s) and calibration were kept."
        else "Check the settings: site names ${if (sameSite) "kept" else "changed"}, sensors $na → $nb, calibration ${if (sameCal) "kept" else "changed"}."
    }

    private fun finishWith(msg: String, ok: Boolean = false) {
        busy = false
        window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setIndeterminate(false)
        progress.setProgressCompat(if (ok) 100 else 0, false)
        status.text = msg
        setButtons(true)
        if (ok) toast("Firmware updated")
    }

    // The bar can only change mode while hidden
    private fun setIndeterminate(on: Boolean) {
        if (progress.isIndeterminate == on) return
        val v = progress.visibility
        progress.visibility = View.INVISIBLE
        progress.isIndeterminate = on
        progress.visibility = v
    }

    private fun toast(m: String) = Toast.makeText(this, m, Toast.LENGTH_LONG).show()
}
