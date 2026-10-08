/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.View
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.progressindicator.LinearProgressIndicator
import com.google.android.material.textfield.TextInputEditText
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.json.JSONObject

/**
 * Wizard for a new (or moved) tank:
 * 1 connect to its hotspot -> 2 admin login (new password on a new tank) -> 3 names ->
 * 4 pick the home / society Wi-Fi from the tank's own scan -> wait until it joins ->
 * 5 phone back on its normal Wi-Fi, tank saved under its new address.
 */
class SetupActivity : AppCompatActivity() {

    private enum class Step { CONNECT, LOGIN, NAME, NETWORK, JOINING, DONE }

    private lateinit var busy: LinearProgressIndicator
    private lateinit var stepNo: TextView
    private lateinit var stepTitle: TextView
    private lateinit var stepText: TextView
    private lateinit var message: TextView
    private lateinit var primary: MaterialButton
    private lateinit var secondary: MaterialButton
    private lateinit var secLogin: View
    private lateinit var secNewPw: View
    private lateinit var secName: View
    private lateinit var secNet: View
    private lateinit var netGroup: RadioGroup

    private var api: TankApi? = null
    private var site = JSONObject()
    private var namesChanged = false
    private var ssid = ""
    private var newIp = ""
    private var host = ""
    private var title = ""
    private var savedAddress = ""

    private val askLocation = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        if (granted) connectHotspot()
        else error("Android 8 and 9 need the location permission to list Wi-Fi networks. You can also join the WTMS Wi-Fi in Android settings.")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_setup)
        findViewById<MaterialToolbar>(R.id.toolbar).setNavigationOnClickListener { finish() }
        busy = findViewById(R.id.busy)
        stepNo = findViewById(R.id.stepNo)
        stepTitle = findViewById(R.id.stepTitle)
        stepText = findViewById(R.id.stepText)
        message = findViewById(R.id.message)
        primary = findViewById(R.id.primary)
        secondary = findViewById(R.id.secondary)
        secLogin = findViewById(R.id.secLogin)
        secNewPw = findViewById(R.id.secNewPw)
        secName = findViewById(R.id.secName)
        secNet = findViewById(R.id.secNet)
        netGroup = findViewById(R.id.netGroup)
        findViewById<MaterialButton>(R.id.rescan).setOnClickListener { scanTankNetworks() }
        show(Step.CONNECT)
    }

    override fun onDestroy() {
        HotspotLink.release(this)
        super.onDestroy()
    }

    private fun text(id: Int) = findViewById<TextInputEditText>(id).text?.toString()?.trim().orEmpty()
    private fun setText(id: Int, v: String) = findViewById<TextInputEditText>(id).setText(v)
    private fun working(on: Boolean) { busy.visibility = if (on) View.VISIBLE else View.GONE; primary.isEnabled = !on }
    private fun error(msg: String) { working(false); message.text = msg }

    private fun show(step: Step) {
        message.text = ""
        listOf(secLogin, secName, secNet).forEach { it.visibility = View.GONE }
        secondary.visibility = View.VISIBLE
        primary.visibility = View.VISIBLE
        working(false)
        when (step) {
            Step.CONNECT -> {
                stepNo.text = "Step 1 of 5"; stepTitle.text = "Connect to the tank"
                stepText.text = "Power the tank device. Its Wi-Fi is called WTMS-XXXX, or WTMS <building> <tank> once named. " +
                    "If it doesn't appear, press the BOOT button on the board to turn the hotspot on.\n\n" +
                    "Press Connect and choose the tank's Wi-Fi from the list. This phone stays on mobile data for other apps."
                primary.text = "Connect"; primary.setOnClickListener { connectHotspot() }
                secondary.text = "Cancel"; secondary.setOnClickListener { finish() }
            }
            Step.LOGIN -> {
                stepNo.text = "Step 2 of 5"; stepTitle.text = "Log in to the tank"
                stepText.text = "Enter the tank's admin password. A new tank uses admin, and you'll then choose your own password."
                secLogin.visibility = View.VISIBLE; secNewPw.visibility = View.GONE
                primary.text = "Log in"; primary.setOnClickListener { login() }
                secondary.text = "Cancel"; secondary.setOnClickListener { finish() }
            }
            Step.NAME -> {
                stepNo.text = "Step 3 of 5"; stepTitle.text = "Name this tank"
                stepText.text = "Used on the dashboard, in reports and for the tank's Wi-Fi name and web address, so every tank in a society is easy to tell apart. " +
                    "You can change it later."
                secName.visibility = View.VISIBLE
                setText(R.id.inOrg, site.optString("org")); setText(R.id.inBld, site.optString("building")); setText(R.id.inTank, site.optString("tank"))
                primary.text = "Next"; primary.setOnClickListener { saveNames() }
                secondary.text = "Skip"; secondary.setOnClickListener { show(Step.NETWORK) }
            }
            Step.NETWORK -> {
                stepNo.text = "Step 4 of 5"; stepTitle.text = "Choose the home or society Wi-Fi"
                stepText.text = "These are the networks the tank itself can receive, strongest first. Pick one and enter its password. " +
                    "A hidden network can be typed in."
                secNet.visibility = View.VISIBLE
                primary.text = "Connect tank"; primary.setOnClickListener { joinNetwork() }
                secondary.text = "Skip"; secondary.setOnClickListener { skipWifi() }
                if (netGroup.childCount == 0) scanTankNetworks()
            }
            Step.JOINING -> {
                stepNo.text = "Step 4 of 5"; stepTitle.text = "Connecting the tank to $ssid…"
                stepText.text = "This usually takes 5 to 20 seconds. Keep the phone near the tank."
                primary.visibility = View.GONE; secondary.visibility = View.GONE
                working(true)
            }
            Step.DONE -> {
                stepNo.text = "Step 5 of 5"; stepTitle.text = "All set"
                stepText.text = buildString {
                    append("Tank: ").append(title.ifBlank { "Water tank" }).append('\n')
                    if (ssid.isNotBlank()) append("Network: ").append(ssid).append('\n')
                    append("Address: http://").append(savedAddress)
                    if (host.isNotBlank()) append("  (").append(host).append(".local)")
                    append("\n\nThe tank's hotspot turns off by itself now that it is on your Wi-Fi.")
                    if (namesChanged) append(" It restarts once to use its new names.")
                    append("\n\nTo see this tank from anywhere, add a remote address later: long-press the tank in the list.")
                }
                primary.text = "Open dashboard"
                primary.setOnClickListener {
                    startActivity(Intent(this, DashboardActivity::class.java)
                        .putExtra(DashboardActivity.EXTRA_ADDRESS, savedAddress)
                        .putExtra(DashboardActivity.EXTRA_TITLE, title.ifBlank { savedAddress }))
                    finish()
                }
                secondary.text = "Finish"; secondary.setOnClickListener { finish() }
            }
        }
    }

    // ---------------------------------------------------------------- 1. hotspot

    private fun connectHotspot() {
        working(true); message.text = ""
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            HotspotLink.connect(this) { ok, msg -> if (ok) afterHotspot() else error(msg) }
            return
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            working(false); askLocation.launch(Manifest.permission.ACCESS_FINE_LOCATION); return
        }
        HotspotLink.scanLegacy(this) { names ->
            when {
                names.isEmpty() -> error("No tank Wi-Fi (WTMS…) found nearby. Check the tank is powered, press its BOOT button, and turn on location.")
                names.size == 1 -> HotspotLink.connectLegacy(this, names[0]) { ok, msg -> if (ok) afterHotspot() else error(msg) }
                else -> MaterialAlertDialogBuilder(this).setTitle("Choose the tank")
                    .setItems(names.toTypedArray()) { _, i -> HotspotLink.connectLegacy(this, names[i]) { ok, msg -> if (ok) afterHotspot() else error(msg) } }
                    .setOnCancelListener { working(false) }
                    .show()
            }
        }
    }

    private fun afterHotspot() {
        lifecycleScope.launch {
            val a = TankApi(this@SetupActivity, HotspotLink.ADDRESS)
            try {
                val st = a.statusJson()
                site = st.optJSONObject("site") ?: JSONObject()
                host = st.optJSONObject("wifi")?.optString("host").orEmpty()
                api = a
                show(Step.LOGIN)
            } catch (e: Exception) {
                error("Connected to the tank's Wi-Fi, but the tank did not answer (${e.message}). Move closer and try again.")
            }
        }
    }

    // ---------------------------------------------------------------- 2. login

    private fun login() {
        val a = api ?: return show(Step.CONNECT)
        working(true)
        lifecycleScope.launch {
            try {
                if (secNewPw.visibility == View.VISIBLE) {
                    val p1 = text(R.id.inNewPw); val p2 = text(R.id.inNewPw2)
                    when {
                        p1.length < 6 -> return@launch error("Use at least 6 characters.")
                        p1 != p2 -> return@launch error("The two passwords don't match.")
                        p1 == "admin" -> return@launch error("Choose a password other than \"admin\".")
                    }
                    a.changePassword(p1)
                    show(Step.NAME)
                    return@launch
                }
                a.login(text(R.id.inPw))
                if (a.mustChangePassword) {
                    working(false)
                    stepText.text = "Logged in. This tank still uses the default password: choose your own admin password now. Keep it safe; " +
                        "the society's admins need it to change settings."
                    findViewById<View>(R.id.tilPw).visibility = View.GONE
                    secNewPw.visibility = View.VISIBLE
                    primary.text = "Save password"
                } else show(Step.NAME)
            } catch (e: Exception) {
                error(e.message ?: "Login failed")
            }
        }
    }

    // ---------------------------------------------------------------- 3. names

    private fun saveNames() {
        val a = api ?: return show(Step.CONNECT)
        val org = text(R.id.inOrg); val bld = text(R.id.inBld); val tank = text(R.id.inTank)
        if (org == site.optString("org") && bld == site.optString("building") && tank == site.optString("tank")) { show(Step.NETWORK); return }
        working(true)
        lifecycleScope.launch {
            try {
                a.setSite(org, bld, tank)
                namesChanged = true
                site = JSONObject().put("org", org).put("building", bld).put("tank", tank)
                show(Step.NETWORK)
            } catch (e: Exception) { error(e.message ?: "Could not save the names") }
        }
    }

    // ---------------------------------------------------------------- 4. home / society Wi-Fi

    private fun scanTankNetworks() {
        val a = api ?: return
        working(true); message.text = "The tank is scanning for networks…"
        netGroup.removeAllViews()
        lifecycleScope.launch {
            try {
                val nets = a.scanNetworks().sortedByDescending { it.rssi }
                working(false)
                message.text = if (nets.isEmpty()) "The tank found no networks. Move it closer to the router, or type the name below." else ""
                for (n in nets) {
                    val bars = when { n.rssi > -55 -> "▂▄▆█"; n.rssi > -65 -> "▂▄▆"; n.rssi > -75 -> "▂▄"; else -> "▂" }
                    netGroup.addView(RadioButton(this@SetupActivity).apply {
                        text = "${n.ssid}   $bars ${if (n.open) "open" else "🔒"}"
                        id = View.generateViewId()
                        layoutParams = RadioGroup.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT)
                        setOnClickListener { setText(R.id.inSsid, n.ssid); setText(R.id.inWifiPw, "") }
                    })
                }
            } catch (e: Exception) { error("Scan failed: ${e.message}") }
        }
    }

    private fun joinNetwork() {
        val a = api ?: return show(Step.CONNECT)
        ssid = text(R.id.inSsid)
        val pass = findViewById<TextInputEditText>(R.id.inWifiPw).text?.toString().orEmpty()
        if (ssid.isEmpty()) return error("Choose a network or type its name.")
        if (pass.isNotEmpty() && (pass.length < 8 || pass.length > 63)) return error("Wi-Fi passwords are 8 to 63 characters (leave empty for open networks).")
        working(true)
        lifecycleScope.launch {
            try { a.connectWifi(ssid, pass) } catch (e: Exception) { return@launch error(e.message ?: "The tank refused the request") }
            show(Step.JOINING)
            waitForJoin(a)
        }
    }

    /** Polls the tank until it reports the result. Its hotspot may blink while it changes channel. */
    private suspend fun waitForJoin(a: TankApi) {
        val deadline = System.currentTimeMillis() + 45_000
        while (System.currentTimeMillis() < deadline) {
            delay(2000)
            val st = try { a.statusJson(4000) } catch (e: Exception) { continue }
            val wifi = st.optJSONObject("wifi") ?: continue
            when (wifi.optString("state")) {
                "connected" -> {
                    newIp = wifi.optString("ip"); host = wifi.optString("host", host)
                    val s = st.optJSONObject("site") ?: site
                    title = listOf(s.optString("building"), s.optString("tank")).filter { it.isNotBlank() }.joinToString(" · ")
                    finishOnHomeNetwork(a); return
                }
                "failed" -> {
                    show(Step.NETWORK)
                    error(wifi.optString("msg").ifBlank { "The tank could not join $ssid. Check the password." })
                    return
                }
            }
        }
        // No answer: the hotspot probably went away after a successful join. Look for the tank on the home network.
        finishOnHomeNetwork(a)
    }

    // ---------------------------------------------------------------- 5. back on the home network

    private suspend fun finishOnHomeNetwork(a: TankApi) {
        stepText.text = "The tank is on $ssid. Switching this phone back to its normal Wi-Fi…"
        HotspotLink.release(this)
        delay(5000)

        var address = newIp
        if (address.isBlank() && host.isNotBlank()) address = findByHostName(host)
        var reachable = false
        if (address.isNotBlank()) {
            for (attempt in 1..10) {
                if (StatusClient.fetch(this, address, 3000).reachable) { reachable = true; break }
                delay(2000)
            }
        }
        if (address.isBlank()) {
            show(Step.NETWORK)
            error("The tank's new address could not be found. Make sure this phone is on $ssid, then add the tank with \"Find tanks\".")
            return
        }
        if (reachable && namesChanged) {
            try { TankApi(this, address).also { it.useToken(a.token) }.restart() } catch (_: Exception) {}
        }
        // Save (or update) the tank in the app
        val list = DeviceStore.load(this)
        val existing = list.firstOrNull { (host.isNotBlank() && it.host == host) || it.address == address }
        if (existing != null) { existing.address = address; existing.host = host; if (title.isNotBlank()) existing.name = title }
        else list.add(Device(name = title.ifBlank { address }, address = address, host = host))
        DeviceStore.save(this, list)
        savedAddress = address
        show(Step.DONE)
        if (!reachable) message.text = "Saved. This phone can't reach the tank yet: check the phone is on $ssid."
    }

    private suspend fun findByHostName(name: String): String {
        var ip = ""
        val d = Discovery(this) { service, found -> if (service.equals(name, true)) ip = found }
        d.start()
        repeat(16) { if (ip.isEmpty()) delay(500) }
        d.stop()
        return ip
    }

    private fun skipWifi() {
        MaterialAlertDialogBuilder(this)
            .setMessage("Keep the tank on its own hotspot only? It can still be used by connecting to its Wi-Fi.")
            .setNegativeButton("Back", null)
            .setPositiveButton("Yes") { _, _ ->
                val list = DeviceStore.load(this)
                if (list.none { it.address == HotspotLink.ADDRESS && it.host == host }) {
                    val t = listOf(site.optString("building"), site.optString("tank")).filter { it.isNotBlank() }.joinToString(" · ")
                    list.add(Device(name = t.ifBlank { "Tank hotspot" }, address = HotspotLink.ADDRESS, host = host))
                    DeviceStore.save(this, list)
                }
                finish()
            }
            .show()
    }
}
