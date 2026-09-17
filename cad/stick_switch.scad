// Stick-Switch shared single-axis chassis and sensor modules.
// Generate one STL:  openscad -D 'part="base_chassis"' -o cad/stl/base_chassis.stl cad/stick_switch.scad
//
// Coordinate system (base origin = rear-left-bottom of chassis):
//   +X forward (student push / toward sensor bay)
//   +Y left-to-right
//   +Z up (stick)
// Hinge axis is +Y through [hinge_x, base_y/2, hinge_z].

include <lib.scad>

part = "base_chassis"; // see render_part() at bottom

base_x = 90;
base_y = 70;
base_z = 24;
wall = 2.4;
r_ext = 2.0;

hinge_x = 28;
hinge_z = 18;
cam_r = 25;
grip_r = 110;
stick_od = 8.0;
stick_slot_y = 8.4;
stick_slot_x = 12.0;

module_pocket = [40, 32, 18];
module_clear = 0.2;
module_holes = [34, 26];
module_origin = [hinge_x + 8, (base_y - module_pocket[1]) / 2, wall];

cover_z = 4.0;
insert_1_4_20_d = 8.2;
insert_1_4_20_h = 10.0;

esp32 = [55, 29, 13];

// --- chassis ---

module base_chassis() {
  difference() {
    union() {
      rounded_box([base_x, base_y, base_z], r_ext);
      // hinge towers
      for (s = [-1, 1])
        translate([hinge_x, base_y / 2 + s * 18, 0])
          rounded_box([16, 10, hinge_z + 7], 1.5);
    }

    // interior basin
    translate([wall, wall, wall])
      rounded_box([base_x - 2 * wall, base_y - 2 * wall, base_z], 1.2);

    // module pocket (open to interior, registered to cam)
    translate([module_origin[0], module_origin[1], module_origin[2]])
      cube([module_pocket[0], module_pocket[1], module_pocket[2] + 2]);

    // module M2.5 inserts (34 x 26 mm)
    translate([
      module_origin[0] + (module_pocket[0] - module_holes[0]) / 2,
      module_origin[1] + (module_pocket[1] - module_holes[1]) / 2,
      wall - 0.1
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0])
            m25_insert(h = 5.2);
    }

    // 8-pin 2.54 mm header well at rear of pocket: 3V3 GND SOFT HARD ADC SDA SCL ID
    translate([
      module_origin[0] - 3.2,
      module_origin[1] + module_pocket[1] / 2 - 10.8,
      wall + 2
    ])
      cube([4.0, 21.6, 10]);

    // ESP32 DevKit tray (USB toward -X / rear)
    translate([wall + 1.5, (base_y - esp32[1]) / 2, wall])
      cube([esp32[0], esp32[1], esp32[2] + 6]);
    // USB cutout
    translate([-0.2, base_y / 2 - 6, wall + 2])
      cube([wall + 2, 12, 8]);

    // 3.5 mm jack well (SJ1-3513N), +Y wall, tip toward outside
    translate([base_x - 22, base_y - wall - 0.2, 8])
      rotate([-90, 0, 0])
        cylinder(h = wall + 6, d = 6.2);

    // P-clip / strain-relief M3
    translate([base_x - 10, wall + 6, -0.1])
      m3_clearance(h = wall + 2);

    // 1/4-20 brass insert, underside center
    translate([base_x / 2, base_y / 2, -0.1])
      cylinder(h = insert_1_4_20_h, d = insert_1_4_20_d);

    // 4x M4 tray slots, 20 mm, near corners
    for (px = [12, base_x - 12])
      for (py = [10, base_y - 10])
        translate([px, py, -0.1])
          rotate([0, 0, 90])
            m4_slot(length = 20, h = wall + 2);

    // cover M3 inserts (4)
    for (px = [8, base_x - 8])
      for (py = [8, base_y - 8])
        translate([px, py, base_z - 6])
          m3_insert(h = 6.2);

    // hinge through-holes and 623ZZ seats (outer faces)
    translate([hinge_x, -0.2, hinge_z])
      rotate([-90, 0, 0])
        cylinder(h = base_y + 0.4, d = 3.3);
    translate([hinge_x, 4.0, hinge_z])
      rotate([-90, 0, 0])
        bearing_623zz_seat();
    translate([hinge_x, base_y - 4.0, hinge_z])
      rotate([90, 0, 0])
        bearing_623zz_seat();

    // torsion-spring preload holes in left tower: 0 / 30 / 60 deg
    for (a = [0, 30, 60])
      translate([hinge_x, 8, hinge_z])
        rotate([90, a, 0])
          translate([7.5, 0, 0])
            cylinder(h = 8, d = 1.8);

    // cable exit + 40 mm service-loop channel along -X wall
    translate([wall - 0.2, 18, wall + 2])
      cube([wall + 1, 8, 6]);
  }

  // module pocket floor (thin, with pocket already cut)
  translate([module_origin[0], module_origin[1], wall])
    difference() {
      cube([module_pocket[0], module_pocket[1], 1.6]);
      translate([
        (module_pocket[0] - module_holes[0]) / 2,
        (module_pocket[1] - module_holes[1]) / 2,
        -0.2
      ]) {
        for (px = [0, module_holes[0]])
          for (py = [0, module_holes[1]])
            translate([px, py, 0])
              cylinder(h = 2.2, d = 2.6);
      }
    }
}

module cover_slot() {
  difference() {
    union() {
      rounded_box([base_x, base_y, cover_z], r_ext);
      // bellows groove ring around stick slot
      translate([hinge_x, base_y / 2, cover_z])
        cylinder(h = 2.2, d = 22);
    }

    // stick slot: 8.4 mm Y (side-load reject), 12 mm X (travel)
    translate([hinge_x, base_y / 2, -0.2])
      hull() {
        translate([-stick_slot_x / 2 + stick_slot_y / 2, 0, 0])
          cylinder(h = cover_z + 4, d = stick_slot_y);
        translate([stick_slot_x / 2 - stick_slot_y / 2, 0, 0])
          cylinder(h = cover_z + 4, d = stick_slot_y);
      }

    // bellows groove (1.2 mm channel)
    translate([hinge_x, base_y / 2, cover_z + 0.6])
      difference() {
        cylinder(h = 1.4, d = 20.4);
        cylinder(h = 1.5, d = 17.6);
      }

    // M3 cover screws
    for (px = [8, base_x - 8])
      for (py = [8, base_y - 8])
        translate([px, py, -0.1]) {
          m3_clearance(h = cover_z + 3);
          translate([0, 0, cover_z - 1.6])
            countersink_m3(h = 2.2);
        }

    // M3 hard-stop set screw, 25 mm forward of hinge (cam radius)
    translate([hinge_x + cam_r - 2, base_y / 2, -0.1]) {
      cylinder(h = cover_z + 4, d = 2.5); // tap M3
      translate([0, 0, cover_z - 0.4])
        cylinder(h = 1.2, d = 5.5); // hex access
    }

    // access window over ESP32 USB / EN
    translate([4, base_y / 2 - 10, -0.1])
      rounded_box([18, 20, cover_z + 1], 1.5);
  }
}

module rocker_arm() {
  arm_y = 8;
  difference() {
    union() {
      // hub
      translate([0, 0, 0])
        rotate([90, 0, 0])
          cylinder(h = arm_y, d = 16, center = true);
      // stick boss (+Z)
      translate([0, 0, 0])
        cylinder(h = 16, d = 16);
      // cam arm (+X), 12 x 8 cam face at 25 mm
      hull() {
        rotate([90, 0, 0])
          cylinder(h = arm_y, d = 12, center = true);
        translate([cam_r, 0, -2])
          cube([12, arm_y, 8], center = true);
      }
      // 10 deg ramp + 1.5 mm flat on cam underside (presses -Z onto module)
      translate([cam_r - 2, 0, -6])
        rotate([0, 10, 0])
          cube([14, 8, 3], center = true);
    }

    // M3 hinge bore
    rotate([90, 0, 0])
      cylinder(h = 20, d = 3.3, center = true);

    // 623ZZ seats both sides
    translate([0, arm_y / 2 - 0.05, 0])
      rotate([-90, 0, 0])
        bearing_623zz_seat(depth = 1.2); // hub is thin; bearings live in towers
    translate([0, -arm_y / 2 + 0.05, 0])
      rotate([90, 0, 0])
        bearing_623zz_seat(depth = 1.2);

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
  cylinder(h = 2.2, d = 14);
}

module magnet_plug() {
  union() {
    cylinder(h = 1.2, d = 8.0);
    cylinder(h = 3.4, d = 5.9);
  }
}

// --- Method 3 ---

module mod3_switch_plate() {
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       3.2];
  difference() {
    rounded_box(s, 1.2);
    // M2.5 module holes
    translate([
      (s[0] - module_holes[0]) / 2,
      (s[1] - module_holes[1]) / 2,
      -0.2
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0])
            cylinder(h = 5, d = 2.7);
    }
    // D2F pockets 12.8 x 6.5 x 5.8, 0.2 mm clearance, in-line +X
    // plungers face cam (-X). Hard switch 1.6 mm further from cam.
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
    // 8-pin header pads
    translate([-0.2, s[1] / 2 - 10.16, 1.4])
      cube([3.0, 20.32, 3]);
  }
}

module mod3_ramp_cam() {
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
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       8];
  difference() {
    rounded_box(s, 1.2);
    translate([
      (s[0] - module_holes[0]) / 2,
      (s[1] - module_holes[1]) / 2,
      -0.2
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0])
            cylinder(h = 10, d = 2.7);
    }
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
  }
}

module mod2_puck() {
  cylinder(h = 3.0, d = 8.0);
}

module mod2_retainer() {
  difference() {
    cylinder(h = 0.5, d = 22);
    translate([0, 0, -0.1])
      cylinder(h = 0.8, d = 8.6); // puck pass-through
    for (a = [45, 135, 225, 315])
      rotate([0, 0, a])
        translate([10, 0, -0.1])
          cylinder(h = 0.8, d = 2.2);
  }
}

// --- Method 4a ---

module mod4a_hall_tray() {
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       6];
  difference() {
    rounded_box(s, 1.2);
    translate([
      (s[0] - module_holes[0]) / 2,
      (s[1] - module_holes[1]) / 2,
      -0.2
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0])
            cylinder(h = 8, d = 2.7);
    }
    // 8 mm slot for SS49E gap cal (M2)
    translate([s[0] / 2, s[1] / 2, -0.2])
      slot_xy(8, 2.2, 8);
    // TO-92 pocket 4.2 x 4.2 x 1.8, pins toward -Z
    translate([s[0] / 2 - 2.1, s[1] / 2 - 2.1, 2.4])
      cube([4.2, 4.2, 2.2]);
    translate([s[0] / 2 - 2.0, s[1] / 2 - 1.3, -0.2])
      cube([4.0, 2.6, 3.2]); // pin well
  }
}

module gap_gauge(t = 2) {
  difference() {
    rounded_box([12, 30, t], 1.0);
    translate([6, 24, -0.1])
      cylinder(h = t + 0.2, d = 3.3);
  }
}

// --- Method 4b ---

module mod4b_cell_saddle() {
  s = [module_pocket[0] - module_clear,
       module_pocket[1] - module_clear,
       10];
  difference() {
    rounded_box(s, 1.2);
    translate([
      (s[0] - module_holes[0]) / 2,
      (s[1] - module_holes[1]) / 2,
      -0.2
    ]) {
      for (px = [0, module_holes[0]])
        for (py = [0, module_holes[1]])
          translate([px, py, 0])
            cylinder(h = 12, d = 2.7);
    }
    // TAL220 / YZC-133 live in 8 mm slots — caliper the cell before locking
    translate([s[0] / 2, s[1] / 2, 4])
      cube([56, 13.2, 13], center = true);
    for (dx = [-22, 22])
      translate([s[0] / 2 + dx, s[1] / 2, -0.2])
        rotate([0, 0, 90])
          slot_xy(8, 4.3, 12);
    // overload posts 0.4 mm above rest
    for (dx = [-8, 8])
      translate([s[0] / 2 + dx + 12, s[1] / 2, 8.0])
        cylinder(h = 3, d = 4);
  }
}

module mod4b_ball_anvil() {
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
  rounded_box([10, 40, 0.4], 0.8);
}

module wear_pad() {
  rounded_box([12, 6, 1.0], 0.6);
}

module render_part() {
  if (part == "base_chassis") base_chassis();
  else if (part == "cover_slot") cover_slot();
  else if (part == "rocker_arm") rocker_arm();
  else if (part == "stick_collar") stick_collar();
  else if (part == "grip_sleeve") grip_sleeve();
  else if (part == "bellows") bellows();
  else if (part == "tpu_foot") tpu_foot();
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
  else assert(false, "unknown part");
}

render_part();
