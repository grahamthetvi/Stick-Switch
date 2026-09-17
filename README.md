# Stick-Switch

Single-axis **forward** pencil-grip stick for two-level AAC / iPad Switch Control. Soft→hard is an **upgrade** (HID `1` then `2`), never a double-select. The APH Adaptable Stick Switch (`1-08615-00`) stays the binary classroom backup.

Student motion: thumb + index **push forward**. Not a 360° wobble.

## What to print first

Method 3 + APH on GPIO33 is the first classroom experiment. Print:

| Part | Material | Notes |
| --- | --- | --- |
| `base_chassis` | PETG | Floor on the bed |
| `cover_slot` | PETG | Interior face on the bed |
| `rocker_arm` | PETG | Hub face on the bed; bearings are in the towers only |
| `stick_collar` | PETG | |
| `grip_sleeve` `bellows` `tpu_foot` ×4 | TPU 95A | |
| `mod3_switch_plate` | PETG | ID resistor **10k** to GND |
| `mod3_ramp_cam` | PETG | Optional |

Exploded look: `openscad -D 'part="kit_preview"' cad/stick_switch.scad` (not an STL). Keep APH on GPIO33 the same day.

Later: Method 4b if travel is too small (chassis-mounted TAL220 saddle — it does **not** fit the 40×32 mm pocket). Method 2 if clicks are disliked. Method 4a for software trip points.

## HID / iPad

Pair the ESP32 BLE keyboard named `Stick-Switch`. Then **Settings → Accessibility → Switch Control → Switches → External**:

- `1` = Move to Next Item
- `2` = Select Item

Firmware never holds both keys. Crossing hard **releases `1` and presses `2`**. Easing off hard without a full release stays on `2`. Full release clears keys.

Binary / APH uses Space (existing single-switch mapping). Serial: `MODE BINARY`, `BINARY_KEY SPACE`.

## Firmware

ESP32 DevKit V1 / WROOM, PlatformIO env `esp32dev` (`firmware/stick_switch`). Uses NimBLE + ESP32 BLE Keyboard. **ADC1 only** (GPIO32 sensor, GPIO34 battery, GPIO35 module ID). Do not use ADC2 — BLE owns the radio.

```
cd firmware/stick_switch
pio run -e esp32dev --target upload
```

Native policy tests (no hardware):

```
make -C firmware/native_test test
```

USB serial 115200. `HELP` lists commands. Chrome calibrator: `tools/calibrate.html` (Web Serial). Wizard is Rest/TARE → hold soft/CAL SOFT → hold hard/CAL HARD. `MODE`, `TARE`, `CAL STOP`, `KEYS`, `BINARY_KEY`, `INVERT`, `LUT`, and `SCALE` **auto-save to NVS**. Classroom power-cycle keeps cal. `SAVE` is still an explicit dump confirmation.

| GPIO | Function |
| --- | --- |
| 18 | SW_SOFT (INPUT_PULLUP) |
| 19 | SW_HARD (INPUT_PULLUP) |
| 33 | APH backup (INPUT_PULLUP) |
| 32 | ADC FSR / Hall (ADC1_CH4) |
| 34 | Battery millivolts (ADC1_CH6), 100k/100k from the cell |
| 35 | Module ID (ADC1_CH7), header pin 8 |
| 21 / 22 | I2C SDA / SCL (NAU7802) |
| 4 | TARE |
| 23 | CAL (reserved; serial is primary) |
| 25 / 26 | LED soft / hard |
| 27 | JACK_OUT → 2N7000 gate |

NVS modes: `MICRO` `FSR` `HALL` `LOAD` `BINARY`. On boot a valid module ID sets the matching mode; serial `MODE` still overrides for this session (and auto-saves). Empty header keeps the NVS mode. `DUMP` JSON includes `mode`, `level`, `batt_mv`, `module_id`, `module_ohms`, and cal fields. Below ~3400 mV the LEDs blink slowly **only while idle** (not during a press).

Module ID (47k pull-up to 3V3 on the chassis, resistor to GND on the plate):

| Plate | Resistor | `module_id` |
| --- | --- | --- |
| Method 3 | 10k | `MICRO` |
| Method 2 | 22k | `FSR` |
| Method 4a | 47k | `HALL` |
| Method 4b | 100k | `LOAD` |
| none | open | `EMPTY` |

## 18650 power

Chassis is **110 × 108 × 32 mm**. Protected 18650 in a Keystone 1043-class holder (~77 × 20.65 × 14.86 mm) sits in a **78 × 22 × 19 mm** well along +X on the −Y side. Charger and boost sit in the corridor south of the left hinge tower; the ESP32 sits in the aisle between the towers so those wells are not filled in by the towers. The 1/4-20 insert stays under the stick, not under the cell. Use a **protected 18650 or** a TP4056 module **with DW01 protection**.

```
18650 in holder → TP4056 B+ / B−
TP4056 OUT → SS-12F15 slide switch → MT3608 IN
MT3608 5.0 V → 1N5819/SS14 Schottky → ESP32 VIN
100k / 100k from TP4056 OUT (cell) → GPIO34 → GND
Charge: TP4056 USB-C through the rear wall (cover can stay on)
Program: ESP32 USB with the slide switch OFF
```

If both USBs are live and the switch is ON, 5 V fights at VIN — leave the slide **OFF** while programming. The Schottky stops USB from backfeeding the boost.

Jack honesty: Method 3 **diode-OR still closes the jack with the ESP32 off**. Analog modules (FSR / Hall / 4b) drive the jack with a 2N7000 from GPIO27 — that path **dies with the board**.

## Shared chassis

OpenSCAD: `cad/stick_switch.scad`. STLs: `cad/stl/` (regenerated from this scad; `part="kit_preview"` is exploded first-print only). Export: `bash cad/export_all.sh`. `base_chassis` may warn “not a valid 2-manifold” in OpenSCAD; it is one solid and slices.

Print PETG, 0.20 mm, 5 perimeters, 40% gyroid, no living hinge as the return spring.

| Part | Material |
| --- | --- |
| `base_chassis` `cover_slot` `rocker_arm` `stick_collar` | PETG |
| `grip_sleeve` `bellows` `tpu_foot` (×4) `magnet_plug` | TPU 95A |
| Module plates | PETG (TPU puck only on Method 2) |

Tolerances: M3 holes 3.3 mm, 623ZZ seats 10.1 mm (`$fn` 72), stick slot 8.4 mm (Y) × 12 mm (X), module pocket 40×32 mm, M2.5 pattern 34×26 mm. Stick: **8.0 mm OD × 140 mm 6061**, grip center **110 mm** from hinge. Cam at **25 mm**. Cover M3 set-screw is the travel stop (12 mm grip / 14 mm slam; **0.4 mm cam** for Method 4b).

Return: McMaster **9271K22** left-hand 90° piano-wire torsion spring (0.281″ OD, 0.172″ shaft, 0.030″ wire, 3.25 coils, 1″ legs, 0.67 in·lbf max at 90°). Do not mix **9271K21** RH. Printed 4.4 mm boss on the left tower; preload holes 0/30/60/90° at 7.5 mm radius. Start at 60° (≈0.5 N at the grip); 90° if the stick feels light. Target **0.5–0.7 N** at grip (luggage scale). Hinge: M3×20 + two **623ZZ in the towers**. Side-load is killed by the cover slot. Mount: 1/4-20 insert, 4× M4 slots, Dual Lock SJ3550, TPU feet. Cord: P-clip + 40 mm loop. Pinch: TPU bellows, edges R≥2 mm.

8-pin module header (rear, pin 1 = 3V3): `3V3, GND, SOFT, HARD, ADC, SDA, SCL, ID`. ID → GPIO35.

Jack: Same Sky **SJ1-3513N**. Tip = NO, Sleeve = GND, Ring unused (APH TS plug mates).

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

Firmware: `MODE MICRO`. 1 kHz poll, 15 ms debounce. HARD if GPIO19 low (even if GPIO18 failed). No downgrade until both high 15 ms. Jack still works with ESP32 **off** (diode-OR).

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

SparkFun **SEN-13329 TAL220 10 kg** (or YZC-133 5 kg) is **55 × 12.7 mm** and does not fit the 40×32 mm module pocket. `mod4b_cell_saddle` is a **72 × 24 mm** chassis-mounted beam that bolts to the M4 slots under the cam — not a 40 mm plate. Later experiment, not first-print. Adafruit **4538** NAU7802, gain 128, LDO 3.0 V, 80 SPS. `mod4b_ball_anvil`, `feeler_0_4` (0.4 mm **gauge**, not a structural part). Set cover stop to **0.4 mm** cam motion.

```
Red → E+   Black → E−   Green → A+   White → A−
NAU VIN→3V3  GND→GND  SDA→GPIO21  SCL→GPIO22
(no extra I2C pull-ups on STEMMA)
ID 100k between header pin 8 and GND
```

`MODE LOAD`. Soft 120 gf / hard 300 gf / hyst 25 gf until student cal. Quiet slow-tare 1%/3 s only when |g|<15. I2C fail → APH GPIO33 still works. Clone color-code: `INVERT LD 1`. Scale with 500 g on the **grip**: `SCALE <gf_per_count>`. Jack FET is dead when the ESP32 is off.

## Bench (all methods)

1. Luggage-scale **10 presses** at the grip; rewrite thresholds from that, not from 1.2 / 3.0 N defaults.
2. Serial `PLOT 1`: 20 slow, 20 fast, 10 slams. Dual-fire count must be 0.
3. iPad Notes: one stroke types `1` then `2`, never overlapping `12`.
4. Switch Control mapping as above.

Method 3 extra: continuity on each NO; jack closes on either click with power off.

## Student trial (10 min, eyes-free after demo)

Watch: two distinct levels, accidental hard, fatigue, resting on the stick (false soft), inability to unload (spring too strong). Abort the last. Method 4b: “push harder, don’t push farther.”

## Safety

Bellows over the hinge. Encapsulated magnet only. No loose neodymium. P-clip the cord. Overload posts on 4b. Loctite 222 after classroom trial. Protected cell or protected charger; slide switch off in a bag.

## BOM

Machine-readable list: [`hardware/bom.csv`](hardware/bom.csv).
