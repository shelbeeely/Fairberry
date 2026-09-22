# FairberryESP32S3Mainboard

KiCad 7 schematic for the standalone smart-keyboard mainboard (ESP32-S3-WROOM-1,
trackball, mic, speaker amp, microSD, LiPo charger). Pin assignments match
`BBQ10/boards.h`'s `FAIRBERRY_ESP32S3_SMART` board type exactly.

31 components, 44 verified nets (every net has 2+ pins, no dangling nets).
Connectivity uses net labels (`(label "NETNAME" ...)`) on short wire stubs at
each pin rather than routed point-to-point wires, which is standard KiCad
practice for pin-dense boards and keeps the sheet readable.

![Schematic](FairberryESP32S3Mainboard.svg)

This is the schematic only -- `FairberryESP32S3Mainboard.kicad_pcb` exists
(see PCB section below) but only has the netlist imported: footprints sit on
an unplaced grid with no board outline or routing yet. `FairberryESP32S3Mainboard.svg`
is exported straight from the `.kicad_sch` via `kicad-cli sch export svg`
(see the command below); if you change the schematic, regenerate it the
same way so the image doesn't go stale:

```sh
export KICAD7_SYMBOL_DIR=/usr/share/kicad/symbols
xvfb-run -a kicad-cli sch export svg FairberryESP32S3Mainboard.kicad_sch -o /tmp/out
cp /tmp/out/FairberryESP32S3Mainboard.svg FairberryESP32S3Mainboard.svg
```

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

## Generating the PCB

`kicad-cli`'s `pcb` subcommand only has `export` in KiCad 7 -- there's no
`pcb import-netlist`, and the GUI's "Update PCB from Schematic" dialog
isn't exposed to scripting either. `generator/build_pcb.py` does that step
directly against the `pcbnew` Python API instead: it loads each
component's real footprint (verifying every one exists on disk -- one
placeholder footprint name and one wrong library path were caught and
fixed this way, see `build_sch3.py`'s `FP` dict comments), places them on
an unrouted grid, and wires every pad to its net by pad *number* (cross-checked
against each real `.kicad_mod` file, not assumed to equal the schematic
pin number sight unseen -- e.g. the microSD footprint's shield pin is 4
physical pads that all share pad number 9, matched by grouping pads by
number rather than taking the first hit).

```sh
cd KiCad/FairberryESP32S3Mainboard/generator
python3 build_pcb.py
```

This overwrites `../FairberryESP32S3Mainboard.kicad_pcb` and prints the
footprint/net counts. It needs `pcbnew` (KiCad's Python bindings,
`/usr/lib/python3/dist-packages/pcbnew.py` + `_pcbnew.so` on this
environment's install) importable from plain `python3` — no
`KICAD7_SYMBOL_DIR` or `xvfb` needed for this step, those only matter for
`kicad-cli`.

**Not done yet:** board outline, footprint placement (beyond "doesn't
overlap"), and routing. Placement/outline depend on the case's real
dimensions; routing is real layout work on top of this.

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
IO0/IO3/IO45, the octal-PSRAM-reserved IO35/36/37 (see the Module SKU
section — RXD0/TXD0 aren't in this list anymore, they carry mic I2S now),
MAX98357A's NC pins, and J1's MIC pin from the keyboard connector, which
isn't used since a separate mic — U5 — is on this board).

To check the same thing on the PCB instead of the schematic (useful after
running `build_pcb.py`, to confirm every pad landed on the net it should
have):

```sh
python3 -c "
import pcbnew
board = pcbnew.LoadBoard('FairberryESP32S3Mainboard.kicad_pcb')
netpads = {}
for fp in board.GetFootprints():
    for pad in fp.Pads():
        netpads.setdefault(pad.GetNetname(), 0)
        netpads[pad.GetNetname()] += 1
single = [n for n, c in netpads.items() if c == 1 and n]
print('single-pad (dangling) nets:', single or 'none')
"
```

A PCB-only rendering (copper + silkscreen + outline, no schematic-style
labels) can be exported the same way as the schematic, except `-o` here
takes a file path directly rather than a directory when `--layers` is
given:

```sh
xvfb-run -a kicad-cli pcb export svg FairberryESP32S3Mainboard.kicad_pcb -o /tmp/out.svg --layers F.Cu,F.Silkscreen,Edge.Cuts
```

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
- **A schematic's `Footprint` property string is never validated against
  real files.** The microSD footprint (`Connector_Card:Conn_01x08_MicroSD_Card`)
  was a guessed name that doesn't exist in KiCad's footprint library, and
  the custom keyboard connector's footprint was referenced under a made-up
  `Fairberry:` library instead of the real `Connectors_Hirose_extra:` one
  it actually lives in — both loaded fine as schematic text and only broke
  when `build_pcb.py` called `pcbnew.FootprintLoad()` on them and got
  `None`. Nothing catches a bad footprint string until PCB generation.
- **Footprint pads can outnumber symbol pins**, and pad *numbers* -- not
  symbol pin numbers assumed to carry over unchanged -- are what has to
  match. The microSD footprint has 4 physical shield pads that all share
  pad number 9, corresponding to the symbol's single SHIELD pin; grouping
  a footprint's pads by `GetNumber()` before assigning nets (rather than
  taking the first match, or assuming a 1:1 pad-to-pin count) is what
  catches this.
