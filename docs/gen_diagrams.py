# Water Tanks Monitor System - wiring diagram generator
# woodyouloveit.com
# Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the license below.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Source: https://github.com/ChanchalSakardeQH/WATERTANK-LEVEL-INDICATOR-with-Dashboard
# SPDX-License-Identifier: GPL-3.0-or-later

"""Generate wiring diagrams (SVG) for ESP32 DevKit 30-pin and NodeMCU ESP8266 30-pin."""

C_3V3 = "#6a1b9a"
C_5V, C_GND, C_TRIG, C_ECHO, C_DIN = "#d32f2f", "#333333", "#ef6c00", "#1565c0", "#2e7d32"
FONT = "font-family='Segoe UI,Helvetica,Arial,sans-serif'"
MONO = "font-family='Consolas,Menlo,monospace'"

BX0, BX1, BY0, BY1 = 140, 340, 90, 520

# Brand logo embedded in every diagram (docs/logo.png, resized)
def _load_logo():
    import base64, io, os
    try:
        from PIL import Image
        im = Image.open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "logo.png")).convert("RGBA")
        im = im.resize((400, round(400 * im.height / im.width)), Image.LANCZOS)
        buf = io.BytesIO(); im.save(buf, "PNG", optimize=True)
        return base64.b64encode(buf.getvalue()).decode()
    except Exception:
        return ""
LOGO_B64 = _load_logo()          # board rectangle
def pin_y(i): return 120 + 26 * i                 # i = 0..14


class Svg:
    def __init__(self, w, h):
        self.w, self.h, self.p = w, h, []

    def add(self, s): self.p.append(s)

    def text(self, x, y, s, size=13, anchor="start", weight="normal", fill="#222", mono=False):
        f = MONO if mono else FONT
        self.add(f"<text x='{x}' y='{y}' font-size='{size}' text-anchor='{anchor}' "
                 f"font-weight='{weight}' fill='{fill}' {f}>{s}</text>")

    def wire(self, pts, color, w=2.5):
        d = " ".join(f"{x},{y}" for x, y in pts)
        self.add(f"<polyline points='{d}' fill='none' stroke='{color}' stroke-width='{w}' "
                 f"stroke-linejoin='round' stroke-linecap='round'/>")

    def dot(self, x, y, color):
        self.add(f"<circle cx='{x}' cy='{y}' r='4' fill='{color}'/>")

    def resistor_h(self, x0, x1, y, color, label):
        """Horizontal resistor between x0..x1 on a wire at y."""
        m, w = (x0 + x1) / 2, 44
        self.wire([(x0, y), (m - w / 2, y)], color)
        self.wire([(m + w / 2, y), (x1, y)], color)
        self.add(f"<rect x='{m - w/2}' y='{y - 8}' width='{w}' height='16' rx='2' "
                 f"fill='#fff8e1' stroke='#5d4037' stroke-width='1.5'/>")
        self.text(m, y - 13, label, 12, "middle", "bold", "#5d4037")

    def resistor_v(self, x, y0, y1, color, label):
        m, h = (y0 + y1) / 2, 40
        self.wire([(x, y0), (x, m - h / 2)], color)
        self.wire([(x, m + h / 2), (x, y1)], C_GND)
        self.add(f"<rect x='{x - 8}' y='{m - h/2}' width='16' height='{h}' rx='2' "
                 f"fill='#fff8e1' stroke='#5d4037' stroke-width='1.5'/>")
        self.text(x + 14, m + 4, label, 12, "start", "bold", "#5d4037")

    def gnd(self, x, y):
        """Ground symbol with its top at (x, y)."""
        for i, hw in enumerate((12, 8, 4)):
            yy = y + i * 5
            self.add(f"<line x1='{x-hw}' y1='{yy}' x2='{x+hw}' y2='{yy}' stroke='{C_GND}' stroke-width='2'/>")

    def v5(self, x, y):
        """+5V flag with its bottom at (x, y)."""
        self.add(f"<polygon points='{x-7},{y} {x+7},{y} {x},{y-9}' fill='{C_5V}'/>")
        self.text(x, y - 13, "+5V", 12, "middle", "bold", C_5V)

    def box(self, x, y, w, h, title, sub, fill="#eef3fb", stroke="#3949ab"):
        self.add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='8' fill='{fill}' "
                 f"stroke='{stroke}' stroke-width='1.8'/>")
        cy = y + h / 2
        self.text(x + w / 2 + 20, cy - 2, title, 14, "middle", "bold", stroke)
        self.text(x + w / 2 + 20, cy + 16, sub, 11, "middle", "normal", "#555")

    def pin_label(self, x, y, s, anchor="start"):
        self.text(x, y + 4, s, 11, anchor, "bold", "#333", mono=True)

    def out(self):
        head = (f"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' "
                f"viewBox='0 0 {self.w} {self.h}' width='{self.w}' height='{self.h}'>"
                f"<rect width='100%' height='100%' fill='#ffffff'/>")
        if LOGO_B64:   # brand logo, top right
            head += (f"<image x='{self.w - 222}' y='16' width='200' height='33' "
                     f"href='data:image/png;base64,{LOGO_B64}' xlink:href='data:image/png;base64,{LOGO_B64}'/>")
        return head + "".join(self.p) + "</svg>"


def draw_board(s, name, sub, left, right, used):
    """left/right: 15 pin names top->bottom (antenna top, USB bottom). used: {label: color}."""
    s.add(f"<rect x='{BX0}' y='{BY0}' width='{BX1-BX0}' height='{BY1-BY0}' rx='10' "
          f"fill='#263238' stroke='#111' stroke-width='2'/>")
    s.add(f"<rect x='{BX0+55}' y='{BY0+10}' width='90' height='70' rx='4' fill='#90a4ae'/>")
    s.text((BX0 + BX1) / 2, BY0 + 50, "antenna", 11, "middle", fill="#263238")
    s.add(f"<rect x='{(BX0+BX1)/2-22}' y='{BY1-6}' width='44' height='22' rx='3' fill='#b0bec5' stroke='#555'/>")
    s.text((BX0 + BX1) / 2, BY1 + 31, "USB", 11, "middle", "bold", "#555")
    s.text((BX0 + BX1) / 2, 260, name, 15, "middle", "bold", "#ffffff")
    s.text((BX0 + BX1) / 2, 280, sub, 12, "middle", fill="#cfd8dc")
    s.text((BX0 + BX1) / 2, 298, "top view", 11, "middle", fill="#90a4ae")
    for side, names, x in (("L", left, BX0), ("R", right, BX1)):
        for i, n in enumerate(names):
            y = pin_y(i)
            col = used.get(f"{side}{i}")
            s.add(f"<circle cx='{x}' cy='{y}' r='5' fill='{col or '#ffd54f'}' stroke='#111'/>")
            fill = "#ffffff" if col else "#90a4ae"
            anchor, tx = ("start", x + 12) if side == "L" else ("end", x - 12)
            s.text(tx, y + 4, n, 11, anchor, "bold" if col else "normal", fill, mono=True)


def legend(s, y, items=None):
    items = items or [("+5V", C_5V), ("GND", C_GND), ("TRIG", C_TRIG), ("ECHO", C_ECHO), ("LED data", C_DIN)]
    x = 40
    for label, col in items:
        s.add(f"<line x1='{x}' y1='{y}' x2='{x+28}' y2='{y}' stroke='{col}' stroke-width='3'/>")
        s.text(x + 34, y + 4, label, 12)
        x += 110
    s.text(40, y + 26, "Symbols with the same name (+5V / ground) are connected together. "
           "Pin order can differ on clone boards: always check the silkscreen.", 11, fill="#555")
    s.text(40, y + 50, "woodyouloveit.com", 12, weight="bold", fill="#e7004e")
    s.text(192, y + 50, "\u00a9 2026 Chanchal Sakarde. All Rights Reserved. Open source under GPL-3.0.", 11, fill="#333")


def sensor_block(s, x, y, echo_y, trig_y):
    """Sensor driver board. Power pins on top/bottom edges, signals on left edge."""
    w, h = 200, (max(echo_y, trig_y) - y) + 40
    s.box(x, y, w, h, "JSN-SR04T", "sensor driver board")
    for py, lab in ((echo_y, "ECHO"), (trig_y, "TRIG")):
        s.pin_label(x + 8, py, lab)
    # 5V top, GND bottom
    s.wire([(x + 60, y), (x + 60, y - 18)], C_5V); s.v5(x + 60, y - 18)
    s.text(x + 60, y + 16, "5V", 11, "middle", "bold", "#333", mono=True)
    s.wire([(x + 160, y + h), (x + 160, y + h + 14)], C_GND); s.gnd(x + 160, y + h + 14)
    s.text(x + 160, y + h - 8, "GND", 11, "middle", "bold", "#333", mono=True)
    # probe cable
    s.wire([(x + w, y + h / 2), (x + w + 40, y + h / 2)], "#555", 3)
    s.add(f"<circle cx='{x+w+62}' cy='{y+h/2}' r='22' fill='#212121' stroke='#555' stroke-width='2'/>")
    s.add(f"<circle cx='{x+w+62}' cy='{y+h/2}' r='12' fill='#616161'/>")
    s.text(x + w + 62, y + h / 2 + 40, "probe", 11, "middle", fill="#555")
    s.text(x + w + 62, y + h / 2 + 54, "(faces water)", 10, "middle", fill="#777")


def led_block(s, x, y, din_y):
    w, h = 200, 80
    s.box(x, y, w, h, "WS2812B strip", "1–300 LEDs, DIN end", fill="#eef7ee", stroke="#2e7d32")
    s.pin_label(x + 8, din_y, "DIN")
    for i in range(6):
        s.add(f"<rect x='{x+w+10+i*18}' y='{y+h/2-7}' width='14' height='14' rx='2' "
              f"fill='{['#e53935','#f4511e','#fb8c00','#fdd835','#7cb342','#43a047'][i]}'/>")
    s.text(x + w + 62, y + h / 2 + 26, "strip →", 11, "middle", fill="#555")
    s.wire([(x + 60, y), (x + 60, y - 18)], C_5V); s.v5(x + 60, y - 18)
    s.text(x + 60, y + 16, "5V", 11, "middle", "bold", "#333", mono=True)
    s.wire([(x + 160, y + h), (x + 160, y + h + 14)], C_GND); s.gnd(x + 160, y + h + 14)
    s.text(x + 160, y + h - 8, "GND", 11, "middle", "bold", "#333", mono=True)


def power_block(s, x, y):
    w, h = 170, 70
    s.box(x - 20, y, w, h, "5V 2A supply", "powers everything", fill="#fdecea", stroke="#c62828")
    s.wire([(x + 20, y), (x + 20, y - 18)], C_5V); s.v5(x + 20, y - 18)
    s.text(x + 20, y + 16, "+", 13, "middle", "bold", "#c62828")
    s.wire([(x + 20, y + h), (x + 20, y + h + 14)], C_GND); s.gnd(x + 20, y + h + 14)
    s.text(x + 20, y + h - 6, "−", 13, "middle", "bold", "#c62828")
    # capacitor
    cx = x + 220
    s.wire([(cx, y - 4), (cx, y + 26)], C_5V); s.v5(cx, y - 4)
    s.add(f"<line x1='{cx-14}' y1='{y+26}' x2='{cx+14}' y2='{y+26}' stroke='#333' stroke-width='3'/>")
    s.add(f"<path d='M{cx-14},{y+40} Q{cx},{y+32} {cx+14},{y+40}' fill='none' stroke='#333' stroke-width='3'/>")
    s.wire([(cx, y + 36), (cx, y + 70)], C_GND); s.gnd(cx, y + 70)
    s.text(cx + 20, y + 30, "1000µF", 12, "start", "bold", "#5d4037")
    s.text(cx + 20, y + 45, "≥10V", 11, "start", fill="#5d4037")
    s.text(cx - 20, y + 24, "+", 12, "end", "bold", "#333")


def board_power(s, vin_i, gnd_i):
    y = pin_y(vin_i)
    s.wire([(BX0, y), (BX0 - 50, y), (BX0 - 50, y - 16)], C_5V); s.v5(BX0 - 50, y - 16)
    y = pin_y(gnd_i)
    s.wire([(BX1, y), (BX1 + 40, y), (BX1 + 40, y + 12)], C_GND); s.gnd(BX1 + 40, y + 12)


def esp32():
    s = Svg(1000, 636)
    s.text(40, 40, "Water level indicator: ESP32 DevKit V1 (30-pin)", 20, weight="bold")
    s.text(40, 62, "Sensor on 3V3 · GND · D15 · D2 (one 4-pin connector) · LED strip G16", 13, fill="#555")
    left = ["EN", "VP 36", "VN 39", "D34", "D35", "D32", "D33", "D25", "D26", "D27",
            "D14", "D12", "D13", "GND", "VIN"]
    right = ["D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "TX2 17", "RX2 16",
             "D4", "D2", "D15", "GND", "3V3"]
    used = {"L14": C_5V, "R9": C_DIN, "R11": C_ECHO, "R12": C_TRIG, "R13": C_GND, "R14": C_3V3}
    draw_board(s, "ESP32", "DevKit V1 · 30-pin", left, right, used)
    # VIN from the 5V supply
    y = pin_y(14)
    s.wire([(BX0, y), (BX0 - 50, y), (BX0 - 50, y - 16)], C_5V); s.v5(BX0 - 50, y - 16)

    # Sensor: four straight wires from the four neighbouring pins
    sx, w = 640, 200
    ys = {"echo": pin_y(11), "trig": pin_y(12), "gnd": pin_y(13), "vcc": pin_y(14)}
    top, bot = ys["echo"] - 30, ys["vcc"] + 24
    s.box(sx, top, w, bot - top, "AJ-SR04M", "SR04M-2 board", fill="#f3e5f5", stroke="#6a1b9a")
    for key, lab, col in (("echo", "TX = ECHO", C_ECHO), ("trig", "RX = TRIG", C_TRIG),
                          ("gnd", "GND", C_GND), ("vcc", "5V pin", C_3V3)):
        s.wire([(BX1, ys[key]), (sx, ys[key])], col)
        s.pin_label(sx + 8, ys[key], lab)
    s.text(470, ys["vcc"] - 7, "3.3V", 12, "middle", "bold", C_3V3)
    s.wire([(sx + 160, bot), (sx + 160, bot + 12)], C_GND); s.gnd(sx + 160, bot + 12)   # common ground
    s.wire([(sx + w, (top + bot) / 2), (sx + w + 40, (top + bot) / 2)], "#555", 3)
    s.add(f"<circle cx='{sx+w+62}' cy='{(top+bot)/2}' r='22' fill='#212121' stroke='#555' stroke-width='2'/>")
    s.add(f"<circle cx='{sx+w+62}' cy='{(top+bot)/2}' r='12' fill='#616161'/>")
    s.text(sx + w + 62, (top + bot) / 2 + 40, "probe", 11, "middle", fill="#555")
    # Note about strapping pins
    s.add("<rect x='372' y='500' width='262' height='56' rx='6' fill='#fff3e0' stroke='#e65100' stroke-width='1.5'/>")
    s.text(503, 519, "Sensor 5V pin → ESP32 3V3 only", 12, "middle", "bold", "#e65100")
    s.text(503, 535, "D15 = TRIG and D2 = ECHO, not swapped:", 11, "middle", fill="#e65100")
    s.text(503, 549, "D2 must be LOW while uploading", 11, "middle", fill="#e65100")

    # LED strip: G16 -> 330R -> DIN
    g16 = pin_y(9)
    din = 150
    led_block(s, sx, din - 36, din)
    s.wire([(BX1, g16), (470, g16), (470, din), (560, din)], C_DIN)
    s.resistor_h(560, sx, din, C_DIN, "330Ω")
    power_block(s, sx + 40, 250)
    legend(s, 570, [("+5V", C_5V), ("3.3V", C_3V3), ("GND", C_GND), ("TRIG", C_TRIG),
                    ("ECHO", C_ECHO), ("LED data", C_DIN)])
    return s.out()


def esp8266():
    s = Svg(1000, 636)
    s.text(40, 40, "Water level indicator: NodeMCU ESP8266 (30-pin)", 20, weight="bold")
    s.text(40, 62, "Sketch pins: LED D2 (GPIO4), TRIG D5 (GPIO14), ECHO D6 (GPIO12)", 13, fill="#555")
    left = ["A0", "RSV", "RSV", "SD3", "SD2", "SD1", "CMD", "SD0", "CLK", "GND",
            "3V3", "EN", "RST", "GND", "VIN"]
    right = ["D0", "D1", "D2", "D3", "D4", "3V3", "GND", "D5", "D6", "D7",
             "D8", "RX", "TX", "GND", "3V3"]
    used = {"L14": C_5V, "R13": C_GND, "R2": C_DIN, "R7": C_TRIG, "R8": C_ECHO}
    draw_board(s, "NodeMCU", "ESP8266 · 30-pin", left, right, used)
    board_power(s, 14, 13)

    sx = 640
    # LED on top: D2 -> 330R -> DIN
    d2 = pin_y(2)
    led_block(s, sx, d2 - 36, d2)
    s.wire([(BX1, d2), (560, d2)], C_DIN)
    s.resistor_h(560, sx, d2, C_DIN, "330Ω")
    # Sensor below: TRIG D5, ECHO D6 (straight across)
    trig_y, echo_y = pin_y(7), pin_y(8)
    sensor_block(s, sx, trig_y - 40, echo_y, trig_y)
    s.wire([(BX1, trig_y), (sx, trig_y)], C_TRIG)
    s.wire([(BX1, echo_y), (560, echo_y)], C_ECHO)
    s.resistor_h(560, sx, echo_y, C_ECHO, "1kΩ")
    s.dot(500, echo_y, C_ECHO)
    s.resistor_v(500, echo_y, echo_y + 80, C_ECHO, "2kΩ"); s.gnd(500, echo_y + 80)
    power_block(s, sx + 40, 470)
    legend(s, 570)
    return s.out()


def esp8266_3v3():
    """NodeMCU with AJ-SR04M powered from 3.3V: ECHO wired directly, no divider."""
    s = Svg(1000, 636)
    s.text(40, 40, "NodeMCU ESP8266 + AJ-SR04M on 3.3V (no divider)", 20, weight="bold")
    s.text(40, 62, "LED D2 (GPIO4)  ·  sensor RX = TRIG from D5 (GPIO14)  ·  sensor TX = ECHO to D6 (GPIO12)",
           13, fill="#555")
    left = ["A0", "RSV", "RSV", "SD3", "SD2", "SD1", "CMD", "SD0", "CLK", "GND",
            "3V3", "EN", "RST", "GND", "VIN"]
    right = ["D0", "D1", "D2 GPIO4", "D3", "D4", "3V3", "GND", "D5 GPIO14", "D6 GPIO12", "D7",
             "D8", "RX", "TX", "GND", "3V3"]
    used = {"L14": C_5V, "R13": C_GND, "R2": C_DIN, "R7": C_TRIG, "R8": C_ECHO, "R5": C_3V3}
    draw_board(s, "NodeMCU", "ESP8266 · 30-pin", left, right, used)
    board_power(s, 14, 13)

    sx = 640
    # LED strip on top: D2 -> 330R -> DIN
    d2 = pin_y(2)
    led_block(s, sx, d2 - 36, d2)
    s.wire([(BX1, d2), (560, d2)], C_DIN)
    s.resistor_h(560, sx, d2, C_DIN, "330Ω")

    # Sensor box
    trig_y, echo_y = pin_y(7), pin_y(8)
    x, y, w = sx, trig_y - 40, 200
    h = (echo_y - y) + 40
    s.box(x, y, w, h, "AJ-SR04M", "SR04M-2 board", fill="#f3e5f5", stroke="#6a1b9a")
    s.pin_label(x + 8, trig_y, "RX=TRIG")
    s.pin_label(x + 8, echo_y, "TX=ECHO")
    s.text(x + 60, y + 16, "5V pin", 11, "middle", "bold", "#333", mono=True)
    s.text(x + 160, y + h - 8, "GND", 11, "middle", "bold", "#333", mono=True)
    s.wire([(x + 160, y + h), (x + 160, y + h + 14)], C_GND); s.gnd(x + 160, y + h + 14)
    s.wire([(x + w, y + h / 2), (x + w + 40, y + h / 2)], "#555", 3)
    s.add(f"<circle cx='{x+w+62}' cy='{y+h/2}' r='22' fill='#212121' stroke='#555' stroke-width='2'/>")
    s.add(f"<circle cx='{x+w+62}' cy='{y+h/2}' r='12' fill='#616161'/>")
    s.text(x + w + 62, y + h / 2 + 40, "probe", 11, "middle", fill="#555")
    s.text(x + w + 62, y + h / 2 + 54, "(faces water)", 10, "middle", fill="#777")

    # 3V3 pin -> sensor "5V" pin (explicit wire)
    v3 = pin_y(5)
    s.wire([(BX1, v3), (x + 60, v3), (x + 60, y)], C_3V3)
    s.text(470, v3 - 8, "3.3V", 12, "middle", "bold", C_3V3)

    # Signals straight across, no divider
    s.wire([(BX1, trig_y), (sx, trig_y)], C_TRIG)
    s.wire([(BX1, echo_y), (sx, echo_y)], C_ECHO)

    # Warning note
    s.add("<rect x='380' y='372' width='250' height='62' rx='6' fill='#fff3e0' stroke='#e65100' stroke-width='1.5'/>")
    s.text(505, 394, "Sensor 5V pin → NodeMCU 3V3 only", 12, "middle", "bold", "#e65100")
    s.text(505, 412, "Never connect it to the 5V supply:", 11, "middle", fill="#e65100")
    s.text(505, 427, "TX would send 5V straight into D6.", 11, "middle", fill="#e65100")

    power_block(s, sx + 40, 470)
    legend(s, 570, [("+5V", C_5V), ("3.3V", C_3V3), ("GND", C_GND), ("TRIG", C_TRIG),
                    ("ECHO", C_ECHO), ("LED data", C_DIN)])
    return s.out()



# ---------------------------------------------------------------- firmware 3.0.0: several sensors

def v33(s, x, y):
    """3.3V flag with its bottom at (x, y)."""
    s.add(f"<polygon points='{x-7},{y} {x+7},{y} {x},{y-9}' fill='{C_3V3}'/>")
    s.text(x, y - 13, "3V3", 12, "middle", "bold", C_3V3)


def small_sensor(s, x, y, w, h, title, pins, side="right", probe=None):
    """Compact sensor board: pins [(y, label)] on the given edge; probe = (cx, cy) or None."""
    s.add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='7' fill='#f3e5f5' stroke='#6a1b9a' stroke-width='1.6'/>")
    tx = x + 8 if side == "right" else x + w - 8
    anchor = "start" if side == "right" else "end"
    s.text(tx, y + 16, title, 12, anchor, "bold", "#6a1b9a")
    s.text(tx, y + h - 8, "VCC→3V3 · GND", 10, anchor, fill="#555")
    for py, lab in pins:
        if side == "right": s.text(x + w - 6, py + 4, lab, 10, "end", "bold", "#333", mono=True)
        else:               s.text(x + 6, py + 4, lab, 10, "start", "bold", "#333", mono=True)
    if probe:
        cx, cy = probe
        s.add(f"<circle cx='{cx}' cy='{cy}' r='10' fill='#212121' stroke='#555' stroke-width='1.5'/>")



def vstrip(s, x, top, bot, label, din_bottom=True, n=8):
    """A vertical WS2812B strip (30 px wide) with n LEDs, DIN/DOUT labels, +5V and GND stubs on its right."""
    s.add(f"<rect x='{x}' y='{top}' width='30' height='{bot-top}' rx='5' fill='#edf7ee' stroke='#2e7d32' stroke-width='1.6'/>")
    cols = ["#e53935", "#f4511e", "#fb8c00", "#fdd835", "#c0ca33", "#7cb342", "#43a047", "#2e7d32"]
    step = (bot - top - 16) / n
    for i in range(n):                                    # the level always fills from the bottom (red) up (green)
        yy = bot - 8 - (i + 1) * step
        s.add(f"<rect x='{x+7}' y='{yy+2:.1f}' width='16' height='{step-4:.1f}' rx='2' fill='{cols[i*len(cols)//n]}'/>")
    s.text(x + 15, (bot + 13) if din_bottom else (top - 5), "DIN", 9, "middle", "bold", C_DIN, mono=True)
    s.text(x + 15, (top - 5) if din_bottom else (bot + 13), "DOUT", 9, "middle", "bold", "#555", mono=True)
    yv, yg = top + 26, bot - 26
    s.wire([(x + 30, yv), (x + 46, yv)], C_5V); s.text(x + 49, yv + 4, "+5V", 10, "start", "bold", C_5V)
    s.wire([(x + 30, yg), (x + 42, yg), (x + 42, yg + 6)], C_GND); s.gnd(x + 42, yg + 6)
    first, second = label.split(" · ")
    s.text(x + 15, bot + 32, first, 11, "middle", "bold", "#2e7d32")
    s.text(x + 15, bot + 46, second, 10, "middle", fill="#2e7d32")


def strip_chain(s, x0, top, bot, gap, count, din_x0, din_y):
    """count strips chained zig-zag, fed from a data wire that arrives at (din_x0, din_y) below strip 1."""
    xs = [x0 + i * gap for i in range(count)]
    for i, x in enumerate(xs):
        vstrip(s, x, top, bot, f"Strip {i+1} · sensor {i+1}", din_bottom=(i % 2 == 0))
    # data: G16/D2 -> 330R -> strip 1 DIN (bottom)
    s.wire([(din_x0, din_y), (xs[0] + 15, din_y), (xs[0] + 15, bot)], C_DIN)
    for i in range(count - 1):                            # DOUT -> next DIN, alternating top and bottom
        if i % 2 == 0:
            s.wire([(xs[i] + 15, top), (xs[i] + 15, top - 18), (xs[i + 1] + 15, top - 18), (xs[i + 1] + 15, top)], C_DIN)
        else:
            s.wire([(xs[i] + 15, bot), (xs[i] + 15, bot + 18), (xs[i + 1] + 15, bot + 18), (xs[i + 1] + 15, bot)], C_DIN)

def esp32_multi():
    """ESP32 with up to 4 sensors: sensor 1 on the right (as before), sensors 2-4 on the left."""
    global BX0, BX1
    keep = (BX0, BX1)
    BX0, BX1 = 470, 670
    try:
        s = Svg(1600, 690)
        s.text(40, 40, "ESP32 DevKit V1: up to 4 ultrasonic sensors (one per tank compartment)", 20, weight="bold")
        s.text(40, 62, "Sensor 1 D15/D2 · sensor 2 D32/D35 · sensor 3 D33/D34 · sensor 4 D25/D26 · one LED strip per compartment, "
                       "chained on G16 · every sensor powered from 3V3", 13, fill="#555")
        left = ["EN", "VP 36", "VN 39", "D34", "D35", "D32", "D33", "D25", "D26", "D27",
                "D14", "D12", "D13", "GND", "VIN"]
        right = ["D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "TX2 17", "RX2 16",
                 "D4", "D2", "D15", "GND", "3V3"]
        used = {"L14": C_5V, "L3": C_ECHO, "L4": C_ECHO, "L5": C_TRIG, "L6": C_TRIG, "L7": C_TRIG, "L8": C_ECHO,
                "R9": C_DIN, "R11": C_ECHO, "R12": C_TRIG, "R13": C_GND, "R14": C_3V3}
        draw_board(s, "ESP32", "DevKit V1 · 30-pin", left, right, used)
        y = pin_y(14)
        s.wire([(BX0, y), (BX0 - 40, y), (BX0 - 40, y - 16)], C_5V); s.v5(BX0 - 40, y - 16)

        # ---- sensors 2-4 on the left: every signal wire is straight ----
        # sensor 2: ECHO D35 (L4), TRIG D32 (L5)
        y2e, y2t = pin_y(4), pin_y(5)
        small_sensor(s, 300, 206, 140, 62, "Sensor 2", [(y2e, "TX=ECHO"), (y2t, "RX=TRIG")], probe=(282, 237))
        s.wire([(440, y2e), (BX0, y2e)], C_ECHO); s.wire([(440, y2t), (BX0, y2t)], C_TRIG)
        # sensor 3: ECHO D34 (L3), TRIG D33 (L6), wires pass above and below sensor 2
        y3e, y3t = pin_y(3), pin_y(6)
        small_sensor(s, 60, 180, 160, 116, "Sensor 3", [(y3e, "TX=ECHO"), (y3t, "RX=TRIG")], probe=(40, 238))
        s.wire([(220, y3e), (BX0, y3e)], C_ECHO); s.wire([(220, y3t), (BX0, y3t)], C_TRIG)
        # sensor 4: TRIG D25 (L7), ECHO D26 (L8)
        y4t, y4e = pin_y(7), pin_y(8)
        small_sensor(s, 300, 288, 140, 58, "Sensor 4", [(y4t, "RX=TRIG"), (y4e, "TX=ECHO")], probe=(282, 317))
        s.wire([(440, y4t), (BX0, y4t)], C_TRIG); s.wire([(440, y4e), (BX0, y4e)], C_ECHO)
        # power note for the optional sensors
        s.add("<rect x='40' y='372' width='366' height='92' rx='6' fill='#f3e5f5' stroke='#6a1b9a' stroke-width='1.5'/>")
        s.text(223, 394, "Sensors 2-4: power and ground", 13, "middle", "bold", "#6a1b9a")
        s.text(223, 414, "Each sensor's \"5V\" pin → ESP32 3V3, each GND → GND.", 11, "middle", fill="#333")
        s.text(223, 430, "The board has one 3V3 pin: join the wires on a", 11, "middle", fill="#333")
        s.text(223, 446, "terminal strip. Wire only the sensors you use.", 11, "middle", fill="#333")

        # ---- sensor 1 on the right: four straight wires (unchanged wiring) ----
        sx, w = 970, 200
        ys = {"echo": pin_y(11), "trig": pin_y(12), "gnd": pin_y(13), "vcc": pin_y(14)}
        top, bot = ys["echo"] - 30, ys["vcc"] + 24
        s.box(sx, top, w, bot - top, "Sensor 1", "AJ-SR04M / JSN", fill="#f3e5f5", stroke="#6a1b9a")
        for key, lab, col in (("echo", "TX = ECHO", C_ECHO), ("trig", "RX = TRIG", C_TRIG),
                              ("gnd", "GND", C_GND), ("vcc", "5V pin", C_3V3)):
            s.wire([(BX1, ys[key]), (sx, ys[key])], col)
            s.pin_label(sx + 8, ys[key], lab)
        s.text(800, ys["vcc"] - 7, "3.3V (to every sensor)", 12, "middle", "bold", C_3V3)
        s.wire([(sx + 160, bot), (sx + 160, bot + 12)], C_GND); s.gnd(sx + 160, bot + 12)
        s.wire([(sx + w, (top + bot) / 2), (sx + w + 40, (top + bot) / 2)], "#555", 3)
        s.add(f"<circle cx='{sx+w+62}' cy='{(top+bot)/2}' r='22' fill='#212121' stroke='#555' stroke-width='2'/>")
        s.add(f"<circle cx='{sx+w+62}' cy='{(top+bot)/2}' r='12' fill='#616161'/>")
        s.add("<rect x='702' y='528' width='262' height='56' rx='6' fill='#fff3e0' stroke='#e65100' stroke-width='1.5'/>")
        s.text(833, 547, "Every sensor's 5V pin → 3V3 only", 12, "middle", "bold", "#e65100")
        s.text(833, 563, "Sensor 1: D15 = TRIG, D2 = ECHO, not swapped:", 11, "middle", fill="#e65100")
        s.text(833, 577, "D2 must be LOW while uploading", 11, "middle", fill="#e65100")

        # ---- LED strips: one per compartment, chained zig-zag on G16 ----
        g16, top, bot = pin_y(9), 104, 288
        s.wire([(BX1, g16), (800, g16), (800, 306)], C_DIN)
        s.resistor_h(800, 880, 306, C_DIN, "330Ω")
        strip_chain(s, 960, top, bot, 110, 3, 880, 306)
        s.add("<rect x='1290' y='96' width='214' height='108' rx='6' fill='#edf7ee' stroke='#2e7d32' stroke-width='1.5'/>")
        s.text(1397, 116, "Chained LED strips", 12, "middle", "bold", "#2e7d32")
        for k, line in enumerate(["Zig-zag: strip 1 up, strip 2 down,", "strip 3 up: each DOUT beside the next DIN.",
                                  "5V and GND to every strip.", "Setup: LED strip shows → One strip per", "compartment · LEDs per strip · Zig-zag"]):
            s.text(1397, 134 + k * 15, line, 10, "middle", fill="#333")
        power_block(s, 1310, 254)
        legend(s, 620, [("+5V", C_5V), ("3.3V", C_3V3), ("GND", C_GND), ("TRIG", C_TRIG),
                        ("ECHO", C_ECHO), ("LED data", C_DIN)])
        return s.out()
    finally:
        BX0, BX1 = keep


def esp8266_multi():
    """NodeMCU with 2 sensors (sensor 2 on D7/D1, shown with net labels)."""
    s = Svg(1060, 760)
    s.text(40, 40, "NodeMCU ESP8266: 2 sensors (one per compartment)", 20, weight="bold")
    s.text(40, 62, "Sensor 1 TRIG D5 / ECHO D6 · sensor 2 TRIG D7 / ECHO D1 · two LED strips chained on D2 · sensors on 3V3",
           13, fill="#555")
    left = ["A0", "RSV", "RSV", "SD3", "SD2", "SD1", "CMD", "SD0", "CLK", "GND",
            "3V3", "EN", "RST", "GND", "VIN"]
    right = ["D0", "D1 GPIO5", "D2 GPIO4", "D3", "D4", "3V3", "GND", "D5 GPIO14", "D6 GPIO12", "D7 GPIO13",
             "D8", "RX", "TX", "GND", "3V3"]
    used = {"L14": C_5V, "R13": C_GND, "R2": C_DIN, "R7": C_TRIG, "R8": C_ECHO, "R5": C_3V3,
            "R1": C_ECHO, "R9": C_TRIG}
    draw_board(s, "NodeMCU", "ESP8266 · 30-pin", left, right, used)
    board_power(s, 14, 13)

    sx = 640
    d2 = pin_y(2)
    s.wire([(BX1, d2), (520, d2)], C_DIN)
    s.resistor_h(520, 580, d2, C_DIN, "330Ω")
    s.wire([(580, d2), (600, d2), (600, 204)], C_DIN)
    strip_chain(s, 640, 98, 188, 100, 2, 600, 204)
    s.add("<rect x='830' y='96' width='200' height='80' rx='6' fill='#edf7ee' stroke='#2e7d32' stroke-width='1.5'/>")
    s.text(930, 115, "Chained LED strips", 12, "middle", "bold", "#2e7d32")
    for k, line in enumerate(["Zig-zag: strip 1 up, strip 2 down.", "5V and GND to both strips.", "Setup: One strip per compartment"]):
        s.text(930, 133 + k * 15, line, 10, "middle", fill="#333")

    # Sensor 1 (as before)
    trig_y, echo_y = pin_y(7), pin_y(8)
    x, y, w = sx, trig_y - 40, 200
    h = (echo_y - y) + 40
    s.box(x, y, w, h, "Sensor 1", "AJ-SR04M / JSN", fill="#f3e5f5", stroke="#6a1b9a")
    s.pin_label(x + 8, trig_y, "RX=TRIG"); s.pin_label(x + 8, echo_y, "TX=ECHO")
    s.text(x + 60, y + 16, "5V pin", 11, "middle", "bold", "#333", mono=True)
    s.text(x + 160, y + h - 8, "GND", 11, "middle", "bold", "#333", mono=True)
    s.wire([(x + 160, y + h), (x + 160, y + h + 14)], C_GND); s.gnd(x + 160, y + h + 14)
    v3 = pin_y(5)
    s.wire([(BX1, v3), (x + 60, v3), (x + 60, y)], C_3V3)
    s.text(470, v3 - 8, "3.3V", 12, "middle", "bold", C_3V3)
    s.wire([(BX1, trig_y), (sx, trig_y)], C_TRIG)
    s.wire([(BX1, echo_y), (sx, echo_y)], C_ECHO)

    # Sensor 2 through net labels: D1 -> "S2 ECHO", D7 -> "S2 TRIG"
    def tag(xx, yy, text, col):
        s.add(f"<rect x='{xx}' y='{yy-10}' width='74' height='20' rx='4' fill='#ffffff' stroke='{col}' stroke-width='1.8'/>")
        s.text(xx + 37, yy + 4, text, 11, "middle", "bold", col, mono=True)
    d1, d7 = pin_y(1), pin_y(9)
    s.wire([(BX1, d1), (390, d1)], C_ECHO); tag(390, d1, "S2 ECHO", C_ECHO)
    s.wire([(BX1, d7), (390, d7)], C_TRIG); tag(390, d7, "S2 TRIG", C_TRIG)
    bx, by = 640, 560
    s.box(bx, by, 200, 96, "Sensor 2", "AJ-SR04M / JSN", fill="#f3e5f5", stroke="#6a1b9a")
    for yy, lab, text, col in ((by + 30, "RX=TRIG", "S2 TRIG", C_TRIG), (by + 66, "TX=ECHO", "S2 ECHO", C_ECHO)):
        s.wire([(bx - 40, yy), (bx, yy)], col); tag(bx - 114, yy, text, col); s.pin_label(bx + 8, yy, lab)
    s.wire([(bx + 60, by), (bx + 60, by - 16)], C_3V3); v33(s, bx + 60, by - 16)
    s.text(bx + 60, by + 14, "5V pin", 10, "middle", "bold", "#333", mono=True)
    s.wire([(bx + 160, by + 96), (bx + 160, by + 108)], C_GND); s.gnd(bx + 160, by + 108)
    s.text(bx + 160, by + 88, "GND", 10, "middle", "bold", "#333", mono=True)
    s.add(f"<circle cx='{bx+232}' cy='{by+48}' r='18' fill='#212121' stroke='#555' stroke-width='2'/>")
    s.text(380, 520, "Same-named tags are connected: D7 → sensor 2 RX, D1 ← sensor 2 TX", 11, "start", fill="#555")

    s.add("<rect x='40' y='560' width='260' height='76' rx='6' fill='#fff3e0' stroke='#e65100' stroke-width='1.5'/>")
    s.text(170, 580, "Both sensors' 5V pin → 3V3 only", 12, "middle", "bold", "#e65100")
    s.text(170, 598, "Never the 5V supply: TX would put", 11, "middle", fill="#e65100")
    s.text(170, 613, "5V into D6 / D1. NodeMCU: 2 sensors max.", 11, "middle", fill="#e65100")
    power_block(s, sx + 40, 410)
    legend(s, 690, [("+5V", C_5V), ("3.3V", C_3V3), ("GND", C_GND), ("TRIG", C_TRIG),
                    ("ECHO", C_ECHO), ("LED data", C_DIN)])
    return s.out()

if __name__ == "__main__":
    import os, cairosvg
    out = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    for name, svg in (("wiring_esp32", esp32()), ("wiring_esp8266", esp8266()), ("wiring_esp8266_3v3", esp8266_3v3()),
                      ("wiring_esp32_multi", esp32_multi()), ("wiring_esp8266_multi", esp8266_multi())):
        with open(f"{out}/{name}.svg", "w") as f:
            f.write(svg)
        cairosvg.svg2png(bytestring=svg.encode(), write_to=f"{out}/{name}.png")
    print("ok")
