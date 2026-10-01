The enclosure is ready: two STL files to print, plus the OpenSCAD source for adjusting sizes.

**Size:** 130 × 51 × 36 mm for the box. With the mounting ears it's 154 mm long, and the lid adds 2.4 mm on top.

**What's in the design:**

- **Probe passage** in the right-hand wall, next to the JSN board.
- **Power passage** in the back wall. I also added a **third passage for the LED strip's 3 wires**, since they have to leave the box too; it can be switched off with one setting.
- All three passages are **slots open at the top**. You lay the cable in, connector and all; the probe's white plug wouldn't fit through a small round hole. The lid has tongues that close each slot and lightly clamp the cable.
- **5V fan:** a 30 × 30 × 10 mm fan (24 mm hole spacing) screws to the underside of the lid and blows out through a grille. Air comes in through slots low in the front wall. 25 mm and 40 mm fans are a one-number change, or you can turn the fan off and get plain lid vents.
- **Holes over the BOOT and RST buttons,** labelled, so you can press them with a pen. BOOT is needed for the on-demand hotspot and the password reset.
- **Micro-USB opening** for programming, and **wall-mounting ears**.
- **Corner cradles** hold both boards 15 mm above the floor, leaving room for the header pins and jumper wires underneath.
- **The lid is engraved** with "Water Tanks Monitor" and "woodyouloveit.com".

**Printing:**

- Print the box with the open side up, and the lid top face down. Neither needs supports.
- Use PETG or ASA if it will sit near heat or sun; PLA is fine indoors.
- Settings: 0.2 mm layers, 3 walls, 20% infill.

**Hardware:**

- 4 × M3 × 8–10 mm screws for the lid.
- 4 × M3 × 8 mm screws for the fan.
- 2 wall screws for the ears.

Connect the fan to **5V and GND** (VIN and GND), never to 3V3.

**Before printing, measure three things with calipers.** If any differ from these defaults, change them at the top of the `.scad` file and export again from OpenSCAD (free):

1. **ESP32 board size:** set as 51.5 × 28.5 mm; clone boards vary slightly. The JSN board uses your spec sheet's 41.3 × 29 mm.
2. **Probe cable diameter:** set as 5.0 mm.
3. **Power cable diameter:** set as 5.5 mm.

**Note:** with a fan and vents, the box is **not splash-proof**. Mount it under cover, out of rain and direct sun, with the passages facing down or sideways.



Try Creating Box : [ChanchalSakardeQH/3d-Box-STL-FILE-Generator: Parametric 3D box generator with dividers, lids, and text engraving — exports STL files for 3D printing, entirely in the browser.](https://github.com/ChanchalSakardeQH/3d-Box-STL-FILE-Generator)