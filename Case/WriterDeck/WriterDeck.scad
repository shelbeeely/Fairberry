// Writer Deck combined case -- Fairberry keyboard + Xteink X4, one clamshell.
// See Case/WriterDeck/README.md for the full reasoning and what's still
// unverified here.
//
// STRATEGY: don't reinvent the hinge. The user-provided reference model
// (Case/WriterDeck/reference/, an "XteinkX4FullCoverageFlipCase") has a
// real, evidently-tested 3-piece interlocking hinge:
//   - BOTTOM: the X4 cradle (27590 triangles -- has the device-specific
//     port/button cutouts). Reused here completely unmodified.
//   - HINGE: a separate comb+pin bar, X 15.06-50.86mm, that meshes with
//     knuckle teeth on both BOTTOM and the (replaced) TOP. Reused
//     unmodified.
//   - TOP: in the reference, a plain flip-lid (11760 triangles, much
//     simpler than BOTTOM -- no device-specific cutouts). This is what
//     gets replaced with the keyboard half below. Only TOP's two corner
//     hinge-knuckle loops are reused (extracted via intersection(), see
//     hingeKnuckleLoops() below) -- the flat lid body itself is discarded
//     and replaced with the keyboard tray.
//
// Coordinate system matches the reference files' own: this draws the
// CLOSED position (keyboard half stacked on top of the X4 cradle, same
// footprint), matching how BOTTOM/TOP were modeled relative to each other
// in the original. The two halves are meant to be printed in this
// relative arrangement, then the hinge pin threaded through after
// printing -- same assembly process the reference case itself would use.
//
// UNVERIFIED (see README): keyboard tray cavity depth/height for the
// FAIRBERRY_ESP32S3_SMART board + battery + trackball stack (no PCB
// exists yet to measure against -- see Documentation/Hardware_Standalone_
// Smart_Keyboard.md's own "no enclosure yet" note, which still applies),
// trackball/mic/speaker cutout positions, wall thickness/hinge alignment
// tolerance (not print-tested).

$fn = $preview ? 24 : 60;

// ---- Reused reference geometry (unmodified) ----

module x4Cradle() {
  import("reference/xteinkflipcaseBOTTOM.stl", convexity=6);
}

module hingePin() {
  import("reference/xteinkflipcaseHINGE.stl", convexity=6);
}

// Extracts just the two corner hinge-knuckle loops from the reference
// TOP piece (bounding boxes measured directly off the mesh -- see the
// analysis in this repo's session history, not eyeballed). The rest of
// TOP (the flat lid body) is intentionally discarded.
module hingeKnuckleLoops() {
  intersection() {
    import("reference/xteinkflipcaseTOP.stl", convexity=6);
    union() {
      translate([-4, 104, 5.5]) cube([14, 14, 4]);   // left loop
      translate([59, 104, 5.5]) cube([12, 14, 4]);   // right loop
    }
  }
}

// ---- New keyboard half ----
// Same X-Y footprint as the X4 cradle (0-70 x 0-~117) so the extracted
// knuckle loops land in the right place and the two halves stack/close
// flush, matching the reference's own BOTTOM/TOP relationship.

TRAY_WIDTH = 70;
TRAY_LENGTH = 117;
TRAY_WALL = 2;                 // UNVERIFIED -- not tuned against any specific printer/tolerance yet
TRAY_FLOOR_Z = 6.0;            // Sits where the reference's TOP started (see file header) -- keeps knuckle loops aligned without needing to re-derive their Z position
KEYBOARD_CAVITY_HEIGHT = 8;    // UNVERIFIED -- rough guess for keyboard module (5.2mm, per Case/Generator/import/BBQ10KBD.stl) + mainboard PCB + clearance. No PCB exists yet for FAIRBERRY_ESP32S3_SMART to measure against; see Documentation/Hardware_Standalone_Smart_Keyboard.md

// Real, measured keyboard module dimensions (Case/Generator/import/BBQ10KBD.stl: 66.558 x 36.044 x 5.225mm), not guessed.
KEYBOARD_MODULE_WIDTH = 66.6;
KEYBOARD_MODULE_LENGTH = 36.1;

// Placed with its far (non-hinge) edge near Y=8 and extending toward the
// hinge, leaving room between the keyboard and the hinge edge for the
// trackball -- BlackBerry-style, trackball between the keys and the
// screen so it's naturally reachable while looking at the display when
// the deck is open. Tune KEYBOARD_Y_OFFSET to adjust.
KEYBOARD_Y_OFFSET = 8;

module keyboardCutout() {
  translate([(TRAY_WIDTH - KEYBOARD_MODULE_WIDTH)/2, KEYBOARD_Y_OFFSET, -1])
    cube([KEYBOARD_MODULE_WIDTH, KEYBOARD_MODULE_LENGTH, KEYBOARD_CAVITY_HEIGHT + 2]);
}

// Trackball module footprint (ICSH044A / SparkFun-clone, ~28x22x8mm per
// Documentation/Hardware_ESP32_Trackball_Mainboard.md's BOM) -- placed
// near the hinge edge, centered, per the BlackBerry-style reasoning
// above. Position is a first guess, not fitted to anything.
TRACKBALL_WIDTH = 28;
TRACKBALL_LENGTH = 22;
TRACKBALL_Y = TRAY_LENGTH - 30; // 30mm in from the hinge edge

module trackballCutout() {
  translate([(TRAY_WIDTH - TRACKBALL_WIDTH)/2, TRACKBALL_Y, -1])
    cube([TRACKBALL_WIDTH, TRACKBALL_LENGTH, KEYBOARD_CAVITY_HEIGHT + 2]);
}

module keyboardTrayShell() {
  // Open-top tray: walls + floor only, no lid (electronics drop in from
  // above, same idea as the existing Fairberry.scad case). The inner cut
  // deliberately overshoots the top face so it's a clean through-cut
  // rather than stopping short and leaving an accidental thin ceiling.
  translate([0, 0, TRAY_FLOOR_Z])
  difference() {
    cube([TRAY_WIDTH, TRAY_LENGTH, KEYBOARD_CAVITY_HEIGHT + TRAY_WALL]);
    translate([TRAY_WALL, TRAY_WALL, TRAY_WALL])
      cube([TRAY_WIDTH - 2*TRAY_WALL, TRAY_LENGTH - 2*TRAY_WALL, KEYBOARD_CAVITY_HEIGHT + 10]);
  }
}

module keyboardHalf() {
  union() {
    difference() {
      keyboardTrayShell();
      translate([0, 0, TRAY_FLOOR_Z]) keyboardCutout();
      translate([0, 0, TRAY_FLOOR_Z]) trackballCutout();
    }
    hingeKnuckleLoops();
  }
}

// ---- Assembly (closed position) ----

color("SteelBlue") x4Cradle();
color("DarkGray") hingePin();
color("Orange") keyboardHalf();
