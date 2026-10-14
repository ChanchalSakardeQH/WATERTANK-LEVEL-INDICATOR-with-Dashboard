/*
 * Water Tanks Monitor System - Android app
 * woodyouloveit.com
 * Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the GNU GPL v3 or later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
package com.woodyouloveit.wtms

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.os.SystemClock
import android.util.AttributeSet
import android.view.View
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin

/**
 * The save-water mascot: a friendly water drop that bounces, blinks and smiles, with waves inside it,
 * hearts rising beside it and twinkling sparkles. Drawn in a 300 x 300 design space and scaled to the view.
 * Animates only while visible on screen; stays still when the phone's animations are turned off.
 */
class SaveWaterView @JvmOverloads constructor(ctx: Context, attrs: AttributeSet? = null) : View(ctx, attrs) {

    private val fill = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val stroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE; strokeCap = Paint.Cap.ROUND; strokeJoin = Paint.Join.ROUND
    }
    private val drop = Path()
    private val wave = Path()
    private val shape = Path()
    private val rect = RectF()
    private val start = SystemClock.uptimeMillis()

    private val cBackground = Color.parseColor("#E0F2F1")
    private val cSparkle = Color.parseColor("#14B8A6")
    private val cShadow = Color.argb(46, 15, 118, 110)
    private val cBody = Color.parseColor("#99F6E4")
    private val cWater1 = Color.parseColor("#2DD4BF")
    private val cWater2 = Color.parseColor("#14B8A6")
    private val cOutline = Color.parseColor("#0F766E")
    private val cDark = Color.parseColor("#134E4A")
    private val cCheek = Color.argb(97, 231, 0, 78)
    private val cHeart = Color.parseColor("#E7004E")

    private fun animating(): Boolean = isShown && ValueAnimator.areAnimatorsEnabled()

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val t = if (ValueAnimator.areAnimatorsEnabled()) (SystemClock.uptimeMillis() - start) / 1000f else 0.6f
        val s = min(width, height) / 300f
        canvas.save()
        canvas.translate((width - 300f * s) / 2f, (height - 300f * s) / 2f)
        canvas.scale(s, s)
        drawMascot(canvas, t)
        canvas.restore()
        if (animating()) postInvalidateOnAnimation()
    }

    override fun onVisibilityChanged(changedView: View, visibility: Int) {
        super.onVisibilityChanged(changedView, visibility)
        if (visibility == VISIBLE) invalidate()
    }

    private fun oval(c: Canvas, x: Float, y: Float, rx: Float, ry: Float) {
        rect.set(x - rx, y - ry, x + rx, y + ry); c.drawOval(rect, fill)
    }

    private fun buildDrop(dy: Float) {
        drop.reset()
        drop.moveTo(150f, 60f + dy)
        drop.cubicTo(150f, 60f + dy, 80f, 140f + dy, 80f, 185f + dy)
        rect.set(80f, 115f + dy, 220f, 255f + dy)
        drop.arcTo(rect, 180f, -180f, false)                    // bottom half, left -> right
        drop.cubicTo(220f, 140f + dy, 150f, 60f + dy, 150f, 60f + dy)
        drop.close()
    }

    private fun sparkle(c: Canvas, x: Float, y: Float, r: Float) {
        shape.reset()
        shape.moveTo(x, y - r)
        shape.quadTo(x, y, x + r, y); shape.quadTo(x, y, x, y + r)
        shape.quadTo(x, y, x - r, y); shape.quadTo(x, y, x, y - r)
        shape.close()
        c.drawPath(shape, fill)
    }

    private fun heart(c: Canvas, x: Float, y: Float, sz: Float) {
        shape.reset()
        shape.moveTo(x, y + sz * 0.35f)
        shape.cubicTo(x - sz * 0.9f, y - sz * 0.25f, x - sz * 0.45f, y - sz * 0.95f, x, y - sz * 0.45f)
        shape.cubicTo(x + sz * 0.45f, y - sz * 0.95f, x + sz * 0.9f, y - sz * 0.25f, x, y + sz * 0.35f)
        shape.close()
        c.drawPath(shape, fill)
    }

    private fun waveFill(c: Canvas, level: Float, amp: Float, k: Float, phase: Float, bottom: Float, color: Int) {
        wave.reset()
        wave.moveTo(70f, bottom)
        var x = 70f
        while (x <= 230f) { wave.lineTo(x, level + amp * sin(x / k + phase)); x += 4f }
        wave.lineTo(230f, bottom)
        wave.close()
        fill.color = color
        c.drawPath(wave, fill)
    }

    private fun drawMascot(c: Canvas, t: Float) {
        // Background disc
        fill.color = cBackground; fill.alpha = 255
        c.drawCircle(150f, 165f, 130f, fill)

        // Sparkles twinkling
        val sparkles = floatArrayOf(52f, 92f, 9f, 0f, 250f, 80f, 7f, 1.3f, 262f, 205f, 8f, 2.1f, 40f, 220f, 6f, 0.7f)
        fill.color = cSparkle
        for (i in sparkles.indices step 4) {
            val a = 0.35f + 0.65f * (0.5f + 0.5f * sin(t * 2.2f + sparkles[i + 3]))
            fill.alpha = (255 * a).toInt()
            sparkle(c, sparkles[i], sparkles[i + 1], sparkles[i + 2])
        }
        fill.alpha = 255

        // Bounce and shadow
        val b = abs(sin(Math.PI.toFloat() * t / 1.2f))
        val dy = -10f * b
        fill.color = cShadow
        oval(c, 150f, 268f, 52f * (1 - 0.22f * b), 8f * (1 - 0.22f * b))

        // Body, then the water with moving waves clipped to the body
        buildDrop(dy)
        fill.color = cBody
        c.drawPath(drop, fill)
        c.save()
        c.clipPath(drop)
        val level = 206f + dy
        val ph = t * 2.6f
        waveFill(c, level, 6f, 18f, ph, 270f + dy, cWater1)
        waveFill(c, level + 14f, 5f, 15f, -ph * 1.3f, 270f + dy, cWater2)
        c.restore()

        // Outline
        stroke.color = cOutline; stroke.strokeWidth = 5f
        c.drawPath(drop, stroke)

        // Shine
        fill.color = Color.argb(191, 255, 255, 255)
        c.save(); c.rotate(28.6f, 118f, 140f + dy); oval(c, 118f, 140f + dy, 9f, 20f); c.restore()

        // Eyes, blinking every 3.2 s
        val cyc = t % 3.2f
        val open = if (cyc < 0.16f) abs(cyc - 0.08f) / 0.08f else 1f
        for (ex in floatArrayOf(125f, 175f)) {
            fill.color = Color.WHITE
            oval(c, ex, 168f + dy, 13f, 16f * max(open, 0.08f))
            if (open > 0.3f) {
                fill.color = cDark
                oval(c, ex + 2f, 171f + dy, 7f, 9f * open)
                fill.color = Color.WHITE
                c.drawCircle(ex + 5f, 166f + dy, 2.6f, fill)
            }
        }

        // Cheeks and smile
        fill.color = cCheek
        oval(c, 106f, 188f + dy, 11f, 7f)
        oval(c, 194f, 188f + dy, 11f, 7f)
        stroke.color = cDark; stroke.strokeWidth = 5f
        shape.reset()
        shape.moveTo(131f, 196f + dy)
        shape.quadTo(150f, 214f + dy, 169f, 196f + dy)
        c.drawPath(shape, stroke)

        // Hearts rising beside the drop (3, staggered, 3 s life)
        fill.color = cHeart
        for (i in 0 until 3) {
            val life = ((t + i) % 3f) / 3f
            val hx = 232f + i * 10f + 9f * sin(life * 6f + i * 2f)
            val hy = 150f - 95f * life + dy * 0.5f
            val a = if (life < 0.15f) life / 0.15f else 1f - (life - 0.15f) / 0.85f
            fill.alpha = (255 * a.coerceIn(0f, 1f)).toInt()
            heart(c, hx, hy, 16f - i * 3f)
        }
        fill.alpha = 255
    }
}
