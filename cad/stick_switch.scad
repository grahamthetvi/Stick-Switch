// Stick-Switch shared single-axis chassis and sensor modules.
// Generate one STL:  openscad -D 'part="base_chassis"' -o cad/stl/base_chassis.stl cad/stick_switch.scad
// First-print kit (Method 3 + APH backup): part="kit_preview" for an exploded look.
//
// Coordinate system (base origin = rear-left-bottom of chassis):
//   +X forward (student push / toward sensor bay)
//   +Y left-to-right (battery on −Y, 3.5 mm jack on +Y)
//   +Z up (stick)
// Hinge axis is +Y through [hinge_x, hinge_y, hinge_z].
//
// First classroom print (PETG unless noted):
//   base_chassis, cover_slot, rocker_arm, stick_collar,
//   grip_sleeve (TPU), bellows (TPU), tpu_foot x4, p_clip,
//   mod3_switch_plate, optional mod3_ramp_cam.
// Method 4b is a later chassis-mounted TAL220 saddle — not a 40 mm plate.

include <lib.scad>

part = "base_chassis"; // see render_part() at bottom

// Packing: 3x AA well on −Y (2x AA uses the same well + printed shim),
// MT3608 +X of the holder, ESP32 in the aisle BETWEEN the towers,
// module bay +X of ESP32. No in-chassis charger.
// tower_off=23 so the inner span (36 mm) clears the 29 mm DevKit.
base_x = 110;
base_y = 108;
base_z = 32;
wall = 2.4;
r_ext = 2.0;

hinge_x = 56;
hinge_y = 74;
hinge_z = 18; // keep cam height on Method 3 D2F plungers
cam_r = 25;
grip_r = 110;
stick_od = 8.0;
stick_slot_y = 8.4;
stick_slot_x = 12.0;
tower_off = 23;
tower_w = 10;
tower_d = 16;

module_pocket = [40, 32, 18];
module_clear = 0.2;
module_holes = [34, 26];
module_origin = [hinge_x + 8, hinge_y - module_pocket[1] / 2, wall];

cover_z = 4.0;
insert_1_4_20_d = 8.2;
insert_1_4_20_h = 10.0;

esp32 = [55, 29, 13];
esp32_origin = [wall + 1.5, hinge_y - esp32[1] / 2, wall];

// 3x AA holder (Keystone 2465/2464, ~58 x 48 x 17 mm). Pocket has
// ~0.8–1.2 mm XY clearance and extra Z so the holder sits fully in the well.
// A 2x AA holder (Keystone 2463/2462, ~58 x 32 x 17) locates against −Y; print aa_2x_shim
// into the leftover +Y slack. Same well, no second pocket.
batt_pocket = [60, 50, 18];
batt_origin = [11, wall, wall]; // between the −Y cover posts
aa_2x_w = 32.8;
aa_shim = [batt_pocket[0] - 0.8, batt_pocket[1] - aa_2x_w, 16];

// MT3608 boost ~36 x 17 x 7 mm, set to 5.0 V, output through Schottky to VIN.
// Sits +X of the AA well, north of the front-left cover post.
boost = [36, 17, 7];
boost_origin = [batt_origin[0] + batt_pocket[0] + 2, batt_origin[1] + 12, wall];

// SS-12F15 on rear wall, between the AA well and ESP32 USB.
sw_y = batt_origin[1] + batt_pocket[1] + 2;
sw_z = 8;
sw_slot = [10, 5];

// Cover screws in corner posts that do not invade the AA well.
cover_x1 = 6;
cover_x2 = base_x - 6;
cover_y1 = 6;
cover_y2 = base_y - 6;

// P-clip M3 through the floor, +Y inner wall, −X of the jack.
// Clears the SJ1-3513N body and the +X+Y cover post.
pclip_x = base_x - 38;
pclip_y = base_y - wall - 6;

// USB-A → Micro-USB power-bank plug. Rear-wall window 18 x 12 mm
// (molded strain relief; the old 12 x 8 mm cut was too tight).
usb_cut_y = 18;
usb_cut_z = 12;
usb_cut_z0 = wall + 2;
// Second P-clip for the USB cable, floor just +Y of the DevKit USB.
usb_pclip_x = wall + 6;
usb_pclip_y = hinge_y + usb_cut_y / 2 + 1;

// McMaster 9271K22 LH 90° piano-wire torsion spring.
// 0.281" OD, 0.172" shaft, 0.030" wire, 3.25 coils, 1" legs, 0.67 in·lbf @ 90°.
spring_shaft_d = 4.4;  // 0.173" printed boss over the M3 hinge pin
spring_hole_d = 2.0;
spring_leg_r = 7.5;

// Method 4b later: TAL220 saddle M3 slots under the cam, outside the 40 mm pocket.
m4b_cx = hinge_x + cam_r;
m4b_dx = 24;

module cover_screw_xy() {
  for (px = [cover_x1, cover_x2])
    for (py = [cover_y1, cover_y2])
      translate([px, py, 0])
        children();
}

// --- chassis ---

module hinge_towers() {
  z0 = wall - 0.5;
  for (s = [-1, 1])
    translate([hinge_x - tower_d / 2, hinge_y + s * tower_off - tower_w / 2, z0])
      rounded_box([tower_d, tower_w, hinge_z + 7 - z0], 1.5);
  // spring shaft boss, inner face of left (−Y) tower, 9271K22 0.172" shaft
  translate([hinge_x, hinge_y - tower_off + tower_w / 2 - 1.5, hinge_z])
    rotate([-90, 0, 0])
      cylinder(h = 6.0, d = spring_shaft_d);
}

module base_chassis() {
  // Print: exterior floor on the bed (Z=0). Towers and wells point up.
  // Towers are added after the basin cut so they stay solid.
  difference() {
    union() {
      difference() {
        rounded_box([base_x, base_y, base_z], r_ext);

        // interior basin (open top)
        translate([wall, wall, wall])
          rounded_box([base_x - 2 * wall, base_y - 2 * wall, base_z], 1.2);

        // 3x AA well along X on −Y; 0.6 mm floor recess. 2x AA uses aa_2x_shim.
        translate([batt_origin[0], batt_origin[1], wall - 0.6])
          cube([batt_pocket[0], batt_pocket[1], batt_pocket[2] + 2]);

        // MT3608 well, +X of the AA holder (no TP4056 / no in-chassis charging)
        translate([boost_origin[0], boost_origin[1], wall - 0.4])
          cube([boost[0], boost[1], boost[2] + 4]);

        // SS-12F15 slide switch through rear wall (pack off in a bag)
        translate([-0.2, sw_y, sw_z - sw_slot[1] / 2])
          cube([wall + 6, sw_slot[0], sw_slot[1]]);

        // module pocket (open to interior, registered to cam)
        translate([module_origin[0], module_origin[1], module_origin[2]])
          cube([module_pocket[0], module_pocket[1], module_pocket[2] + 2]);

        // 8-pin 2.54 mm header well at rear of pocket: 3V3 GND SOFT HARD ADC SDA SCL ID
        translate([
          module_origin[0] - 3.2,
          module_origin[1] + module_pocket[1] / 2 - 10.8,
          wall + 2
        ])
          cube([4.0, 21.6, 10]);

        // ESP32 DevKit tray (USB toward −X / rear)
        translate([esp32_origin[0], esp32_origin[1], wall - 0.4])
          cube([esp32[0], esp32[1], esp32[2] + 6]);
        translate([-0.2, hinge_y - usb_cut_y / 2, usb_cut_z0])
          cube([wall + 6, usb_cut_y, usb_cut_z]);

        // USB-cable P-clip, −X inner floor, +Y of the USB window
        translate([usb_pclip_x, usb_pclip_y, -0.1])
          m3_clearance(h = wall + 2);

        // 3.5 mm jack well (SJ1-3513N), +Y wall, tip toward outside
        translate([base_x - 22, base_y - wall - 0.2, 8])
          rotate([-90, 0, 0])
            cylinder(h = wall + 6, d = 6.2);

        // P-clip / strain-relief M3 on +Y inner wall (not in the AA well)
        translate([pclip_x, pclip_y, -0.1])
          m3_clearance(h = wall + 2);

        // 1/4-20 brass insert, underside under the stick — not under the AA pack
        translate([base_x / 2, hinge_y, -0.1])
          cylinder(h = insert_1_4_20_h, d = insert_1_4_20_d);

        // 4x M3 tray slots, 20 mm, clear of the battery well
        for (px = [12, base_x - 12])
          for (py = [batt_origin[1] + batt_pocket[1] + 8, base_y - 10])
            translate([px, py, -0.1])
              rotate([0, 0, 90])
                m3_slot(length = 20, h = wall + 2);

        // Method 4b later: M3 slots under the cam, outside the 40 mm pocket
        for (dx = [-m4b_dx, m4b_dx])
          translate([m4b_cx + dx, hinge_y, -0.1])
            rotate([0, 0, 90])
              m3_slot(length = 12, h = wall + 2);

        translate([wall - 0.2, hinge_y + esp32[1] / 2 + 2, wall + 2])
          cube([wall + 1, 8, 6]);
      }

      hinge_towers();

      // cover-screw posts (inserts live here, not in empty basin)
      cover_screw_xy()
        translate([-4, -4, wall - 0.4])
          rounded_box([8, 8, base_z - wall + 0.4], 1.2);

      // module pocket floor (overlaps chassis floor so CGAL sees one volume)
      translate([module_origin[0], module_origin[1], wall - 0.4])
        cube([module_pocket[0], module_pocket[1], 2.0]);

      // rib between AA well and electronics so the holder cannot wander +Y
      translate([batt_origin[0], batt_origin[1] + batt_pocket[1] - 0.4, wall - 0.4])
        cube([batt_pocket[0], 2.0, 10.4]);
    }

    // module M2.5 inserts (34 x 26 mm) through the added pocket floor
    translate([
      module_origin[0] + (module_pocket[0] - module_holes[0]) / 2,
      module_origin[1] + (module_pocket[1] - module_holes[1]) / 2,
      wall - 0.1
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0]) {
            m25_insert(h = 5.2);
            cylinder(h = 2.2, d = 2.6);
          }
    }

    // cover M3 inserts through the corner posts
    cover_screw_xy()
      translate([0, 0, base_z - 6])
        m3_insert(h = 6.2);

    // hinge through-holes in the towers only — do not bore the AA well
    translate([hinge_x, hinge_y - tower_off - tower_w / 2 - 0.2, hinge_z])
      rotate([-90, 0, 0])
        cylinder(h = 2 * tower_off + tower_w + 0.4, d = 3.3);
    translate([hinge_x, hinge_y - tower_off - tower_w / 2, hinge_z])
      rotate([-90, 0, 0])
        bearing_623zz_seat();
    translate([hinge_x, hinge_y + tower_off + tower_w / 2, hinge_z])
      rotate([90, 0, 0])
        bearing_623zz_seat();

    // 9271K22 LH preload holes in left tower: 0 / 30 / 60 / 90 deg
    for (a = [0, 30, 60, 90])
      translate([hinge_x, hinge_y - tower_off - tower_w / 2 - 0.2, hinge_z])
        rotate([-90, a, 0])
          translate([spring_leg_r, 0, 0])
            cylinder(h = 10, d = spring_hole_d);
  }
}

module cover_slot() {
  // Print: interior face (Z=0, mates to chassis) on the bed.
  difference() {
    union() {
      rounded_box([base_x, base_y, cover_z], r_ext);
      // bellows groove ring around stick slot
      translate([hinge_x, hinge_y, cover_z])
        cylinder(h = 2.2, d = 22);
    }

    // stick slot: 8.4 mm Y (side-load reject), 12 mm X (travel)
    translate([hinge_x, hinge_y, -0.2])
      hull() {
        translate([-stick_slot_x / 2 + stick_slot_y / 2, 0, 0])
          cylinder(h = cover_z + 4, d = stick_slot_y);
        translate([stick_slot_x / 2 - stick_slot_y / 2, 0, 0])
          cylinder(h = cover_z + 4, d = stick_slot_y);
      }

    // bellows groove (1.2 mm channel)
    translate([hinge_x, hinge_y, cover_z + 0.6])
      difference() {
        cylinder(h = 1.4, d = 20.4);
        cylinder(h = 1.5, d = 17.6);
      }

    // M3 cover screws
    cover_screw_xy()
      translate([0, 0, -0.1]) {
        m3_clearance(h = cover_z + 3);
        translate([0, 0, cover_z - 1.6])
          countersink_m3(h = 2.2);
      }

    // M3 hard-stop set screw, 25 mm forward of hinge (cam radius)
    translate([hinge_x + cam_r - 2, hinge_y, -0.1]) {
      cylinder(h = cover_z + 4, d = 2.5); // tap M3
      translate([0, 0, cover_z - 0.4])
        cylinder(h = 1.2, d = 5.5); // hex access
    }

    // access window over ESP32 USB / EN
    translate([4, hinge_y - 10, -0.1])
      rounded_box([18, 20, cover_z + 1], 1.5);

    // rear-edge U-notch so a power-bank Micro-USB plug clears the lid lip
    translate([-0.2, hinge_y - usb_cut_y / 2, -0.1])
      cube([8, usb_cut_y, cover_z + 1]);

    // rear-edge notch so the slide switch stays reachable with the lid on
    translate([-0.2, sw_y - 1, -0.1])
      cube([7, sw_slot[0] + 2, cover_z + 1]);
  }
}

module rocker_arm() {
  // Print: one hub face on the bed (arm along +X). Bearings are in the towers.
  arm_y = 8;
  difference() {
    union() {
      // hub — 8 mm thick, M3 through only (no 623ZZ seats)
      rotate([90, 0, 0])
        cylinder(h = arm_y, d = 16, center = true);
      // stick boss (+Z)
      cylinder(h = 16, d = 16);
      // cam arm (+X), 12 x 8 cam face at 25 mm
      hull() {
        rotate([90, 0, 0])
          cylinder(h = arm_y, d = 12, center = true);
        translate([cam_r, 0, -2])
          cube([12, arm_y, 8], center = true);
      }
      // 10 deg ramp + 1.5 mm flat on cam underside (presses −Z onto module)
      translate([cam_r - 2, 0, -6])
        rotate([0, 10, 0])
          cube([14, 8, 3], center = true);
      // 9271K22 moving-leg hook, +X / +Z of the hub (matches left-tower holes)
      translate([6.5, 0, 4])
        cube([3.2, arm_y, 3.2], center = true);
    }

    // M3 hinge bore
    rotate([90, 0, 0])
      cylinder(h = 20, d = 3.3, center = true);

    // stick bore + pinch slot
    translate([0, 0, -1])
      cylinder(h = 22, d = 8.2);
    translate([0, 0, 6])
      cube([1.4, 18, 16], center = true);
    // 2x M3 pinch
    for (z = [6, 12])
      translate([0, 0, z])
        rotate([90, 0, 0])
          cylinder(h = 20, d = 3.3, center = true);

    // magnet pocket: 6x3 mm N35, 8 mm from cam face, along +X
    translate([cam_r - 8, 0, 0])
      rotate([0, 90, 0])
        cylinder(h = 3.3, d = 6.2);
  }
}

module stick_collar() {
  // Print: large cylinder end on the bed.
  difference() {
    hull() {
      cylinder(h = 14, d = 16);
      translate([6, 0, 0])
        rounded_box([8, 12, 14], 1.2, center = true);
    }
    translate([0, 0, -0.2])
      cylinder(h = 15, d = 8.1);
    translate([0, 0, 7])
      cube([1.4, 18, 16], center = true);
    for (z = [4, 10])
      translate([0, 0, z])
        rotate([90, 0, 0])
          cylinder(h = 20, d = 3.3, center = true);
  }
}

module grip_sleeve() {
  // Print: large cylinder end on the bed. TPU 95A.
  difference() {
    union() {
      cylinder(h = 30, d = 12);
      translate([0, 0, 26])
        cylinder(h = 4, d1 = 12, d2 = 13.5);
    }
    translate([0, 0, -0.2])
      cylinder(h = 31, d = 8.2);
  }
}

module bellows() {
  // Print: snap-bead end (Z=0) on the bed. TPU 95A.
  difference() {
    union() {
      for (i = [0:4])
        translate([0, 0, i * 3.6])
          cylinder(h = 2.4, d = 20);
      cylinder(h = 18, d = 16);
    }
    translate([0, 0, -0.2])
      cylinder(h = 20, d = 9.2);
    // snap bead into cover groove
    translate([0, 0, -0.01])
      difference() {
        cylinder(h = 1.3, d = 20.2);
        cylinder(h = 1.4, d = 17.8);
      }
  }
}

module tpu_foot() {
  // Print: large face on the bed. TPU 95A. Need four.
  cylinder(h = 2.2, d = 14);
}

module p_clip() {
  // Print: pad on the bed. PETG. TPU 95A optional for extra grip.
  // M3 through the chassis floor hole; nut on the underside.
  // Hole at origin; loop toward −Y (basin). 7 mm ID for 3.5 mm TS/TRS.
  // Mouth on +Z so the cord drops in.
  id = 7.0;
  th = 2.2;
  w = 8.0;
  pad_h = 2.4;
  cy = -(id / 2 + th + 2.5);

  difference() {
    union() {
      hull() {
        cylinder(h = pad_h, d = 8.0);
        translate([0, cy, 0])
          cylinder(h = pad_h, d = w);
      }
      translate([0, cy, pad_h + id / 2])
        rotate([0, 90, 0])
          cylinder(h = w, d = id + 2 * th, center = true);
    }
    translate([0, cy, pad_h + id / 2])
      rotate([0, 90, 0])
        cylinder(h = w + 0.4, d = id, center = true);
    translate([-w / 2 - 0.2, cy - id / 2, pad_h + id / 2])
      cube([w + 0.4, id, id + th + 1]);
    translate([0, 0, -0.2])
      m3_clearance(h = pad_h + id + th + 1);
  }
}

module aa_2x_shim() {
  // Print: large face on the bed. PETG. Wedge on the +Y side of the AA well
  // so a 2x AA holder locates against the −Y wall. Omit when using 3x AA.
  difference() {
    rounded_box(aa_shim, 1.0);
    // finger scoop to pull the shim out
    translate([aa_shim[0] / 2, aa_shim[1] + 0.2, aa_shim[2] - 4])
      rotate([90, 0, 0])
        cylinder(h = 6, d = 10);
  }
  // locating ribs that bear on the 2x holder (+Y face of the nest)
  for (z = [4, 11])
    translate([-0.6, -1.4, z])
      cube([aa_shim[0] + 1.2, 1.6, 2.2]);
}

module magnet_plug() {
  // Print: flange (Ø8) on the bed. TPU 95A. Method 4a only.
  union() {
    cylinder(h = 1.2, d = 8.0);
    cylinder(h = 3.4, d = 5.9);
  }
}

module module_plate_holes(s, h) {
  translate([
    (s[0] - module_holes[0]) / 2,
    (s[1] - module_holes[1]) / 2,
    -0.2
  ]) {
    for (px = [0, module_holes[0]])
      for (py = [0, module_holes[1]])
        translate([px, py, 0])
          cylinder(h = h, d = 2.7);
  }
}

module module_header_cut(s) {
  translate([-0.2, s[1] / 2 - 10.16, 1.4])
    cube([3.0, 20.32, 3]);
}

module module_id_cut(s) {
  // ID resistor: pin 8 (ID, +Y end of header) to pin 2 (GND).
  translate([0, s[1] / 2 + 8.9, 0])
    module_id_resistor_cut();
}

// --- Method 3 ---

module mod3_switch_plate() {
  // Print: plate floor on the bed. ID resistor 10k to GND.
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       3.2];
  difference() {
    rounded_box(s, 1.2);
    module_plate_holes(s, 5);
    // D2F pockets 12.8 x 6.5 x 5.8, 0.2 mm clearance, in-line +X
    // plungers face cam (−X). Hard switch 1.6 mm further from cam.
    for (pair = [[10.0, 0], [10.0 + 12.8 + 1.6, 0]]) {
      translate([pair[0], s[1] / 2 - 3.35, 1.2])
        cube([13.0, 6.7, 6]);
      // 2.2 x 8 mm M2 slots
      translate([pair[0] + 3.2, s[1] / 2, -0.2])
        rotate([0, 0, 90])
          slot_xy(8, 2.2, 5);
      translate([pair[0] + 9.6, s[1] / 2, -0.2])
        rotate([0, 0, 90])
          slot_xy(8, 2.2, 5);
    }
    module_header_cut(s);
    module_id_cut(s);
  }
}

module mod3_ramp_cam() {
  // Print: large 14 x 8 face on the bed. Optional first-print extra.
  difference() {
    union() {
      cube([14, 8, 4], center = true);
      translate([2, 0, -1.2])
        rotate([0, 10, 0])
          cube([12, 8, 2.4], center = true);
      translate([5, 0, 0.4])
        cube([1.5, 8, 2], center = true); // 1.5 mm flat
    }
    for (y = [-2.5, 2.5])
      translate([-4, y, -3])
        cylinder(h = 8, d = 2.2);
  }
}

// --- Method 2 ---

module mod2_anvil() {
  // Print: plate floor on the bed. ID resistor 22k to GND.
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       8];
  difference() {
    rounded_box(s, 1.2);
    module_plate_holes(s, 10);
    // FSR well Ø16 x 0.4 mm
    translate([s[0] / 2 + 4, s[1] / 2, s[2] - 0.4])
      cylinder(h = 0.6, d = 16);
    // tail channel 8 x 0.6, bend radius >= 5 mm
    translate([s[0] / 2 + 4, s[1] / 2 - 4, s[2] - 0.6])
      cube([s[0], 8, 0.7]);
    translate([s[0] - 6, s[1] / 2, s[2] - 0.6])
      cylinder(h = 0.7, d = 10); // generous tail bend
    // puck well 0.5 mm recess
    translate([s[0] / 2 + 4, s[1] / 2, s[2] - 0.9])
      cylinder(h = 0.55, d = 8.4);
    // retainer M2
    for (a = [45, 135, 225, 315])
      translate([s[0] / 2 + 4, s[1] / 2, -0.2])
        rotate([0, 0, a])
          translate([10, 0, 0])
            cylinder(h = 10, d = 2.2);
    module_header_cut(s);
    module_id_cut(s);
  }
}

module mod2_puck() {
  // Print: large face on the bed. TPU 95A.
  cylinder(h = 3.0, d = 8.0);
}

module mod2_retainer() {
  // Print: large face on the bed. 1.4 mm so it actually prints (was 0.5 mm).
  difference() {
    cylinder(h = 1.4, d = 22);
    translate([0, 0, -0.1])
      cylinder(h = 1.8, d = 8.6); // puck pass-through
    for (a = [45, 135, 225, 315])
      rotate([0, 0, a])
        translate([10, 0, -0.1])
          cylinder(h = 1.8, d = 2.2);
  }
}

// --- Method 4a ---

module mod4a_hall_tray() {
  // Print: plate floor on the bed. ID resistor 47k to GND.
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       6];
  difference() {
    rounded_box(s, 1.2);
    module_plate_holes(s, 8);
    // 8 mm slot for SS49E gap cal (M2)
    translate([s[0] / 2, s[1] / 2, -0.2])
      slot_xy(8, 2.2, 8);
    // TO-92 pocket 4.2 x 4.2 x 1.8, pins toward −Z
    translate([s[0] / 2 - 2.1, s[1] / 2 - 2.1, 2.4])
      cube([4.2, 4.2, 2.2]);
    translate([s[0] / 2 - 2.0, s[1] / 2 - 1.3, -0.2])
      cube([4.0, 2.6, 3.2]); // pin well
    module_header_cut(s);
    module_id_cut(s);
  }
}

module gap_gauge(t = 2) {
  // Print: large face on the bed. Calibration gauge, not a structural part.
  difference() {
    rounded_box([12, 30, t], 1.0);
    translate([6, 24, -0.1])
      cylinder(h = t + 0.2, d = 3.3);
  }
}

// --- Method 4b ---
// TAL220 is ~55 x 12.7 mm and does not fit the 40 x 32 mm module pocket.
// This saddle bolts to chassis M3 slots under the cam (later experiment).

module mod4b_cell_saddle() {
  // Print: large floor on the bed. ID resistor 100k to GND on the flying header.
  sx = 72;
  sy = 24;
  sz = 10;
  difference() {
    rounded_box([sx, sy, sz], 1.2);
    // TAL220 / YZC-133 well — caliper the cell before locking
    translate([sx / 2, sy / 2, 4])
      cube([56, 13.2, 13], center = true);
    // M3 holes, 48 mm spacing, match chassis m4b slots under the cam
    for (dx = [-m4b_dx, m4b_dx])
      translate([sx / 2 + dx, sy / 2, -0.2])
        m3_clearance(h = 12);
    // overload posts 0.4 mm above rest (printed bosses kept by not cutting here)
    module_header_cut([sx, sy, sz]);
    module_id_cut([sx, sy, sz]);
  }
  for (dx = [-8, 8])
    translate([sx / 2 + dx + 12, sy / 2, 8.0])
      cylinder(h = 3, d = 4);
}

module mod4b_ball_anvil() {
  // Print: large cylinder end on the bed.
  difference() {
    union() {
      cylinder(h = 6, d = 12);
      translate([0, 0, 6])
        cylinder(h = 3, d1 = 12, d2 = 8);
    }
    translate([0, 0, 7.2])
      sphere(d = 5.2); // Ø5 mm ball seat
    translate([0, 0, -0.2])
      cylinder(h = 5, d = 3.3);
  }
}

module feeler_0_4() {
  // Print: large face on the bed. 0.4 mm feeler gauge, not a structural part.
  rounded_box([10, 40, 0.4], 0.8);
}

module wear_pad() {
  // Print: large face on the bed.
  rounded_box([12, 6, 1.0], 0.6);
}

module kit_preview() {
  // Exploded first-print kit. Not exported as an STL.
  base_chassis();
  translate([0, 0, base_z + 14])
    cover_slot();
  translate([hinge_x, hinge_y, hinge_z + 48])
    rocker_arm();
  translate([hinge_x, hinge_y, hinge_z + 78])
    stick_collar();
  translate([hinge_x, hinge_y, hinge_z + 104])
    grip_sleeve();
  translate([hinge_x, hinge_y, hinge_z + 28])
    bellows();
  for (i = [0:3])
    translate([14 + (i % 2) * 22, 14 + floor(i / 2) * 22, -10])
      tpu_foot();
  translate([module_origin[0], module_origin[1], base_z + 22])
    mod3_switch_plate();
  translate([hinge_x + cam_r, hinge_y, hinge_z + 36])
    mod3_ramp_cam();
  translate([batt_origin[0] + 0.4, batt_origin[1] + aa_2x_w, base_z + 8])
    aa_2x_shim();
  translate([pclip_x, pclip_y, base_z + 10])
    p_clip();
  translate([usb_pclip_x, usb_pclip_y, base_z + 10])
    p_clip();
}

module render_part() {
  if (part == "base_chassis") base_chassis();
  else if (part == "cover_slot") cover_slot();
  else if (part == "rocker_arm") rocker_arm();
  else if (part == "stick_collar") stick_collar();
  else if (part == "grip_sleeve") grip_sleeve();
  else if (part == "bellows") bellows();
  else if (part == "tpu_foot") tpu_foot();
  else if (part == "p_clip") p_clip();
  else if (part == "aa_2x_shim") aa_2x_shim();
  else if (part == "magnet_plug") magnet_plug();
  else if (part == "mod3_switch_plate") mod3_switch_plate();
  else if (part == "mod3_ramp_cam") mod3_ramp_cam();
  else if (part == "mod2_anvil") mod2_anvil();
  else if (part == "mod2_puck") mod2_puck();
  else if (part == "mod2_retainer") mod2_retainer();
  else if (part == "mod4a_hall_tray") mod4a_hall_tray();
  else if (part == "gap_gauge_2") gap_gauge(2);
  else if (part == "gap_gauge_3") gap_gauge(3);
  else if (part == "gap_gauge_4") gap_gauge(4);
  else if (part == "gap_gauge_5") gap_gauge(5);
  else if (part == "gap_gauge_6") gap_gauge(6);
  else if (part == "mod4b_cell_saddle") mod4b_cell_saddle();
  else if (part == "mod4b_ball_anvil") mod4b_ball_anvil();
  else if (part == "feeler_0_4") feeler_0_4();
  else if (part == "wear_pad") wear_pad();
  else if (part == "kit_preview") kit_preview();
  else assert(false, "unknown part");
}

render_part();
