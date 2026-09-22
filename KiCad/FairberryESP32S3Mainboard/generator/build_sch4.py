import sys, os, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_sch3 import components, pin_data, raw_blocks, FP, nu

PROJECT_NAME = "FairberryESP32S3Mainboard"
ROOT_UUID = nu()

def grid(v, step=1.27):
    return round(round(v/step)*step, 3)

out = []
out.append('(kicad_sch (version 20230121) (generator eeschema)\n')
out.append(f'  (uuid {ROOT_UUID})\n')
out.append('  (paper "A2")\n\n')

# lib_symbols cache
out.append('  (lib_symbols\n')
for lib_id, block in raw_blocks.items():
    # indent the raw block by 4 spaces (it starts at column 0 or 2)
    indented = '\n'.join(('    ' + line) if line.strip() else line for line in block.splitlines())
    out.append(indented + '\n')
out.append('  )\n\n')

pin_stub_lines = []
label_lines = []
junction_lines = []
instance_lines = []
net_pin_positions = {}  # net_name -> list of (x,y) for sanity checking

STUB_LEN = 2.54

for c in components:
    ref, lib_id, cx, cy, value = c['ref'], c['lib_id'], c['x'], c['y'], c['value']
    uid = nu()
    fp = FP.get(lib_id, '')
    instance_lines.append(f'  (symbol (lib_id "{lib_id}") (at {cx} {cy} 0) (unit 1)\n')
    instance_lines.append(f'    (in_bom yes) (on_board yes) (dnp no)\n')
    instance_lines.append(f'    (uuid {uid})\n')
    instance_lines.append(f'    (property "Reference" "{ref}" (at {cx} {cy-5} 0) (effects (font (size 1.27 1.27))))\n')
    instance_lines.append(f'    (property "Value" "{value}" (at {cx} {cy+5} 0) (effects (font (size 1.27 1.27))))\n')
    instance_lines.append(f'    (property "Footprint" "{fp}" (at {cx} {cy} 0) (effects (font (size 1.27 1.27)) hide))\n')
    instance_lines.append(f'    (property "Datasheet" "" (at {cx} {cy} 0) (effects (font (size 1.27 1.27)) hide))\n')

    pins = pin_data[lib_id]
    for p in pins:
        key = p['name'] if p['name'] in c['net'] else p['num']
        net = c['net'][key]
        pin_uid = nu()
        instance_lines.append(f'    (pin "{p["num"]}" (uuid {pin_uid}))\n')

        if net is None:
            continue  # deliberately NC, no stub/label (visually a bare pin end, fine for review)

        # Absolute pin position: symbol placed at rotation 0, so it's just
        # component position + pin local offset. This MUST be the exact
        # (unrounded) coordinate -- KiCad's connectivity engine requires the
        # wire endpoint to exactly match the pin's true connection point, and
        # component positions are not necessarily on the 1.27mm grid, so
        # rounding here silently detaches every wire from its pin (confirmed
        # by a minimal 2-resistor test: rounding caused 148/152 nets in the
        # real board to come back "unconnected" from kicad-cli's netlist
        # export, despite the file loading and rendering fine).
        px = cx + p['x']
        py = cy - p['y']  # KiCad Y in symbol defs is up-positive; schematic sheet Y is down-positive, so flip
        # Determine stub direction from the pin's own rotation (0=points
        # left from body i.e. wire extends further left/-X, 180=points
        # right/+X, 90=down/-Y... KiCad pin "at" rotation is the direction
        # the pin STICKS OUT from the body). We only used 0/180 for our
        # custom symbol and rely on library rotation for the rest.
        rot = p['rot'] % 360
        if rot == 0:
            dx, dy = STUB_LEN, 0
        elif rot == 180:
            dx, dy = -STUB_LEN, 0
        elif rot == 90:
            dx, dy = 0, STUB_LEN
        elif rot == 270:
            dx, dy = 0, -STUB_LEN
        else:
            dx, dy = STUB_LEN, 0
        ex, ey = grid(px + dx), grid(py - dy)  # flip dy same as above

        pin_stub_lines.append(
            f'  (wire (pts (xy {px} {py}) (xy {ex} {ey})) (stroke (width 0) (type solid)) (uuid {nu()}))\n'
        )
        label_lines.append(
            f'  (label "{net}" (at {ex} {ey} 0) (effects (font (size 1.27 1.27)) (justify left)) (uuid {nu()}))\n'
        )
        net_pin_positions.setdefault(net, []).append((ref, p['num'], p['name']))

    instance_lines.append('    (instances\n')
    instance_lines.append(f'      (project "{PROJECT_NAME}"\n')
    instance_lines.append(f'        (path "/{ROOT_UUID}"\n')
    instance_lines.append(f'          (reference "{ref}") (unit 1)\n')
    instance_lines.append('        )\n')
    instance_lines.append('      )\n')
    instance_lines.append('    )\n')
    instance_lines.append('  )\n\n')

out.extend(instance_lines)
out.extend(pin_stub_lines)
out.extend(label_lines)
out.append('\n  (sheet_instances\n')
out.append('    (path "/" (page "1"))\n')
out.append('  )\n')
out.append(')\n')

OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'FairberryESP32S3Mainboard.kicad_sch')
with open(OUT_PATH, 'w') as f:
    f.writelines(out)

print("Wrote schematic.")
print(f"\n{len(net_pin_positions)} distinct nets:")
for net, pins in sorted(net_pin_positions.items()):
    print(f"  {net}: {len(pins)} pin(s) -> {pins}")

# Flag nets with only 1 pin (likely a mistake -- dangling)
single = [n for n, pins in net_pin_positions.items() if len(pins) == 1]
if single:
    print(f"\nWARNING: {len(single)} net(s) with only ONE pin (dangling, likely an error):")
    for n in single:
        print(f"  {n}: {net_pin_positions[n]}")
else:
    print("\nNo single-pin (dangling) nets. Every net connects 2+ pins.")
