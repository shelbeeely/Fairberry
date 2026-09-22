// Xteink X4 preset for the Fairberry case generator.
//
// STILL PARTIALLY UNVERIFIED, but better grounded than a plain spec-sheet
// guess: PHONE_WIDTH/PHONE_THICKNESS below come from a community-made,
// to-scale Xteink X4 dummy model (measured off a physical unit, not just
// the rounded marketing spec of 114 x 69 x 5.9mm):
//   "Xteink X4 device model" by Zorian22, CC BY-NC-SA
//   https://www.thingiverse.com/thing:7287950
//   (companion case: https://www.thingiverse.com/thing:7287943 ;
//    parametric Onshape source: see the Thingiverse page for the link)
// That model measures 114.2 x 69.2 x 6.2mm at its widest/thickest points.
//
// This preset still doesn't use that model directly (see PHONE_DUMMY_FILE
// below) -- it only borrows its measured width/thickness for the
// parametric (no-dummy-file) case body. PHONE_KEYBOARD_OVERLAP and
// USB_PORT_Y_OFFSET are still unverified guesses, and side page-turn
// button cutouts still aren't modeled at all.
//
// For a real fit instead of an approximation:
// 1. Download the .3mf from the Thingiverse link above yourself (Claude
//    can't do this step -- Thingiverse's terms explicitly block automated
//    fetching, so this needs a normal browser download).
// 2. Convert it to .stl if needed (OpenSCAD's import() wants stl/off/amf
//    for this repo's existing convention; recent OpenSCAD can import .3mf
//    directly, but converting keeps this consistent with the phone
//    presets) and save it as Case/Generator/import/XteinkX4_Dummy.stl.
// 3. Set PHONE_DUMMY_FILE below to "import/XteinkX4_Dummy.stl" instead of
//    "". This switches Fairberry.scad to cut the case around the actual
//    modeled device shape instead of a plain rounded box, which should
//    also surface the button geometry that model includes.
// Either way: print a bottom-only test fit before committing to a full
// case -- these numbers have not been validated against my own unit yet.

//Phone dimensions in mm
PHONE_WIDTH=69.2;
PHONE_THICKNESS=6.2;
PHONE_KEYBOARD_OVERLAP=8; // Unverified guess, adjust to your device
USB_PORT_Y_OFFSET=0; // Assumed centered, verify against your unit

GENERATE_TOP_CASE=false; // If set to false, will generate the keyboard case. If set to true, will instead create the top case

// Leave empty ("") to generate the phone shape using this script.
// See step 1-3 above for switching this to the real dummy model.
PHONE_DUMMY_FILE="";

// The X4 has no speaker. Fairberry.scad always cuts this slot when
// PHONE_DUMMY_FILE=="" (the cutout is a fixed-radius hull, so setting the
// width to 0 does NOT remove it, it just leaves a ~4mm slot) — it will show
// up as a small non-functional vent near the USB cutout. Harmless, but if
// you want a fully solid case bottom there, remove the speakerHole() call
// from phoneDummy() in Fairberry.scad.
PHONE_SPEAKER_CUTAWAY_WIDTH=4;
PHONE_SPEAKER_CUTAWAY_DISTANCE_FROM_CENTER=18;

// Corner radius of the device. Only relevant since PHONE_DUMMY_FILE==""
PHONE_ROUNDING_RADIUS=6;

PERIMETERS=4; // How many perimeters should the top part of the case be thick?

// How many perimeters should the case be thick below the device?
CASE_BOTTOM_PERIMETERS=3;

CASE_SLEEVE_TOP_PERIMETERS=2;
CASE_SLEEVE_LENGTH=28.5; // Length of the case extension towards the top of the device
CASE_SLEEVE_LINK_LENGTH=5;
CASE_SLEEVE_ROUNDING_RADIUS=4; // Corner radius of the sleeve part of the case.
CASE_ROUNDING_RADIUS=4; // Corner radius of the case. Must stay well below PHONE_THICKNESS since the X4 is thin.
CASE_BOTTOM_ROUNDING_RADIUS=10;
CASE_TOP_ROUNDING_OFFSET=1; // Increasing this number makes the top less rounded

KEYBOARD_PHONE_GAP_PERIMETERS=2; // Thickness of the separator between keyboard and device screen

KEYBOARD_CATCH_DEPTH=1.5;

FORCE_HOLE_CUTAWAYS=true; // Will cut holes for USB even if a dummy file is selected. Use this if the holes aren't included in the dummy file.
