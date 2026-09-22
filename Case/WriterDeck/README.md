# Writer deck combined case

A single clamshell enclosure holding both the [Fairberry keyboard](../../BBQ10) (keyboard + trackball + mic/speaker/battery board) and the Xteink X4 (screen board), as one physical device instead of two paired-over-BLE pieces. See [Documentation/Hardware_X4_Writer_Deck.md](../../Documentation/Hardware_X4_Writer_Deck.md) for the electronics/firmware side this wraps around.

**Status: first-draft OpenSCAD exists (`WriterDeck.scad`), built from a real reference model, not print-tested.** The user provided a reference case -- "XteinkX4FullCoverageFlipCase" (`reference/`, 3 STLs: `BOTTOM`, `TOP`, `HINGE` -- no author/license metadata embedded in the files, confirm the original source's license before redistributing publicly if it came from somewhere like Printables/MakerWorld/Thingiverse). Rather than design a hinge from scratch, this reuses that reference's actual hinge mechanism directly:

- **`BOTTOM.stl`** (27590 triangles, has real X4-specific port/button cutouts) is the X4 cradle. **Reused completely unmodified.**
- **`HINGE.stl`** (1432 triangles, a separate comb+pin bar spanning X 15.06-50.86mm) is the knuckle/pin piece that threads through both halves. **Reused completely unmodified.**
- **`TOP.stl`** (11760 triangles, much simpler than BOTTOM -- no device-specific geometry) was a plain flip-lid in the reference. Measured its two corner hinge-knuckle loops precisely (left loop: X[-2.9, 9.0] Y[105.1, 116.7] Z[6.0, 9.2]; right loop: X[59.7, 68.8], same Y/Z) and extract *just those* via `intersection()` in OpenSCAD -- the flat lid body itself is discarded and replaced with the keyboard tray. This means the knuckle-loop geometry is pixel-for-pixel identical to the tested reference, not a redrawn approximation.

Confirmed this way (not guessed): BOTTOM is the more geometrically complex piece (more than double TOP's triangle count) and sits at a lower Z range in the hinge region (Z up to 6.6) while TOP sits above it (Z 6.0-9.2) -- consistent with BOTTOM being a snug device cradle and TOP being a cover that closes over it, which is also why BOTTOM was the one worth keeping unmodified and TOP was the one safe to gut and repurpose.

## Decisions made so far

- **Portrait orientation -- reversed from an earlier landscape recommendation.** Before seeing the reference model, I'd recommended landscape (the X4's panel is natively 800x480, a landscape pixel buffer -- used as-sold in portrait for reading, the e-reader software rotates that framebuffer 90 degrees, and landscape avoids that rotation while matching Freewrite/Pomera/AlphaSmart's form language). The reference model changed that: it's built portrait (hinge along the X4's short ~70mm edge, footprint matching the X4 held as normally sold), with a real, evidently-tested interlocking hinge. Reusing that proven hinge outweighs the software-rotation nicety -- portrait rendering is already how the X4's stock reader software works, so it's a well-trodden path, not a risky one. Also, portrait keeps the case width close to the keyboard's own ~70mm width (no wasted bezel space), where landscape would have needed the case to be 114mm wide.
- **Clamshell, not rigid single-piece.** Folds closed like a small laptop. In the reference's own closed-position modeling (both halves share the same X-Y footprint, stacked in Z), the X4 half (`BOTTOM`) sits lower and the other half sits on top when closed -- doesn't dictate which one ends up "on the table" when opened, any more than it does on a real laptop.
- **Two independent electronics systems, one shared shell.** No PCB/firmware changes from this -- the X4 board and the Fairberry board each keep their own battery and BLE pairing exactly as already built. Purely a mechanical/enclosure question. No wires need to cross the hinge.
- **Reuse the reference hinge exactly, don't redesign it.** See the Status section above -- `BOTTOM` and `HINGE` reused unmodified, only `TOP`'s two corner knuckle-loops extracted and reused, its flat lid body replaced with the keyboard tray.

## What's still unverified in `WriterDeck.scad`

Everything the file's own comments flag as `UNVERIFIED`, specifically:

- **Keyboard tray cavity height/depth.** No PCB layout exists yet for `FAIRBERRY_ESP32S3_SMART` (see [Hardware_Standalone_Smart_Keyboard.md](../../Documentation/Hardware_Standalone_Smart_Keyboard.md)'s own "no enclosure yet" note) -- the electronics bay dimensions are a reasonable-guess placeholder, not fitted to anything.
- **Trackball/keyboard cutout exact positions.** Keyboard cutout uses the *real* measured dimensions of `Case/Generator/import/BBQ10KBD.stl` (66.6 x 36.1mm) -- that part's solid. Its position within the tray, and the trackball cutout's position/size (28x22mm from the trackball doc's BOM, not measured against a real trackball model), are first-guess placements reasoned from BlackBerry-style ergonomics (trackball near the hinge/screen edge), not fitted to real board geometry.
- **Wall thickness (2mm) and hinge alignment tolerance** -- not print-tested against either this design or even the reference case itself (which I've only analyzed geometrically, not printed).
- **Battery access** (charging/replacement) on both halves isn't addressed at all yet.

## Next step

Print a test fit of just the hinge region first (both halves, short in Y, just enough to include the knuckle loops + a few mm of tray) before committing to a full-size print -- same advice as everywhere else in this repo that carries an "unverified" tag. Then iterate on the tray cavity dimensions once there's a real PCB (or at least a breadboard mockup) to measure against.
