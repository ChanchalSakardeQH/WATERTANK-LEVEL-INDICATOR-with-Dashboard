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
        h.subtitle.text = listOf(s?.org.orEmpty(), where).filter { it.isNotBlank() }.joinToString(" · ")
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
            !s.valid -> {
                h.state.text = "Sensor fault (no echo)"; h.state.setTextColor(color(R.color.bad))
                h.level.text = "–"; h.litres.text = ""; h.bar.setProgressCompat(0, false)
            }
            else -> {
                val pct = s.level.coerceIn(0.0, 100.0)
                h.level.text = "${Math.round(pct)}%"
                h.litres.text = if (s.capacity > 0) "≈ ${s.litres} of ${s.capacity} L" else ""
                h.bar.setProgressCompat(Math.round(pct).toInt(), true)
                val (text, c) = when {
                    s.filling -> "Filling (motor on)" to R.color.brand_teal
                    s.low -> "Low water" to R.color.warn
                    else -> "OK" to R.color.ok
                }
                h.state.text = text; h.state.setTextColor(color(c))
                h.bar.setIndicatorColor(color(if (s.low) R.color.warn else R.color.brand_teal))
            }
        }
    }
}
