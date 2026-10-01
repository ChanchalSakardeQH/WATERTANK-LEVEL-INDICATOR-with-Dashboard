/*
  Water Tanks Monitor System - 3D printed enclosure
  ESP32 DevKit V1 (30-pin) + JSN-SR04M-2.0 / AJ-SR04M ultrasonic board + optional 5V fan
  woodyouloveit.com
  Copyright (C) 2026 Chanchal Sakarde. All Rights Reserved, except as granted by the license below.
  This design is free: you can redistribute it and/or modify it under the terms of the GNU General
  Public License, version 3 or (at your option) any later version. No warranty.
  SPDX-License-Identifier: GPL-3.0-or-later

  How to use
  ----------
  1. Measure your boards with calipers and adjust the "Boards" values below if they differ.
  2. Set part = "box", "lid" or "assembly" (preview with boards), then F6 (Render) and F7 (Export STL).
  3. Print the box open side up and the lid top face down. No supports needed.

  Cable passages are slots open at the top of the wall: lay the cable in (connector and all),
  then the lid's tongue closes the slot and clamps the cable.
*/

part = "assembly";            // "box", "lid", "assembly"

// ---------------- Boards (mm) - measure yours ----------------
esp_l = 51.5;                 // ESP32 DevKit V1 30-pin PCB length (USB end to antenna end)
esp_w = 28.5;                 // PCB width
esp_top_h = 4.0;              // tallest part on top of the ESP32 (module shield / USB)
jsn_l = 41.3;                 // JSN-SR04M-2.0 PCB length (spec sheet)
jsn_w = 29.0;                 // PCB width (spec sheet)
jsn_top_h = 12.0;             // tallest part on the JSN board (transformer, capacitor)
pcb_t = 1.6;                  // PCB thickness
standoff_h = 15;              // space under the boards for header pins + Dupont connectors

// ---------------- Cables (mm) - measure yours ----------------
probe_cable_d = 5.0;          // probe cable outer diameter
power_cable_d = 5.5;          // 5 V supply cable (2-core or USB cable)
led_cable_d   = 4.5;          // LED strip 3-wire cable
led_passage   = true;         // third passage for the LED strip wires
usb_window    = true;         // opening for the ESP32 micro-USB (programming / USB power)

// ---------------- Fan ----------------
fan = true;                   // 5 V fan under the lid, blowing out
fan_size = 30;                // 30 (3010 fan, 24 mm holes) or 25 (2507 fan, 20 mm holes) or 40 (4010, 32 mm)
fan_thick = 10;
fan_hole_spacing = fan_size == 25 ? 20 : fan_size == 40 ? 32 : 24;

// ---------------- Enclosure ----------------
wall = 2.4;
floor_t = 2.4;
lid_t = 2.4;
clear = 0.6;                  // clearance around boards
bay = 28;                     // wiring bay between the boards (capacitor, terminal blocks)
inner_w = 46;
inner_h = max(standoff_h + pcb_t + jsn_top_h, standoff_h + pcb_t + esp_top_h + (fan ? fan_thick : 0)) + 3;
inner_l = 1.5 + esp_l + bay + jsn_l + 3;
ears = true;                  // wall-mounting ears
brand = true;                 // engraved text on the lid
$fn = 48;

L = inner_l + 2*wall;
W = inner_w + 2*wall;
H = floor_t + inner_h;

// Board positions (inner coordinates: x from the left inner wall, y from the front inner wall)
esp_x = 1.5;                  // USB end close to the left wall
esp_y = (inner_w - esp_w) / 2;
jsn_x = esp_x + esp_l + bay;
jsn_y = (inner_w - jsn_w) / 2;
board_z = floor_t + standoff_h;

// Cable slot positions
slot_drop = 8;                // cable centre below the top of the wall
probe_slot = [L - wall/2, wall + inner_w/2];               // right wall
power_slot = [wall + esp_x + esp_l + bay*0.35, W - wall/2]; // back wall, wiring bay
led_slot   = [wall + esp_x + esp_l + bay*0.80, W - wall/2]; // back wall
fan_c = [wall + esp_x + esp_l + 4, wall + inner_w/2];       // fan centre (over ESP32 end / bay)
boot_c = [wall + esp_x + 3.5, wall + esp_y + 4.5];          // BOOT button (front side, USB end)
en_c   = [wall + esp_x + 3.5, wall + esp_y + esp_w - 4.5];  // EN (reset) button

// ================= helpers =================
module rbox(s, r=2) {          // rounded-corner box
  hull() for (x=[r, s[0]-r], y=[r, s[1]-r]) translate([x, y, 0]) cylinder(r=r, h=s[2]);
}

// U-slot open at the top of a wall, cable centre at slot_drop below the top
module cable_slot(pos, d, along_x) {
  w = d + 0.8;
  translate([pos[0], pos[1], H - slot_drop])
    rotate([0, 0, along_x ? 90 : 0]) {
      rotate([90, 0, 0]) cylinder(d=w, h=wall*3, center=true);
      translate([-w/2, -wall*1.5, 0]) cube([w, wall*3, slot_drop + 1]);
    }
}
// Lid tongue that closes the slot and clamps the cable lightly
module cable_tongue(pos, d, along_x) {
  w = d + 0.4;
  translate([pos[0], pos[1], 0])
    rotate([0, 0, along_x ? 90 : 0])
      difference() {
        translate([-w/2, -wall/2 + 0.2, -(slot_drop)]) cube([w, wall - 0.4, slot_drop]);
        translate([0, 0, -slot_drop]) rotate([90, 0, 0]) cylinder(d=d - 0.4, h=wall*2, center=true);
      }
}

// Corner cradle: holds a PCB by its corners at height standoff_h
module cradle(bx, by, bl, bw) {
  post = 6; inset = 2.5; above = 2.2;
  for (cx=[0, 1], cy=[0, 1]) {
    px = bx + (cx ? bl - inset : -(post - inset));
    py = by + (cy ? bw - inset : -(post - inset));
    difference() {
      translate([wall + px, wall + py, floor_t - 0.01]) cube([post, post, standoff_h + pcb_t + above]);
      translate([wall + bx - 0.25, wall + by - 0.25, board_z]) cube([bl + 0.5, bw + 0.5, 20]);
    }
  }
}

// ================= box =================
module box() {
  difference() {
    union() {
      rbox([L, W, H], 3);
      // wall-mounting ears (overlap the wall by 1 mm so they print as one piece)
      if (ears) for (side=[0, 1]) translate([side ? L - 1 : -12, W/2 - 8, 0]) difference() {
        cube([13, 16, 3]);
        translate([side ? 7 : 6, 8, -1]) cylinder(d=4.5, h=5);
      }
    }
    // hollow
    translate([wall, wall, floor_t]) rbox([inner_l, inner_w, H], 1.5);
    // cable passages
    cable_slot(probe_slot, probe_cable_d, true);
    cable_slot(power_slot, power_cable_d, false);
    if (led_passage) cable_slot(led_slot, led_cable_d, false);
    // micro-USB opening (left wall, centred on the ESP32)
    if (usb_window)
      translate([-1, wall + esp_y + esp_w/2 - 6.5, board_z + pcb_t + 1.3 - 4.5])
        cube([wall + 2, 13, 9]);
    // intake vents, low in the front wall
    for (i=[0:7]) translate([wall + esp_x + 18 + i*7.5, -1, floor_t + 4]) cube([2.6, wall + 2, 12]);
  }
  // lid screw bosses in the inner corners
  for (x=[wall + 3.5, L - wall - 3.5], y=[wall + 3.5, W - wall - 3.5])
    difference() {
      translate([x, y, floor_t - 0.01]) cylinder(d=7.5, h=inner_h - 0.01);
      translate([x, y, H - 14]) cylinder(d=2.7, h=15);   // M3 self-tapping, 12 mm
    }
  // board cradles
  cradle(esp_x, esp_y, esp_l, esp_w);
  cradle(jsn_x, jsn_y, jsn_l, jsn_w);
}

// ================= lid =================
// Modelled in place (z = 0 at the top of the walls); exported flipped for printing.
module lid() {
  lip = 3; lip_t = 1.6; gap = 0.35;
  difference() {
    union() {
      rbox([L, W, lid_t], 3);
      // locating lip inside the walls, cut back at the corner bosses
      difference() {
        translate([wall + gap, wall + gap, -lip]) cube([inner_l - 2*gap, inner_w - 2*gap, lip]);
        translate([wall + gap + lip_t, wall + gap + lip_t, -lip - 1]) cube([inner_l - 2*gap - 2*lip_t, inner_w - 2*gap - 2*lip_t, lip + 2]);
        for (x=[wall + 3.5, L - wall - 3.5], y=[wall + 3.5, W - wall - 3.5]) translate([x, y, -lip - 1]) cylinder(d=10, h=lip + 1.01);
        // leave room for the cable tongues
        for (p=[probe_slot, power_slot, led_slot]) translate([p[0], p[1], -lip/2]) cube([12, 12, lip + 2], center=true);
      }
      // tongues closing the cable slots
      cable_tongue(probe_slot, probe_cable_d, true);
      cable_tongue(power_slot, power_cable_d, false);
      if (led_passage) cable_tongue(led_slot, led_cable_d, false);
    }
    // screw holes (M3, countersunk from the top)
    for (x=[wall + 3.5, L - wall - 3.5], y=[wall + 3.5, W - wall - 3.5]) {
      translate([x, y, -1]) cylinder(d=3.4, h=lid_t + 2);
      translate([x, y, lid_t - 1.6]) cylinder(d1=3.4, d2=6.4, h=1.61);
    }
    // fan: grille + mounting holes, or plain vent slots without a fan
    if (fan) {
      translate([fan_c[0], fan_c[1], -1]) {
        for (r=[0:4:fan_size/2 - 3]) for (a=[0:(r == 0 ? 360 : 360/floor(r*1.4)):359])
          rotate([0, 0, a]) translate([r, 0, 0]) cylinder(d=2.8, h=lid_t + 2, $fn=16);
        for (dx=[-1, 1], dy=[-1, 1]) translate([dx*fan_hole_spacing/2, dy*fan_hole_spacing/2, 0]) cylinder(d=3.3, h=lid_t + 2);
      }
    } else {
      for (i=[0:5]) translate([fan_c[0] - 12 + i*5, fan_c[1] - 12, -1]) cube([2.5, 24, lid_t + 2]);
    }
    // access holes for the ESP32 buttons (press with a pen or toothpick)
    for (c=[boot_c, en_c]) translate([c[0], c[1], -1]) cylinder(d=5, h=lid_t + 2);
    // engraved labels on the top face
    translate([boot_c[0] + 4.5, boot_c[1], lid_t - 0.6]) linear_extrude(1) text("BOOT", size=3, font="DejaVu Sans:style=Bold", valign="center");
    translate([en_c[0] + 4.5, en_c[1], lid_t - 0.6]) linear_extrude(1) text("RST", size=3, font="DejaVu Sans:style=Bold", valign="center");
    // centred in the free space between the fan grille and the right-hand screws
    if (brand) translate([((fan ? fan_c[0] + fan_size/2 : wall + jsn_x) + L - 9) / 2, W/2, lid_t - 0.6]) linear_extrude(1) {
      translate([0, 5]) text("Water Tanks Monitor", size=2.9, font="DejaVu Sans:style=Bold", halign="center", valign="center");
      translate([0, -2]) text("woodyouloveit.com", size=2.9, font="DejaVu Sans:style=Bold", halign="center", valign="center");
      translate([0, -9]) text("probe ▸", size=2.8, font="DejaVu Sans", halign="center", valign="center");
    }
  }
}

// ================= preview models of the parts inside =================
module dummy_boards() {
  color("#1f2a33") translate([wall + esp_x, wall + esp_y, board_z]) cube([esp_l, esp_w, pcb_t]);
  color("#9aa5ad") translate([wall + esp_x + 25, wall + esp_y + 7, board_z + pcb_t]) cube([25, 18, 3.2]);
  color("#c0c0c0") translate([wall + esp_x - 1, wall + esp_y + esp_w/2 - 4, board_z + pcb_t]) cube([6, 8, 2.8]);
  color("#1d4fa3") translate([wall + jsn_x, wall + jsn_y, board_z]) cube([jsn_l, jsn_w, pcb_t]);
  color("#333") translate([wall + jsn_x + 4, wall + jsn_y + 4, board_z + pcb_t]) cube([11, 11, jsn_top_h]);
  color("#eee") translate([wall + jsn_x + jsn_l - 12, wall + jsn_y + 9, board_z + pcb_t]) cube([10, 11, 6]);
  if (fan) color("#222") translate([fan_c[0] - fan_size/2, fan_c[1] - fan_size/2, H - fan_thick]) difference() {
    cube([fan_size, fan_size, fan_thick]);
    translate([fan_size/2, fan_size/2, -1]) cylinder(d=fan_size - 3, h=fan_thick + 2);
  }
}

if (part == "box") box();
else if (part == "lid") translate([0, W, lid_t]) rotate([180, 0, 0]) lid();   // top face down
else {
  color("#e8eef2") box();
  dummy_boards();
  color("#0f766e", 0.55) translate([0, 0, H + 14]) lid();
}
