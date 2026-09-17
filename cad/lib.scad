// Shared helpers for Stick-Switch printable parts.
// Units: millimetres. External edges aim for R >= 2 mm.

$fn = 64;

module rounded_box(size, r = 2, center = false) {
  x = size[0];
  y = size[1];
  z = size[2];
  rr = min(r, x / 2 - 0.05, y / 2 - 0.05);
  o = center ? [-x / 2, -y / 2, -z / 2] : [0, 0, 0];
  translate(o)
    hull() {
      for (px = [rr, x - rr])
        for (py = [rr, y - rr])
          translate([px, py, 0])
            cylinder(h = z, r = rr);
    }
}

module slot_xy(length, width, height) {
  hull() {
    translate([-length / 2 + width / 2, 0, 0])
      cylinder(h = height, d = width);
    translate([length / 2 - width / 2, 0, 0])
      cylinder(h = height, d = width);
  }
}

module countersink_m3(h = 3.5) {
  cylinder(h = h + 0.2, d = 6.2);
}

module m3_clearance(h = 20) {
  cylinder(h = h, d = 3.3);
}

module m3_insert(h = 6.0) {
  // 0.1 mm over typical short M3 heat-set insert OD ~4.0 mm
  cylinder(h = h, d = 4.1);
}

module m25_insert(h = 5.0) {
  cylinder(h = h, d = 3.6);
}

module m2_clearance(h = 12) {
  cylinder(h = h, d = 2.2);
}

module m4_slot(length = 20, h = 12) {
  slot_xy(length, 4.3, h);
}

module bearing_623zz_seat(depth = 4.2) {
  cylinder(h = depth, d = 10.1, $fn = 72);
}

module through(h, d) {
  translate([0, 0, -0.2])
    cylinder(h = h + 0.4, d = d);
}

// Two 0.9 mm holes, 7.62 mm pitch, for the module ID resistor (TH 1/4 W).
// Solder between header pin 8 (ID) and pin 2 (GND).
module module_id_resistor_cut() {
  translate([4.0, 0, -0.2]) {
    cylinder(h = 6, d = 0.9);
    translate([0, -7.62, 0])
      cylinder(h = 6, d = 0.9);
  }
}
