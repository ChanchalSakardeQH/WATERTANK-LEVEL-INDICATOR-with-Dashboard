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
import android.net.Uri
import android.os.Build
import android.animation.ObjectAnimator
import android.animation.ValueAnimator
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.View
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton
import com.google.android.material.snackbar.Snackbar
import com.google.android.material.textfield.TextInputEditText
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    private lateinit var adapter: DeviceAdapter
    private lateinit var swipe: SwipeRefreshLayout
    private lateinit var empty: View
    private lateinit var emptyTip: TextView
    private val tipHandler = Handler(Looper.getMainLooper())
    private var tipIndex = 0
    private var titlePulse: ObjectAnimator? = null
    private val nextTip = object : Runnable {
        override fun run() {
            val tips = resources.getStringArray(R.array.save_water_tips)
            tipIndex = (tipIndex + 1) % tips.size
            if (ValueAnimator.areAnimatorsEnabled()) {
                emptyTip.animate().alpha(0f).setDuration(250).withEndAction {
                    emptyTip.text = tips[tipIndex]
                    emptyTip.animate().alpha(1f).setDuration(350).start()
                }.start()
            } else emptyTip.text = tips[tipIndex]
            tipHandler.postDelayed(this, 4500)
        }
    }
    private var devices = mutableListOf<Device>()
    private val workingAddress = mutableMapOf<String, String>()   // device id -> address that answered last
    private var lastAutoScan = 0L

    private val askNotifications = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        if (!granted) toast("Notifications are off for this app. You can turn them on in Android settings.")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        toolbar.inflateMenu(R.menu.main_menu)
        toolbar.menu.findItem(R.id.action_scan)?.icon?.setTint(ContextCompat.getColor(this, R.color.white))
        toolbar.setOnMenuItemClickListener {
            when (it.itemId) {
                R.id.action_scan -> { scan(); true }
                R.id.action_alerts -> { alertSettings(); true }
                R.id.action_about -> { about(); true }
                else -> false
            }
        }

        adapter = DeviceAdapter(onClick = this::openDashboard, onLongClick = this::deviceMenu)
        findViewById<RecyclerView>(R.id.list).apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = this@MainActivity.adapter
        }
        swipe = findViewById(R.id.swipe)
        swipe.setOnRefreshListener { refresh() }
        empty = findViewById(R.id.empty)
        emptyTip = findViewById(R.id.emptyTip)
        emptyTip.text = resources.getStringArray(R.array.save_water_tips)[0]
        findViewById<View>(R.id.emptySetup).setOnClickListener { startActivity(Intent(this, SetupActivity::class.java)) }
        findViewById<View>(R.id.emptyFind).setOnClickListener { scan() }
        findViewById<ExtendedFloatingActionButton>(R.id.fab).setOnClickListener { addChoices() }

        if (Build.VERSION.SDK_INT >= 33 && AlertSettings.enabled(this) &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            askNotifications.launch(Manifest.permission.POST_NOTIFICATIONS)
        }
    }

    override fun onResume() {
        super.onResume()
        devices = DeviceStore.load(this)
        adapter.submit(devices)
        adapter.setAdmins(AuthStore.ids(this))
        empty.visibility = if (devices.isEmpty()) View.VISIBLE else View.GONE
        if (devices.isEmpty()) startEmptyAnimations() else stopEmptyAnimations()
        refresh()
        if (devices.isEmpty()) autoDiscover()
    }

    /** Check every saved tank at once. */
    private fun refresh() {
        if (devices.isEmpty()) { swipe.isRefreshing = false; return }
        swipe.isRefreshing = true
        lifecycleScope.launch {
            val results = mutableMapOf<String, Boolean>()
            devices.map { d ->
                async {
                    adapter.setStatus(d.id, null)
                    val r = StatusClient.fetchAny(this@MainActivity, d)
                    val s = r.status
                    workingAddress[d.id] = r.address
                    adapter.setStatus(d.id, s, r.viaRemote)
                    results[d.id] = s.reachable
                    if (s.reachable && s.host.isNotBlank() && s.host != d.host) { d.host = s.host; DeviceStore.save(this@MainActivity, devices) }
                }
            }.awaitAll()
            swipe.isRefreshing = false
            if (results.values.any { !it }) autoDiscover()
        }
    }

    /**
     * On Wi-Fi, look for tanks by themselves: when a saved tank doesn't answer (its router address may have
     * changed) or no tank is saved yet. Moved tanks are updated by host name; new ones are offered to add.
     */
    private fun autoDiscover() {
        if (NetUtil.wifiNetwork(this) == null) return
        if (SystemClock.elapsedRealtime() - lastAutoScan < 60_000 && lastAutoScan != 0L) return
        lastAutoScan = SystemClock.elapsedRealtime()
        val found = linkedMapOf<String, TankStatus>()
        val discovery = Discovery(this) { _, ip ->
            if (found.containsKey(ip)) return@Discovery
            lifecycleScope.launch {
                val s = StatusClient.fetch(this@MainActivity, ip, 3000)
                if (s.reachable) found[ip] = s
            }
        }
        discovery.start()
        lifecycleScope.launch {
            delay(6000)
            discovery.stop()
            var moved = 0
            for ((ip, s) in found) {
                devices.firstOrNull { it.host.isNotBlank() && it.host == s.host && it.address != ip && it.address != HotspotLink.ADDRESS }
                    ?.let { it.address = ip; moved++ }
            }
            if (moved > 0) { DeviceStore.save(this@MainActivity, devices); toast("Found $moved tank(s) at a new address"); onResume() }
            val fresh = found.filter { (ip, s) -> devices.none { it.address == ip || (it.host.isNotBlank() && it.host == s.host) } }
            if (fresh.isEmpty()) return@launch
            val names = fresh.values.joinToString(", ") { it.title.ifBlank { it.host } }
            Snackbar.make(swipe, "Found ${fresh.size} new tank(s) on this Wi-Fi: $names", Snackbar.LENGTH_INDEFINITE)
                .setAction("Add") {
                    fresh.forEach { (ip, s) -> devices.add(Device(name = s.title.ifBlank { ip }, address = ip, host = s.host)) }
                    DeviceStore.save(this@MainActivity, devices); onResume()
                }
                .show()
        }
    }

    private fun openDashboard(d: Device) {
        startActivity(Intent(this, DashboardActivity::class.java)
            .putExtra(DashboardActivity.EXTRA_ADDRESS, workingAddress[d.id] ?: d.address)
            .putExtra(DashboardActivity.EXTRA_DEVICE_ID, d.id)
            .putExtra(DashboardActivity.EXTRA_TITLE, d.name.ifBlank { d.address }))
    }

    override fun onPause() {
        stopEmptyAnimations()
        super.onPause()
    }

    /** Rotating save-water tips and a gently pulsing headline while no tank is saved. */
    private fun startEmptyAnimations() {
        tipHandler.removeCallbacks(nextTip)
        tipHandler.postDelayed(nextTip, 4500)
        if (titlePulse == null && ValueAnimator.areAnimatorsEnabled()) {
            val title = findViewById<View>(R.id.emptyTitle)
            titlePulse = ObjectAnimator.ofPropertyValuesHolder(title,
                android.animation.PropertyValuesHolder.ofFloat(View.SCALE_X, 1f, 1.07f),
                android.animation.PropertyValuesHolder.ofFloat(View.SCALE_Y, 1f, 1.07f)).apply {
                duration = 900; repeatCount = ValueAnimator.INFINITE; repeatMode = ValueAnimator.REVERSE; start()
            }
        }
    }

    private fun stopEmptyAnimations() {
        tipHandler.removeCallbacks(nextTip)
        titlePulse?.cancel(); titlePulse = null
        findViewById<View>(R.id.emptyTitle)?.apply { scaleX = 1f; scaleY = 1f }
    }

    // ---------------------------------------------------------------- add / edit

    private fun addChoices() {
        val items = arrayOf(
            "Set up a new tank (connect it to your Wi-Fi)",
            "Open a tank's hotspot (WTMS-…)",
            "Add by address")
        MaterialAlertDialogBuilder(this)
            .setTitle("Add tank")
            .setItems(items) { _, which ->
                when (which) {
                    0 -> startActivity(Intent(this, SetupActivity::class.java))
                    1 -> openHotspot()
                    2 -> addDialog(null)
                }
            }
            .show()
    }

    /** View a tank through its own hotspot, without setting it up. */
    private fun openHotspot() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            toast("Join the tank's WTMS Wi-Fi in Android settings, then come back and use Add by address: 192.168.4.1")
            try { startActivity(Intent(android.provider.Settings.ACTION_WIFI_SETTINGS)) } catch (_: Exception) {}
            return
        }
        HotspotLink.connect(this) { ok, msg ->
            if (!ok) { toast(msg); return@connect }
            startActivity(Intent(this, DashboardActivity::class.java)
                .putExtra(DashboardActivity.EXTRA_ADDRESS, HotspotLink.ADDRESS)
                .putExtra(DashboardActivity.EXTRA_TITLE, "Tank hotspot")
                .putExtra(DashboardActivity.EXTRA_RELEASE_HOTSPOT, true))
        }
    }

    private fun addDialog(existing: Device?) {
        val view = layoutInflater.inflate(R.layout.dialog_add_device, null)
        val addr = view.findViewById<TextInputEditText>(R.id.address)
        val name = view.findViewById<TextInputEditText>(R.id.name)
        val remote = view.findViewById<TextInputEditText>(R.id.remote)
        if (existing != null) { addr.setText(existing.address); name.setText(existing.name); remote.setText(existing.remote) }
        MaterialAlertDialogBuilder(this)
            .setTitle(if (existing == null) "Add tank" else "Edit tank")
            .setView(view)
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Check and save") { _, _ ->
                val a = addr.text?.toString()?.trim().orEmpty()
                if (a.isEmpty()) { toast("Enter the device address"); return@setPositiveButton }
                checkAndSave(existing, a, name.text?.toString()?.trim().orEmpty(), remote.text?.toString()?.trim().orEmpty())
            }
            .show()
    }

    private fun checkAndSave(existing: Device?, address: String, name: String, remote: String) {
        swipe.isRefreshing = true
        lifecycleScope.launch {
            val probe = Device(name = name, address = address, remote = remote)
            val s = StatusClient.fetchAny(this@MainActivity, probe).status
            swipe.isRefreshing = false
            val save = {
                val title = name.ifBlank { s.title.ifBlank { address } }
                if (existing == null) devices.add(Device(name = title, address = address, host = s.host, remote = remote))
                else {
                    existing.address = address; existing.name = title; existing.remote = remote
                    if (s.host.isNotBlank()) existing.host = s.host
                }
                DeviceStore.save(this@MainActivity, devices)
                onResume()
            }
            if (s.reachable) { save(); toast("Found ${s.title.ifBlank { "the tank" }} (firmware ${s.fw})") }
            else MaterialAlertDialogBuilder(this@MainActivity)
                .setTitle("Tank not reachable")
                .setMessage("No Water Tanks Monitor device answered at $address (${s.error}).\n\n" +
                    "On the device hotspot, connect this phone to the WTMS Wi-Fi first. Save anyway?")
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Save anyway") { _, _ -> save() }
                .show()
        }
    }

    private fun deviceMenu(d: Device) {
        val items = arrayOf("Open dashboard", "Rename, address or remote address", "Open in browser", "Firmware update (service)", "Remove")
        MaterialAlertDialogBuilder(this)
            .setTitle(d.name.ifBlank { d.address })
            .setItems(items) { _, which ->
                when (which) {
                    0 -> openDashboard(d)
                    1 -> addDialog(d)
                    2 -> try { startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(StatusClient.baseUrl(d.address)))) } catch (_: Exception) {}
                    3 -> startActivity(Intent(this, FirmwareActivity::class.java)
                        .putExtra(FirmwareActivity.EXTRA_ADDRESS, workingAddress[d.id] ?: d.address)
                        .putExtra(FirmwareActivity.EXTRA_TITLE, d.name.ifBlank { d.address }))
                    4 -> MaterialAlertDialogBuilder(this)
                        .setMessage("Remove ${d.name.ifBlank { d.address }} from this app? The device itself is not changed.")
                        .setNegativeButton("Cancel", null)
                        .setPositiveButton("Remove") { _, _ ->
                            devices.removeAll { it.id == d.id }; AuthStore.clear(this, d.id); DeviceStore.save(this, devices); onResume()
                        }
                        .show()
                }
            }
            .show()
    }

    // ---------------------------------------------------------------- discovery

    private fun scan() {
        val found = linkedMapOf<String, Pair<String, TankStatus>>()   // ip -> (service name, status)
        val dialog = MaterialAlertDialogBuilder(this)
            .setTitle("Finding tanks…")
            .setMessage("Searching this Wi-Fi network for Water Tanks Monitor devices (about 8 seconds).")
            .setCancelable(false)
            .create()
        dialog.show()

        val discovery = Discovery(this) { service, ip ->
            if (found.containsKey(ip)) return@Discovery
            lifecycleScope.launch {
                val s = StatusClient.fetch(this@MainActivity, ip, 4000)
                if (s.reachable) found[ip] = service to s
            }
        }
        discovery.start()
        lifecycleScope.launch {
            // The device hotspot is not announced over mDNS, so check its fixed address as well
            val hotspot = StatusClient.fetch(this@MainActivity, "192.168.4.1", 3000)
            if (hotspot.reachable) found["192.168.4.1"] = "hotspot" to hotspot
            delay(8000)
            discovery.stop()
            dialog.dismiss()
            showScanResults(found)
        }
    }

    private fun showScanResults(found: Map<String, Pair<String, TankStatus>>) {
        // Saved tanks whose IP changed are updated by host name
        var moved = 0
        for ((ip, pair) in found) {
            val s = pair.second
            devices.firstOrNull { it.host.isNotBlank() && it.host == s.host && it.address != ip && it.address != "192.168.4.1" }
                ?.let { it.address = ip; moved++ }
        }
        if (moved > 0) DeviceStore.save(this, devices)

        val fresh = found.filter { (ip, pair) -> devices.none { it.address == ip || (it.host.isNotBlank() && it.host == pair.second.host) } }
        if (fresh.isEmpty()) {
            val msg = if (found.isEmpty()) "No tanks found on this Wi-Fi.\n\nCheck the phone is on the same Wi-Fi as the tanks, or on a device hotspot (WTMS-…). " +
                "Some routers block device discovery; then add the tank by its IP address."
            else "All tanks found are already in the list." + if (moved > 0) "\nUpdated the address of $moved tank(s)." else ""
            MaterialAlertDialogBuilder(this).setTitle("Find tanks").setMessage(msg).setPositiveButton("OK", null).show()
            onResume(); return
        }
        val ips = fresh.keys.toList()
        val labels = ips.map { ip -> val s = fresh.getValue(ip).second; "${s.title.ifBlank { s.host.ifBlank { "Tank" } }}  ($ip)" }.toTypedArray()
        val checked = BooleanArray(ips.size) { true }
        MaterialAlertDialogBuilder(this)
            .setTitle("Tanks found")
            .setMultiChoiceItems(labels, checked) { _, i, on -> checked[i] = on }
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Add") { _, _ ->
                ips.forEachIndexed { i, ip ->
                    if (checked[i]) { val s = fresh.getValue(ip).second; devices.add(Device(name = s.title.ifBlank { ip }, address = ip, host = s.host)) }
                }
                DeviceStore.save(this, devices); onResume()
            }
            .show()
    }

    // ---------------------------------------------------------------- notifications

    private fun alertSettings() {
        val labels = arrayOf("Notifications on", "Low water", "Sensor fault (no echo)", "Tank not reachable for 1 hour")
        val checked = booleanArrayOf(AlertSettings.enabled(this), AlertSettings.low(this), AlertSettings.fault(this), AlertSettings.offline(this))
        MaterialAlertDialogBuilder(this)
            .setTitle("Notifications")
            .setMultiChoiceItems(labels, checked) { _, i, on -> checked[i] = on }
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Save") { _, _ ->
                AlertSettings.save(this, checked[0], checked[1], checked[2], checked[3])
                AlertWorker.schedule(this)
                if (checked[0] && Build.VERSION.SDK_INT >= 33 &&
                    ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                    askNotifications.launch(Manifest.permission.POST_NOTIFICATIONS)
                }
                toast(if (checked[0]) "The app checks your tanks about every 15 minutes" else "Notifications off")
            }
            .show()
    }

    // ---------------------------------------------------------------- about

    private fun about() {
        val pad = (20 * resources.displayMetrics.density).toInt()
        val box = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; setPadding(pad, pad, pad, 0) }
        box.addView(ImageView(this).apply {
            setImageResource(R.drawable.brand_logo); adjustViewBounds = true
            layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, (56 * resources.displayMetrics.density).toInt())
        })
        box.addView(TextView(this).apply {
            text = "${getString(R.string.app_name)} ${BuildConfig.VERSION_NAME}\n\n" +
                "Shows every tank's level, opens each device's dashboard, shares reports by WhatsApp or email, " +
                "and alerts you about low water.\n\n${getString(R.string.copyright)}"
            setPadding(0, pad, 0, 0)
        })
        MaterialAlertDialogBuilder(this)
            .setView(box)
            .setNeutralButton("woodyouloveit.com") { _, _ ->
                try { startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://woodyouloveit.com"))) } catch (_: Exception) {}
            }
            .setPositiveButton("OK", null)
            .show()
    }

    private fun toast(msg: String) = Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
}
