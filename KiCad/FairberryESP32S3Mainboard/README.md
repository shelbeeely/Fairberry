# FairberryESP32S3Mainboard

KiCad 7 schematic for the standalone smart-keyboard mainboard (ESP32-S3-WROOM-1,
trackball, mic, speaker amp, microSD, LiPo charger). Pin assignments match
`BBQ10/boards.h`'s `FAIRBERRY_ESP32S3_SMART` board type exactly.

30 components, 44 verified nets (every net has 2+ pins, no dangling nets).
Connectivity uses net labels (`(label "NETNAME" ...)`) on short wire stubs at
each pin rather than routed point-to-point wires, which is standard KiCad
practice for pin-dense boards and keeps the sheet readable.

## Module SKU: ESP32-S3-WROOM-1-N16R8

The schematic symbol (`RF_Module:ESP32-S3-WROOM-1`) is generic across every
WROOM-1 flash/PSRAM SKU — the pinout is identical. For the BOM, this board
specs the **-N16R8** variant (16MB flash, 8MB octal PSRAM) for headroom on
audio buffers, the web dashboard, and WiFi/TLS.

That choice has one real consequence: on any *octal*-PSRAM WROOM-1 SKU
(anything ending `R8` or `R16V`), GPIO35/36/37 are wired internally to the
in-package PSRAM and can't be used as GPIO — they simply don't work as
signal pins, even though the generic symbol still draws them. A cheaper
*quad*-PSRAM SKU (`R2`, e.g. -N16R2, 2MB PSRAM) wouldn't have this
restriction and would need no pin changes, at the cost of much less RAM.
Having picked -N16R8, mic I2S was moved off GPIO35-37 onto GPIO43/44
(silkscreened RXD0/TXD0, i.e. UART0 — free because this board's console
runs over native USB, not UART0) and GPIO46 (a strapping pin, safe here
since MIC_DIN is an input and the mic's output doesn't drive that pin
during a bootloader-mode reset). See the comment above
`MIC_I2S_WS_PIN` in `BBQ10/boards.h` for the full reasoning.

## Known simplifications (not yet fab-ready)

- **J6 (USB)**: a plain 4-pin header standing in for a real USB-C receptacle.
  No CC1/CC2 pull-down resistors are present, so USB-C power negotiation
  won't work as-is — replace with a real USB-C connector symbol/footprint
  (with CC resistors) before fabrication.
- **U3 (3.3V LDO)**: uses the generic `Regulator_Linear:MCP1700x-300xxTT`
  base symbol (the `extends`-based `-330` variant wasn't practical to
  reference programmatically); functionally equivalent, same pinout.
- **U6 (battery-low supervisor)**: uses the generic
  `Power_Supervisor:MCP100-270D` base symbol; the real BOM part is the
  -300 (3.0V threshold) variant, same pinout.
- **U4 (MAX98357A)**: footprint is the QFN package; double-check against
  the actual part you source (some breakout modules differ).

## Regenerating the schematic

The `.kicad_sch` file is generated, not hand-drawn, from `generator/*.py`
(pulls real symbol/pin data straight out of the installed `kicad-symbols`
library files, plus a hand-built symbol for the BBQ10 keyboard's Hirose
connector sourced from `KiCad/FairberryMainboard/library/Keyboard.lib`).
To regenerate after editing `generator/build_sch3.py` (component list /
net map):

```sh
cd KiCad/FairberryESP32S3Mainboard/generator
python3 build_sch4.py
```

This overwrites `../FairberryESP32S3Mainboard.kicad_sch` and prints every
net with its pin list, plus a warning for any single-pin (dangling) net.

## Verifying with kicad-cli

`kicad-cli` (KiCad 7) needs `KICAD7_SYMBOL_DIR` set (the environment this was
built in didn't have it set by default) and a display, even for headless
export (it's a wx/GTK app under the hood) — use `xvfb-run` if there's no X
server:

```sh
export KICAD7_SYMBOL_DIR=/usr/share/kicad/symbols
cd KiCad/FairberryESP32S3Mainboard
xvfb-run -a kicad-cli sch export svg FairberryESP32S3Mainboard.kicad_sch -o /tmp/out
xvfb-run -a kicad-cli sch export netlist FairberryESP32S3Mainboard.kicad_sch -o /tmp/out.net
```

Then sanity-check the netlist: every net should have 2+ nodes except a
short, expected list of no-connects (unused ESP32-S3 strapping pins
IO0/IO3/IO45/IO46, UART0's RXD0/TXD0 which are intentionally left free,
MAX98357A's NC pins, and J1's MIC pin from the keyboard connector, which
isn't used since a separate mic — U5 — is on this board).

### Pitfalls hit building this (kept here so they don't get re-debugged)

- **kicad-cli segfaults** on any schematic referencing a real library
  symbol if `KICAD7_SYMBOL_DIR` is unset or there's no local/global
  `sym-lib-table` — but it *also* segfaults on a well-formed file if a
  symbol's cached name in `lib_symbols` doesn't exactly match its `lib_id`
  (e.g. a `Heater` block cached for something referenced as `Device:R`).
  Getting a clean `Failed to load schematic file` instead of a segfault
  means the structural error is at least being caught.
- **`(wire ...)` must not include a `(fill (type none))` clause** and
  should use `(type solid)`, not `(type default)` — copying the stroke
  syntax from a symbol's graphic *rectangle* onto a wire breaks loading.
- **Pin connection points must be exact, unrounded coordinates.** Snapping
  a computed pin position to the nearest 1.27mm grid before drawing its
  wire stub seems harmless but silently detaches the wire from the pin
  whenever the component's placement position isn't itself grid-aligned —
  the file still loads and renders fine, wires visually touch the pins,
  but `kicad-cli sch export netlist` shows almost everything as
  "unconnected". Always compute `component_position + pin_local_offset`
  with no rounding for the pin end of any wire.
