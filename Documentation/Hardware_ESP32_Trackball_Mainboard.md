# ESP32 mainboard + trackball: design notes

This documents adding an ICSH044A (SparkFun BlackBerry Trackballer Breakout clone, COM-09320-style) five-way trackball to a BLE-first Fairberry keyboard, wired up alongside the existing BBQ10 keyboard matrix on `BOARD_TYPE ESP32`.

**Status: this is an architecture proposal and a working firmware implementation, not a fabricated or tested board.** There's no KiCad schematic/PCB for this yet -- I don't have a way to lay out and verify a PCB in this environment (no KiCad GUI, no way to run ERC/DRC, and definitely no way to test on real hardware), so producing one blind would risk handing you something that looks finished but silently doesn't work. What's real and done: the pin plan below, and the firmware in `BBQ10/trackball.h` (gated behind `TRACKBALL_ENABLED` in `configuration.h`, off by default). What's still open: turning this into an actual schematic/PCB, and validating any of it against hardware.

## Why keep the classic ESP32 (not C3/S3) for this

The existing `BOARD_TYPE ESP32` code already targets the classic ESP32 specifically (`esp_pm_config_esp32_t`, `esp_bt_sleep_enable()`, etc. are chip-specific API names that wouldn't compile as-is on C3/S3 without changes). Reusing it instead of jumping to C3/S3 avoids introducing an untested chip-porting problem on top of an already-not-fully-tested board type. It also has a real upside for "work with anything that supports the wireless protocols": classic ESP32 has both BLE and Bluetooth Classic radios, where C3/S3 are BLE-only -- more headroom for compatibility later, even though this firmware only uses BLE HID today via the `BleKeyboard` library. It also has more usable GPIO than C3, which matters below.

## Pin plan

Reuses the existing `BOARD_TYPE ESP32` keyboard matrix pins unchanged (`boards.h`):

| Function | Pins |
|---|---|
| Keyboard rows (7) | 27, 25, 32, 4, 0, 2, 22 |
| Keyboard cols (5) | 5, 23, 19, 18, 26 |
| Reset | 33 |

New/changed for the trackball:

| Function | Pin | Notes |
|---|---|---|
| Keyboard backlight | **13** (was 35) | The existing pin map had this on GPIO35, one of the ESP32's 4 input-only ADC pins (34/35/36/39) -- no output driver exists on those at all, so backlight PWM could never have worked there. Moved to a normal I/O pin; this also frees 35 for the trackball below. |
| Trackball UP | 34 | Input-only pin. **Needs an external pull-up resistor to 3.3V** (e.g. 10k) -- these pins have no internal pull-up/down hardware, and whether the ICSH044A clone board has its own onboard pull-ups wasn't reliably confirmed during research. Add the resistor regardless; redundant is harmless, missing is not. |
| Trackball DOWN | 36 | Same as above. Silkscreened `SVP`/`VP` or similar on some devkits -- check your board's labeling, it's the same physical GPIO36. |
| Trackball LEFT | 39 | Same as above. Sometimes silkscreened `SVN`/`VN`. |
| Trackball RIGHT | 35 | Same as above (this is the pin freed from the backlight fix). |
| Trackball BTN | 14 | Normal I/O pin, uses the ESP32's internal pull-up (`INPUT_PULLUP` in firmware) -- no external resistor needed. Only 4 input-only pins exist on this chip and all 4 went to the direction lines, so BTN landed on a regular pin instead. |
| Trackball LED RED | 16 | Optional, only wired if `TRACKBALL_LED_ENABLED` is set. |
| Trackball LED GRN | 17 | Optional. |
| Trackball LED BLU | 21 | Optional. |
| Trackball LED WHT | *(not wired)* | Skipped -- there wasn't a 5th free flexible pin in this map. Drive RED+GRN+BLU together for a white-ish approximation if you want that, or free up a pin (e.g. reclaim GPIO2 from the keyboard matrix and move that row elsewhere) if you specifically want the true white LED. |

Pins avoided deliberately: GPIO 6-11 (internal flash, unusable), GPIO 1/3 (UART0, used for programming/serial monitor), and the strapping pins 0/2/5/12/15 where avoidable (two of these, 0 and 2, were already in use by the keyboard matrix rows in the *existing* pin map, not something introduced here -- left as-is for continuity rather than reshuffling working pin assignments).

## Bill of materials (additions)

- ICSH044A / BlackBerry trackball breakout board (2.5-5.25V, ~28x22x8mm)
- 4x 10kΩ resistors (pull-ups for UP/DOWN/LEFT/RIGHT -- see the pin table)
- Current-limiting resistors for the RGB LEDs if `TRACKBALL_LED_ENABLED` and the breakout doesn't already have them on-board (unconfirmed either way -- check with a multimeter/datasheet before assuming, then use standard values for the LED's forward voltage at 3.3V logic, typically in the 220-470Ω range for small SMD LEDs)
- Wire for 7-11 signal connections (5 functional + up to 4 LED) plus VCC/GND, same idea as the existing BBQ10-to-mainboard wiring in [Hardware_Fairberry_Mainboard.md](Hardware_Fairberry_Mainboard.md)

## Firmware

See `BBQ10/trackball.h` for the implementation and `BBQ10/configuration.h` for the `TRACKBALL_ENABLED`/`TRACKBALL_LED_ENABLED`/`TRACKBALL_EDGES_PER_STEP`/`TRACKBALL_BTN_KEY` switches. Short version: movement is read as raw hall-sensor edges via interrupts (the direction pins' idle polarity wasn't consistently documented across sources, so this counts transitions rather than assuming a signal shape) and converted into arrow-key taps once enough edges accumulate on an axis; the center click sends Enter by default. This is a deliberately simple mapping (see the earlier discussion in this repo's history for why arrow keys instead of a real mouse HID report -- mainly that it stays inside the existing pure-keyboard-HID firmware instead of needing a new BLE HID report type).

## Physical mounting

Not designed yet. The module is roughly 28x22x8mm (LxWxH) -- whatever cutout/mount goes into the case needs a hole for the ball itself plus clearance for the board and its wiring. Given the BlackBerry Q10 keyboard this project is built around never had a trackball (Q10 used a capacitive touch strip below the keys, not a trackball -- the trackball was on older BlackBerry Bold/Curve models), there's no existing cavity to reuse; this needs a new cutout in whatever case design ends up being used. Worth deciding where before finalizing the X4 case work: e.g. above the keyboard near the screen, or integrated into the case sleeve area.
