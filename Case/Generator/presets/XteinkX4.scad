// Xteink X4 preset for the Fairberry case generator.
//
// UNVERIFIED: these numbers come from the X4's published spec sheet
// (114 x 69 x 5.9mm body, USB-C port), not from a teardown, CAD file, or
// physical test fit. There is no dummy STL for the X4 in this repo, so the
// case body is generated parametrically from PHONE_WIDTH/PHONE_THICKNESS
// below (see the PHONE_DUMMY_FILE="" path in Fairberry.scad).
//
// Before committing to a full print, verify/adjust at least:
// - PHONE_KEYBOARD_OVERLAP: how far the keyboard should overlap the bottom
//   bezel of the screen. Set from a real measurement of your unit's bezel.
// - USB_PORT_Y_OFFSET: how far off-center (in the thickness direction) the
//   USB-C port sits. Assumed centered here since the body is very thin.
// - This case does not model or cut out the X4's side page-turn buttons.
//   If your case covers them, you'll need to add cutouts by hand.

//Phone dimensions in mm
PHONE_WIDTH=69;
PHONE_THICKNESS=5.9;
PHONE_KEYBOARD_OVERLAP=8; // Unverified guess, adjust to your device
USB_PORT_Y_OFFSET=0; // Assumed centered, verify against your unit

GENERATE_TOP_CASE=false; // If set to false, will generate the keyboard case. If set to true, will instead create the top case

// Leave empty ("") to generate the phone shape using this script
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
