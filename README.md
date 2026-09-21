# Stick-Switch

Single-axis **forward** pencil-grip stick for two-level AAC / iPad Switch Control. Soft→hard is an **upgrade** (HID `1` then `2`), never a double-select. The APH Adaptable Stick Switch (`1-08615-00`) stays the binary classroom backup.

Student motion: thumb + index **push forward**. Not a 360° wobble.

## What to print first

Method 3 + APH on GPIO33 is the first classroom experiment. First-kit print list (part, material, and bed orientation):

| Part | Material | Notes |
| --- | --- | --- |
| `base_chassis` | PETG | Floor on the bed |
| `cover_slot` | PETG | Interior face on the bed |
| `rocker_arm` | PETG | Hub face on the bed; bearings are in the towers only |
| `stick_collar` | PETG | |
| `grip_sleeve` `bellows` `tpu_foot` ×4 | TPU 95A | |
| `p_clip` | PETG | Pad on the bed; TPU 95A optional for extra grip. Print a second for USB-bank cable. |
| `mod3_switch_plate` | PETG | ID resistor **10k** to GND |
| `mod3_ramp_cam` | PETG | Optional |
| `aa_2x_shim` | PETG | Only if using a 2x AA holder |

Exploded look: `openscad -D 'part="kit_preview"' cad/stick_switch.scad` (not an STL). Keep APH on GPIO33 the same day.

Later: Method 4b if travel is too small (chassis-mounted TAL220 saddle — it does **not** fit the 40×32 mm pocket). Method 2 if clicks are disliked. Method 4a for software trip points.

## Bare-bones

Optional low-cost kit: Method 3 clicks + a **3.5 mm TS cord**. No ESP32, AA pack, or boost. Same chassis STLs as the full kit (AA well and ESP32 tray stay empty). Either click closes the cord — **binary** PowerLink / switch-box. Two-level HID / iPad Switch Control needs the USB-bank upgrade below.

### Print

Same mechanical stack as the first-kit table. Skip `aa_2x_shim`. Print a **second** `p_clip` if you will add a USB power bank later.

### Wire (no power)

Gold D2Fs. NC taped. Flying cord through the jack hole is cheapest; SJ1-3513N is optional.

```
D2F-01F-D3 COM → cord sleeve
D2F-01-D3  COM → cord sleeve
D2F-01F-D3 NO  → cord tip
D2F-01-D3  NO  → cord tip
```

`p_clip` on the +Y inner wall, 40 mm loop. Skip ESP32, AA holder, MT3608, slide, ID resistors, 2N7000, APH, module header. Diodes are only needed when GPIO is also attached — if you will drop in an ESP32 later, populate the Method 3 1N4148 diode-OR to the jack now.

### USB power bank (all methods)

Drop the ESP32 into the existing tray. USB-A → Micro-USB through the rear **18 × 12 mm** cut (cover has a matching rear-edge notch). Second `p_clip` on the −X inner floor, +Y of the USB window. User-supplied bank — some banks auto-off at low load; BLE idle usually keeps them awake.

Skip AA, boost, slide, Schottky (USB 5 V on the DevKit already powers analog + the GPIO27 FET jack). Serial `PACK USB` (DUMP `"pack":0`; no low-batt blink). Analog modules use the existing FET jack netlists. Method 3 diode-OR still works with board power off.

## Build

First classroom kit: Method 3 + APH `1-08615-00` on GPIO33. Do these in order. Pin map, netlists, and later methods stay in the sections below.

Web Serial calibrator on GitHub Pages: <https://grahamthetvi.github.io/Stick-Switch/> (this README is the build guide). Enable once: **Settings → Pages → Source: GitHub Actions**.

### 1. Print

Print the first-kit table above. PETG 0.20 mm, 5 perimeters, 40% gyroid. TPU 95A for `grip_sleeve`, `bellows`, `tpu_foot`. **`aa_2x_shim` only for a 2x AA holder.** STLs: `cad/stl/` (`bash cad/export_all.sh`).

### 2. Heat-set, hinge, cover

Brass inserts into PETG with a soldering iron. Posts stay hot — burn hazard. Let them cool before handling.

- Cover: 4× short M3 inserts in the corner posts (from the top).
- Module bay: 4× M2.5 inserts in the pocket floor (34×26 mm) — M3 does not fit the 40×32 mm plate.
- Mount: 1/4-20 insert from the underside **under the stick**, not under the AA well.
- Chassis tray and Method 4b: M3 through-slots (3.3 mm). No heat-set. D2F / Hall hardware stays M2.

Hinge: press one **623ZZ** into each tower (not the rocker). **M3×20** through both. McMaster **9271K22** LH 90° torsion spring on the left-tower 4.4 mm boss. One leg in a preload hole (start at 60°); the other on the rocker. **Do not mix 9271K21 RH.** Spring legs can fly off — eye protection, hold both legs until the cover is on.

Cover screws into the M3 inserts. Cover M3 set-screw is the travel stop (12 mm grip / 14 mm slam). 8.0 mm OD × 140 mm 6061 through `stick_collar`, TPU `grip_sleeve` over the tube, TPU `bellows` into the cover groove. Four TPU feet. `p_clip` on the +Y inner wall, −X of the jack: M3 through the clip and floor, nut on the underside, **40 mm** slack loop. 0.4 mm foam on the cover stop. Loctite 222 after a classroom trial, not before.

### 3. Wire Method 3

Gold contacts only — no silver D2F on GPIO. Soft **D2F-01F-D3** → GPIO18. Hard **D2F-01-D3** → GPIO19. COM → GND. NC taped.

```
D2F-01F-D3 COM → GND
D2F-01F-D3 NO  → GPIO18 and 1N4148 anode
D2F-01-D3  COM → GND
D2F-01-D3  NO  → GPIO19 and 1N4148 anode
both 1N4148 cathodes → jack Tip
jack Sleeve → GND
ID 10k between header pin 8 and GND
```

Optional LEDs: GPIO25 soft, GPIO26 hard, GPIO16 grip (GPIO → 330 Ω → LED anode; cathode → GND). Optional piezo GPIO13.

### 4. APH backup

APH Adaptable Stick Switch **1-08615-00** on **GPIO33 + GND** the same day. Solder or a flying 3.5 mm TS pigtail (tip → GPIO33, sleeve → GND). The chassis **SJ1-3513N is PowerLink OUTPUT**, not an APH input — do not plug the APH into it.

### 5. Install AA pack

Well **60 × 50 × 18 mm** on −Y. Prefer 3x AA for first kit. AA holder options (pack, Keystone part, Digi-Key SKU, and well fit):

| Pack | Holder | Digi-Key | Fit |
| --- | --- | --- | --- |
| 3x AA | Keystone **2465** (6″ leads) or **2464** (PC pins) | `36-2465-ND` / `36-2464-ND` | ~58 × 48 × 17 mm |
| 2x AA | Keystone **2463** (6″ leads) or **2462** (PC pins) | `36-2463-ND` / `36-2462-ND` | ~58 × 32 × 17 mm; print `aa_2x_shim` into +Y slack |

Snap-on 9V-style Keystone **2475** (3x) / **2474** (2x) also fit; add a 9V snap pigtail. Amazon clones are fine if they measure those envelopes. (Older notes said 2468 / 2466-class — those SKUs are AAA / not this well.)

```
holder + → SS-12F15 → MT3608 IN
MT3608 5.0 V → 1N5819/SS14 Schottky → ESP32 VIN
holder − → GND
100k / 100k from pack + → GPIO34 → GND
```

Alkaline AA, polarity as marked on the holder. **No in-chassis charging.** Slide **OFF** while programming (USB 5 V fights VIN if the switch is ON). Serial `PACK 2` or `PACK 3`. USB-bank (no AA): `PACK USB` — [Bare-bones](#bare-bones).

### 6. Connect PowerLink

Same Sky **SJ1-3513N**: tip = NO, sleeve = GND, ring unused. 3.5 mm **TS** cable from this jack into the AbleNet PowerLink **SWITCH** jack. Independent of BLE and of `PROFILE`. Method 3 diode-OR closes the jack with the ESP32 **off**. Analog FET (GPIO27 / 2N7000) needs board power.

### 7. Grip LED (optional)

Isolate the 6061 tube in the PETG `stick_collar`. Run a wire from the tube (collar slot) to GPIO14 (touch T6). LED on GPIO16 only — never HID, never jack. Serial `GRIP CAL` at rest; `GRIP THRESH <n>` if needed.

### 8. Flash

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (or the VS Code PlatformIO IDE). Slide switch OFF.

```
cd firmware/stick_switch
pio run -e esp32dev --target upload
pio device monitor -b 115200
```

If upload fails: hold **BOOT**, tap EN, release BOOT, retry. Type `HELP` then `DUMP`.

### 9. First boot

Slide ON. Serial:

```
PACK 3
PROFILE IPADOS
```

Use `PACK 2` if the holder is 2x AA. Pair BLE keyboard `Stick-Switch-XXXX` (last two MAC bytes). iPad **Settings → Accessibility → Switch Control → Switches → External**: `1` = Move to Next Item, `2` = Select Item. Chrome/Edge calibrator: <https://grahamthetvi.github.io/Stick-Switch/> (`tools/calibrate.html`, Web Serial, 115200). Method 3 clicks need **no** analog CAL.

### 10. First-kit bench

Same checks as [Bench](#bench-all-methods) below: luggage scale, `PLOT 1`, Notes app `1` then `2`, Method 3 jack continuity with power off.

## HID / iPad

Pair the ESP32 BLE keyboard named `Stick-Switch-XXXX` (last two MAC bytes; manufacturer **Graham Labs**). Then **Settings → Accessibility → Switch Control → Switches → External**:

- `1` = Move to Next Item
- `2` = Select Item

Firmware never holds both keys. Crossing hard **releases the soft key and presses the hard key**. Easing off hard without a full release stays on hard. Full release clears keys. Profiles change which HID codes those levels send; they do **not** change the jack.

HID key map by `PROFILE` (soft, hard, and binary/APH):

| `PROFILE` | Soft | Hard | Binary / APH |
| --- | --- | --- | --- |
| `IPADOS` (default) | `1` | `2` | Space |
| `ANDROID` | Tab | Return | Return |
| `FUNCTION` | F1 | F2 | F3 |
| `MEDIA` | vol down | vol up | play/pause |
| `CUSTOM` | NVS `KEYS` / `BINARY_KEY` | | |

Serial: `PROFILE IPADOS`, `MODE BINARY`, `BINARY_KEY SPACE`. `KEYS` / `BINARY_KEY` switch to `CUSTOM` and persist.

## Firmware

ESP32 DevKit V1 / WROOM, PlatformIO env `esp32dev` (`firmware/stick_switch`). Uses NimBLE + ESP32 BLE Keyboard. **ADC1 only** (GPIO32 sensor, GPIO34 battery, GPIO35 module ID). Do not use ADC2 — BLE owns the radio.

```
cd firmware/stick_switch
pio run -e esp32dev --target upload
pio device monitor -b 115200
```

If upload fails, hold **BOOT**, tap EN, release BOOT. First-kit procedure: [Build](#build).

Native policy tests (no hardware):

```
make -C firmware/native_test test
```

USB serial 115200. `HELP` lists commands. Chrome/Edge calibrator: <https://grahamthetvi.github.io/Stick-Switch/> (same file as `tools/calibrate.html`; Web Serial needs HTTPS or localhost). Wizard is Rest/TARE → hold soft/CAL SOFT → hold hard/CAL HARD. `MODE`, `PROFILE`, `PACK` (`2` / `3` / `USB`), `TARE`, `CAL STOP`, `KEYS`, `BINARY_KEY`, `INVERT`, `LUT`, `SCALE`, `GRIP THRESH`, and `GRIP CAL` **auto-save to NVS**. Classroom power-cycle keeps cal. `SAVE` is still an explicit dump confirmation.

BOOT button (GPIO0, after setup): 1 tap = simulate soft, 2 taps = simulate hard, hold = TARE (or CAL start/stop if already collecting / analog held). Optional piezo on GPIO13: startup, paired, click, low battery (silent if unpopulated).

ESP32 pin map (GPIO and function):

| GPIO | Function |
| --- | --- |
| 0 | BOOT diagnostics (INPUT_PULLUP after setup) |
| 18 | SW_SOFT (INPUT_PULLUP) |
| 19 | SW_HARD (INPUT_PULLUP) |
| 33 | APH backup (INPUT_PULLUP) |
| 32 | ADC FSR / Hall (ADC1_CH4) |
| 34 | Pack millivolts (ADC1_CH6), 100k/100k from pack + |
| 35 | Module ID (ADC1_CH7), header pin 8 |
| 21 / 22 | I2C SDA / SCL (NAU7802) |
| 4 | TARE |
| 23 | CAL (reserved; serial is primary) |
| 25 / 26 | LED soft / hard |
| 16 | LED grip (indicator only — never HID, never jack) |
| 27 | JACK_OUT → 2N7000 gate (PowerLink) |
| 14 | Capacitive grip (T6) on isolated 6061 tube |
| 13 | Optional piezo earcon |

NVS modes: `MICRO` `FSR` `HALL` `LOAD` `BINARY`. On boot a valid module ID sets the matching mode; serial `MODE` still overrides for this session (and auto-saves). Empty header keeps the NVS mode. `DUMP` JSON includes `mode`, `profile`, `ble_name`, `level`, `batt_mv`, `batt_pct`, `pack`, `grip`, `module_id`, `module_ohms`, and cal fields. `pack` is `2` / `3` for AA or `0` for `PACK USB`. Idle low-batt LED blink follows the selected AA pack curve (≤15%); USB pack never reports low. Grip hold lights GPIO16 only.

Module ID table (plate, resistor to GND, and `module_id`; 47k pull-up to 3V3 on the chassis):

| Plate | Resistor | `module_id` |
| --- | --- | --- |
| Method 3 | 10k | `MICRO` |
| Method 2 | 22k | `FSR` |
| Method 4a | 47k | `HALL` |
| Method 4b | 100k | `LOAD` |
| none | open | `EMPTY` |

## AA power well and packs

Chassis is **110 × 108 × 32 mm**. One well on the −Y side, **60 × 50 × 18 mm**, takes a 3x AA holder (Keystone **2465** / **2464**, Digi-Key `36-2465-ND` / `36-2464-ND`, ~58 × 48 × 17 mm). A 2x AA holder (Keystone **2463** / **2462**, `36-2463-ND` / `36-2462-ND`, ~58 × 32 × 17 mm) locates in the same well against −Y; print `aa_2x_shim` into the leftover +Y slack. MT3608 sits +X of the holder. No TP4056 — **do not charge AA cells in the chassis**. The 1/4-20 insert stays under the stick, not under the pack. ESP32 stays in the aisle between the towers.

```
AA holder (2x or 3x series) → SS-12F15 slide → MT3608 IN
MT3608 5.0 V → 1N5819/SS14 Schottky → ESP32 VIN
100k / 100k from pack + → GPIO34 → GND
Program: ESP32 USB with the slide switch OFF
```

Serial `PACK 2` / `PACK 3` / `PACK USB` (NVS). Unset pack auto-picks from millivolts on boot (≥3.3 V → 3cell). Curves: 2x empty ~2.0 V full ~3.2 V; 3x empty ~3.0 V full ~4.8 V. BLE HID battery percent and idle low-batt LED use that curve. USB-bank builds skip the AA well: `PACK USB` reports 100% and does not blink low-batt (GPIO34 is floating with no divider).

If USB is live and the switch is ON, 5 V fights at VIN — leave the slide **OFF** while programming. The Schottky stops USB from backfeeding the boost. USB-only (no pack) has no VIN fight.

## PowerLink jack

Same Sky **SJ1-3513N** 3.5 mm TRS, tip = NO, sleeve = GND, ring unused (APH TS plug mates). This is the **PowerLink / wired switch-box output**. It is independent of BLE connection and of `PROFILE`. Method 3 **diode-OR still closes the jack with the ESP32 off**. Analog modules (FSR / Hall / 4b) drive the jack with a 2N7000 from GPIO27 — that path **dies with the board**.

## Grip LED (tube sensor)

Wire the isolated 6061 tube (PETG `stick_collar`) to GPIO14 (touch T6). `touchRead` below the NVS threshold lights GPIO16 only — never HID, never jack. Serial `GRIP THRESH <n>`, `GRIP CAL` (at rest). Fallback: two foil bands on the TPU sleeve, 3.3 V through 1 MΩ, ADC; do not run meaningful current through the student.

## Shared chassis

OpenSCAD: `cad/stick_switch.scad`. STLs: `cad/stl/` (regenerated from this scad; `part="kit_preview"` is exploded first-print only). Export: `bash cad/export_all.sh`. `base_chassis` may warn “not a valid 2-manifold” in OpenSCAD; it is one solid and slices.

Print PETG, 0.20 mm, 5 perimeters, 40% gyroid, no living hinge as the return spring. Shared chassis materials for all methods (not the first-kit print list):

| Part | Material |
| --- | --- |
| `base_chassis` `cover_slot` `rocker_arm` `stick_collar` `p_clip` | PETG |
| `grip_sleeve` `bellows` `tpu_foot` (×4) `magnet_plug` | TPU 95A |
| Module plates | PETG (TPU puck only on Method 2) |

Tolerances: M3 holes and tray slots 3.3 mm, 623ZZ seats 10.1 mm (`$fn` 72), stick slot 8.4 mm (Y) × 12 mm (X), module pocket 40×32 mm, M2.5 pattern 34×26 mm. D2F / Hall stay M2. Stick: **8.0 mm OD × 140 mm 6061**, grip center **110 mm** from hinge. Cam at **25 mm**. Cover M3 set-screw is the travel stop (12 mm grip / 14 mm slam; **0.4 mm cam** for Method 4b).

Return: McMaster **9271K22** left-hand 90° piano-wire torsion spring (0.281″ OD, 0.172″ shaft, 0.030″ wire, 3.25 coils, 1″ legs, 0.67 in·lbf max at 90°). Do not mix **9271K21** RH. Printed 4.4 mm boss on the left tower; preload holes 0/30/60/90° at 7.5 mm radius. Start at 60° (≈0.5 N at the grip); 90° if the stick feels light. Target **0.5–0.7 N** at grip (luggage scale). Hinge: M3×20 + two **623ZZ in the towers**. Side-load is killed by the cover slot. Mount: 1/4-20 insert, 4× M3 tray slots, Dual Lock SJ3550, TPU feet. Cord: printed `p_clip` + 40 mm loop. USB power-bank: rear-wall **18 × 12 mm** cut + cover notch; second `p_clip` on the −X inner floor. Pinch: TPU bellows, edges R≥2 mm.

8-pin module header (rear, pin 1 = 3V3): `3V3, GND, SOFT, HARD, ADC, SDA, SCL, ID`. ID → GPIO35.

PowerLink jack: Same Sky **SJ1-3513N**. Tip = NO, Sleeve = GND, Ring unused (APH TS plug mates).

## Method 3 — staged microswitches (first trial)

- Soft: Omron **D2F-01F-D3** (0.74 N, gold, solder lug)
- Hard: Omron **D2F-01-D3** (1.47 N, gold)
- `mod3_switch_plate` + optional `mod3_ramp_cam`
- Hard switch **1.6 mm** further from the cam. Target grip travel **4–6 mm / 10–12 mm / 14 mm stop**. 0.4 mm foam on the stop.

Wiring (COM→GND, NO→GPIO, NC taped):

```
D2F-01F-D3 COM → GND
D2F-01F-D3 NO  → GPIO18 and 1N4148 anode
D2F-01-D3  COM → GND
D2F-01-D3  NO  → GPIO19 and 1N4148 anode
both 1N4148 cathodes → jack Tip
jack Sleeve → GND
ID 10k between header pin 8 and GND
```

Firmware: `MODE MICRO`. 1 kHz poll, 15 ms debounce. HARD if GPIO19 low (even if GPIO18 failed). No downgrade until both high 15 ms. Jack still works with ESP32 **off** (diode-OR). No-ESP32 parallel-to-cord wiring: [Bare-bones](#bare-bones).

Do not use silver D2F parts on GPIO.

## Method 2 — FSR

Adafruit **#166** (Alpha MF01A-N-221-A01) or Interlink **34-00012**. `mod2_anvil` + TPU `mod2_puck` Ø8×3 + `mod2_retainer` (1.4 mm, printable). Do not solder the FSR tail.

```
FSR A → 3V3
FSR B → GPIO32 and 47k to GND
100 nF GPIO32 → GND
GPIO27 → 10k to GND and 2N7000 gate
2N7000 drain → jack Tip; source → GND
ID 22k between header pin 8 and GND
```

`MODE FSR`. 200 Hz, boxcar/8, 8% hysteresis. Until cal: soft = tare+250, hard = tare+1200. Then `TARE`, five hard holds, `CAL START HARD` / `CAL STOP` (stores 35% / 70% of span). Jack FET is dead when the ESP32 is off.

## Method 4a — Hall travel

Honeywell **SS49E** + encapsulated **6×3 mm N35** (not N52). Rest gap 5 mm, stop gap 2 mm. Brass/nylon M2. `mod4a_hall_tray` + `gap_gauge_2`…`6` + `magnet_plug`.

Honeywell TO-92 (verify bag): pin1 Vcc, pin2 GND, pin3 OUT.

```
Vcc → 3V3   GND → GND   OUT → GPIO32
100 nF Vcc–GND   10 nF OUT–GND
jack FET as Method 2
ID 47k between header pin 8 and GND
```

South toward branded face must **raise** ADC; else `INVERT AN 1` or flip magnet. `MODE HALL`. 5-point LUT:

```
LUT 1800,0,2000,3,2300,6,2700,9,3100,12
```

Sweep grip 0–12 mm; span must be ≥800 counts. Soft 5 mm / hard 11 mm / hyst 0.8 mm. Tares at BLE connect via rest ADC (`TARE`). If the student mouths objects, skip this module.

## Method 4b — load cell (later; near-zero travel)

SparkFun **SEN-13329 TAL220 10 kg** (or YZC-133 5 kg) is **55 × 12.7 mm** and does not fit the 40×32 mm module pocket. `mod4b_cell_saddle` is a **72 × 24 mm** chassis-mounted beam that bolts to the M3 slots under the cam — not a 40 mm plate. The TAL220 bar's own holes stay the vendor size (usually M5). Later experiment, not first-print. Adafruit **4538** NAU7802, gain 128, LDO 3.0 V, 80 SPS. `mod4b_ball_anvil`, `feeler_0_4` (0.4 mm **gauge**, not a structural part). Set cover stop to **0.4 mm** cam motion.

```
Red → E+   Black → E−   Green → A+   White → A−
NAU VIN→3V3  GND→GND  SDA→GPIO21  SCL→GPIO22
(no extra I2C pull-ups on STEMMA)
ID 100k between header pin 8 and GND
```

`MODE LOAD`. Soft 120 gf / hard 300 gf / hyst 25 gf until student cal. Quiet slow-tare 1%/3 s only when abs(g) < 15. I2C fail → APH GPIO33 still works. Clone color-code: `INVERT LD 1`. Scale with 500 g on the **grip**: `SCALE <gf_per_count>`. Jack FET is dead when the ESP32 is off.

## Bench (all methods)

1. Luggage-scale **10 presses** at the grip; rewrite thresholds from that, not from 1.2 / 3.0 N defaults.
2. Serial `PLOT 1`: 20 slow, 20 fast, 10 slams. Dual-fire count must be 0.
3. iPad Notes: one stroke types `1` then `2`, never overlapping `12`.
4. Switch Control mapping as above.

Method 3 extra: continuity on each NO; jack closes on either click with power off.

## Student trial (10 min, eyes-free after demo)

Watch: two distinct levels, accidental hard, fatigue, resting on the stick (false soft), inability to unload (spring too strong). Abort the last. Method 4b: “push harder, don’t push farther.”

## Safety

Bellows over the hinge. Encapsulated magnet only. No loose neodymium. Clip the cord with printed `p_clip` (40 mm loop). Overload posts on 4b. Loctite 222 after classroom trial. Alkaline AA only; polarity as marked; no in-chassis charging. Slide switch off in a bag. Heat-set posts burn. Hold **9271K22** legs until the cover is on (do not mix **9271K21** RH).

## BOM

Machine-readable list: [`hardware/bom.csv`](hardware/bom.csv). `used_on` includes `wired` (no-power Method 3 + cord) and `usb_bank` (ESP32 from a USB power bank).

## GitHub Pages

<https://grahamthetvi.github.io/Stick-Switch/> is the Web Serial calibrator (canonical HTML: `tools/calibrate.html`). This README is the build guide. Workflow `.github/workflows/pages.yml` copies that file to the Pages artifact as `index.html` (static deploy, not Jekyll). Enable once: repo **Settings → Pages → Build and deployment → Source: GitHub Actions**. Use Chrome or Edge; USB serial **115200**. GitHub Pages is HTTPS (required for Web Serial). Safari and Firefox cannot use Web Serial. The same page includes a USB **data**-cable check (plug a known-working USB device through the cable under test); it does not test 3.5 mm PowerLink / TS cords.
