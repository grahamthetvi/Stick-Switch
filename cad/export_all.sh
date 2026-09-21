#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/stl"
mkdir -p "$OUT"
# Printable STLs. kit_preview is an exploded assembly (not exported).
# First classroom print: base_chassis cover_slot rocker_arm stick_collar
#   grip_sleeve bellows tpu_foot p_clip mod3_switch_plate [mod3_ramp_cam].
PARTS=(
  base_chassis cover_slot rocker_arm stick_collar
  grip_sleeve bellows tpu_foot p_clip magnet_plug aa_2x_shim
  mod3_switch_plate mod3_ramp_cam
  mod2_anvil mod2_puck mod2_retainer
  mod4a_hall_tray gap_gauge_2 gap_gauge_3 gap_gauge_4 gap_gauge_5 gap_gauge_6
  mod4b_cell_saddle mod4b_ball_anvil feeler_0_4 wear_pad
)
export_one() {
  local p="$1"
  echo "export $p"
  if command -v xvfb-run >/dev/null 2>&1; then
    xvfb-run -a openscad -q -D "part=\"$p\"" -o "$OUT/$p.stl" "$ROOT/stick_switch.scad"
  else
    openscad -q -D "part=\"$p\"" -o "$OUT/$p.stl" "$ROOT/stick_switch.scad"
  fi
}
for p in "${PARTS[@]}"; do
  export_one "$p"
done
echo "wrote ${#PARTS[@]} STLs to $OUT"
