"""Import the schematic's netlist into a new .kicad_pcb using the pcbnew
Python API directly (BOARD/FOOTPRINT/PAD/NETINFO_ITEM), rather than
kicad-cli -- kicad-cli 7's `pcb` subcommand only has `export`, no netlist
import, and the GUI's "Update PCB from Schematic" dialog isn't exposed to
Python scripting. This re-does that step manually: load each component's
real footprint, place it, and wire its pads to nets by pad NUMBER (which
is required to match the schematic pin NUMBER for this to be meaningful --
verified per-component below, not assumed).

Footprints are placed on a simple unrouted grid, in schematic order --
this is a netlist import, not a placement/routing pass. No board outline
is added yet (deferred until real case dimensions are settled).
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcbnew
from build_sch3 import components, pin_data, FP

HERE = os.path.dirname(os.path.abspath(__file__))
SYS_FP = '/usr/share/kicad/footprints'
# Footprint libraries that aren't in the system kicad-footprints install --
# resolved from this repo instead.
LOCAL_FP_LIBS = {
    'Connectors_Hirose_extra': os.path.join(
        HERE, '..', '..', 'FairberryMainboard', 'modules', 'Connectors_Hirose_extra.pretty'),
}

OUT_PATH = os.path.join(HERE, '..', 'FairberryESP32S3Mainboard.kicad_pcb')


def resolve_fp_dir(lib):
    if lib in LOCAL_FP_LIBS:
        return os.path.abspath(LOCAL_FP_LIBS[lib])
    return os.path.join(SYS_FP, lib + '.pretty')


board = pcbnew.CreateEmptyBoard()

# Simple grid placement, ~15mm pitch, wrapping every 6 columns -- arbitrary,
# just needs to not overlap footprints so the ratsnest is readable pending
# a real placement pass.
COLS = 6
PITCH_MM = 15.0
ORIGIN_MM = (50.0, 50.0)

placed = 0
net_cache = {}  # name -> NETINFO_ITEM

def get_net(name):
    if name in net_cache:
        return net_cache[name]
    existing = board.FindNet(name)
    if existing is not None:
        net_cache[name] = existing
        return existing
    n = pcbnew.NETINFO_ITEM(board, name)
    board.Add(n)
    net_cache[name] = n
    return n

for i, c in enumerate(components):
    ref, lib_id, value = c['ref'], c['lib_id'], c['value']
    fpstr = FP.get(lib_id)
    if not fpstr:
        raise ValueError(f"{ref} ({lib_id}): no footprint in FP dict")
    lib, name = fpstr.split(':', 1)
    fpdir = resolve_fp_dir(lib)
    fp = pcbnew.FootprintLoad(fpdir, name)
    if fp is None:
        raise ValueError(f"{ref}: footprint '{name}' not found in {fpdir} "
                          f"(lib_id={lib_id}, FP entry={fpstr!r})")
    fp.SetReference(ref)
    fp.SetValue(value)

    col = i % COLS
    row = i // COLS
    x_mm = ORIGIN_MM[0] + col * PITCH_MM
    y_mm = ORIGIN_MM[1] + row * PITCH_MM
    fp.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x_mm), pcbnew.FromMM(y_mm)))

    board.Add(fp)
    placed += 1

    # Group pads by number (some footprints have several physical pads
    # sharing one logical pad number, e.g. the microSD shield tabs), and
    # wire every pad of a given number to that pin's net.
    pads_by_num = {}
    for pad in fp.Pads():
        pads_by_num.setdefault(pad.GetNumber(), []).append(pad)

    for p in pin_data[lib_id]:
        key = p['name'] if p['name'] in c['net'] else p['num']
        net_name = c['net'][key]
        if net_name is None:
            continue
        matching_pads = pads_by_num.get(str(p['num']), [])
        if not matching_pads:
            raise ValueError(
                f"{ref}: no footprint pad numbered '{p['num']}' "
                f"(schematic pin {p['name']}, footprint {fpstr})")
        net = get_net(net_name)
        for pad in matching_pads:
            pad.SetNet(net)

print(f"Placed {placed} footprints, {len(net_cache)} nets.")

ok = board.Save(OUT_PATH)
print("Saved" if ok is None or ok else "Save returned:", OUT_PATH)
