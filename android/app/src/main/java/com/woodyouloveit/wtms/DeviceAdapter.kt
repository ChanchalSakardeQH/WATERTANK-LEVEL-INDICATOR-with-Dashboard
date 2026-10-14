/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.progressindicator.LinearProgressIndicator

class DeviceAdapter(
    private val onClick: (Device) -> Unit,
    private val onLongClick: (Device) -> Unit
) : RecyclerView.Adapter<DeviceAdapter.Holder>() {

    private var items: List<Device> = emptyList()
    private val status = mutableMapOf<String, TankStatus?>()   // null = checking
    private val remote = mutableSetOf<String>()                 // devices reached through their remote address
    private var admins: Set<String> = emptySet()                // devices with an admin session in this app

    fun setAdmins(ids: Set<String>) { admins = ids; notifyDataSetChanged() }

    fun submit(list: List<Device>) { items = list.toList(); notifyDataSetChanged() }
    fun setStatus(id: String, s: TankStatus?, viaRemote: Boolean = false) {
        status[id] = s
        if (viaRemote) remote.add(id) else remote.remove(id)
        val i = items.indexOfFirst { it.id == id }
        if (i >= 0) notifyItemChanged(i)
    }

    class Holder(v: View) : RecyclerView.ViewHolder(v) {
        val title: TextView = v.findViewById(R.id.title)
        val subtitle: TextView = v.findViewById(R.id.subtitle)
        val state: TextView = v.findViewById(R.id.status)
        val level: TextView = v.findViewById(R.id.level)
        val litres: TextView = v.findViewById(R.id.litres)
        val bar: LinearProgressIndicator = v.findViewById(R.id.bar)
        val compBox: LinearLayout = v.findViewById(R.id.compBox)
    }

    /** One small row per compartment (name, level, bar) when the tank has several. */
    private fun bindCompartments(h: Holder, s: TankStatus?) {
        h.compBox.removeAllViews()
        if (s == null || !s.reachable || !s.multi) { h.compBox.visibility = View.GONE; return }
        h.compBox.visibility = View.VISIBLE
        val ctx = h.itemView.context
        val dp = ctx.resources.displayMetrics.density
        for (c in s.compartments) {
            val row = LinearLayout(ctx).apply { orientation = LinearLayout.HORIZONTAL; setPadding(0, (4 * dp).toInt(), 0, 0) }
            row.addView(TextView(ctx).apply {
                text = c.name; layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
            })
            row.addView(TextView(ctx).apply {
                text = when {
                    !c.valid -> "no echo"
                    c.capacity > 0 -> "${Math.round(c.level)}% · ${c.litres} L"
                    else -> "${Math.round(c.level)}%"
                } + if (c.filling) " · filling" else ""
                setTextColor(ContextCompat.getColor(ctx, when {
                    !c.valid -> R.color.bad
                    c.level < s.lowAlarm -> R.color.warn
                    else -> R.color.brand_teal
                }))
            })
            h.compBox.addView(row)
            h.compBox.addView(LinearProgressIndicator(ctx).apply {
                max = 100; trackThickness = (5 * dp).toInt(); trackCornerRadius = (3 * dp).toInt()
                setIndicatorColor(ContextCompat.getColor(ctx, if (c.valid && c.level < s.lowAlarm) R.color.warn else R.color.brand_teal_light))
                setProgressCompat(if (c.valid) Math.round(c.level.coerceIn(0.0, 100.0)).toInt() else 0, false)
            })
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
        Holder(LayoutInflater.from(parent.context).inflate(R.layout.item_device, parent, false))

    override fun getItemCount() = items.size

    override fun onBindViewHolder(h: Holder, position: Int) {
        val d = items[position]
        val s = status[d.id]
        val ctx = h.itemView.context
        h.title.text = s?.title?.takeIf { it.isNotBlank() } ?: d.name.ifBlank { d.address }
        val where = if (d.id in remote) "remote" else d.address
        h.subtitle.text = listOf(s?.org.orEmpty(), where, if (d.id in admins) "admin" else "")
            .filter { it.isNotBlank() }.joinToString(" · ")
        bindCompartments(h, s)
        h.itemView.setOnClickListener { onClick(d) }
        h.itemView.setOnLongClickListener { onLongClick(d); true }

        fun color(res: Int) = ContextCompat.getColor(ctx, res)
        when {
            s == null -> {
                h.state.text = "Checking…"; h.state.setTextColor(h.subtitle.currentTextColor)
                h.level.text = "…"; h.litres.text = ""; h.bar.setProgressCompat(0, false)
            }
            !s.reachable -> {
                h.state.text = "Not reachable"; h.state.setTextColor(color(R.color.bad))
                h.level.text = "–"; h.litres.text = ""; h.bar.setProgressCompat(0, false)
            }
            !s.valid && !(s.multi && s.totalValid) -> {
                h.state.text = if (s.multi) "Sensor fault: " + s.faultNames.joinToString(", ") else "Sensor fault (no echo)"
                h.state.setTextColor(color(R.color.bad))
                h.level.text = "–"; h.litres.text = ""; h.bar.setProgressCompat(0, false)
            }
            else -> {
                val pct = s.level.coerceIn(0.0, 100.0)
                h.level.text = "${Math.round(pct)}%"
                h.litres.text = if (s.capacity > 0) "≈ ${s.litres} of ${s.capacity} L" else ""
                h.bar.setProgressCompat(Math.round(pct).toInt(), true)
                val (text, c) = when {
                    s.faultNames.isNotEmpty() -> "Sensor fault: " + s.faultNames.joinToString(", ") to R.color.bad
                    s.anyFilling -> "Filling (motor on)" to R.color.brand_teal
                    s.low -> (if (s.multi) "Low water: " + s.lowNames.joinToString(", ") else "Low water") to R.color.warn
                    else -> (if (s.multi) "OK · whole tank" else "OK") to R.color.ok
                }
                h.state.text = text; h.state.setTextColor(color(c))
                h.bar.setIndicatorColor(color(if (s.low) R.color.warn else R.color.brand_teal))
            }
        }
    }
}
