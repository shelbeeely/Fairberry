import sys, os, uuid
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_schematic import extract_raw_symbol_block, get_pin_defs

def nu():
    return str(uuid.uuid4())

SYM_DIR = '/usr/share/kicad/symbols/'

LIB_SOURCES = {
    'Power_Supervisor:MCP100-270D': (SYM_DIR+'Power_Supervisor.kicad_sym', 'MCP100-270D', 'Package_TO_SOT_SMD:SOT-23-3'),
    'RF_Module:ESP32-S3-WROOM-1': (SYM_DIR+'RF_Module.kicad_sym', 'ESP32-S3-WROOM-1', 'RF_Module:ESP32-S3-WROOM-1'),
    'Battery_Management:MCP73831-2-OT': (SYM_DIR+'Battery_Management.kicad_sym', 'MCP73831-2-OT', 'Package_TO_SOT_SMD:SOT-23-5'),
    'Regulator_Linear:MCP1700x-300xxTT': (SYM_DIR+'Regulator_Linear.kicad_sym', 'MCP1700x-300xxTT', 'Package_TO_SOT_SMD:SOT-23'),
    'Audio:MAX98357A': (SYM_DIR+'Audio.kicad_sym', 'MAX98357A', 'Package_DFN_QFN:TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm'),
    'Sensor_Audio:ICS-43434': (SYM_DIR+'Sensor_Audio.kicad_sym', 'ICS-43434', 'Sensor_Audio:InvenSense_ICS-43434-6_3.5x2.65mm'),
    'Connector:Micro_SD_Card': (SYM_DIR+'Connector.kicad_sym', 'Micro_SD_Card', 'Connector_Card:Conn_01x08_MicroSD_Card'),
    'Connector_Generic:Conn_01x02': (SYM_DIR+'Connector_Generic.kicad_sym', 'Conn_01x02', 'Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical'),
    'Connector_Generic:Conn_01x04': (SYM_DIR+'Connector_Generic.kicad_sym', 'Conn_01x04', 'Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical'),
    'Connector_Generic:Conn_01x07': (SYM_DIR+'Connector_Generic.kicad_sym', 'Conn_01x07', 'Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical'),
    'Device:R': (SYM_DIR+'Device.kicad_sym', 'R', 'Resistor_SMD:R_0603_1608Metric'),
    'Device:C': (SYM_DIR+'Device.kicad_sym', 'C', 'Capacitor_SMD:C_0603_1608Metric'),
    'Device:LED': (SYM_DIR+'Device.kicad_sym', 'LED', 'LED_SMD:LED_0603_1608Metric'),
}

raw_blocks = {}
pin_data = {}
for lib_id, (path, symname, fp_override) in LIB_SOURCES.items():
    block = extract_raw_symbol_block(path, symname)
    # KiCad requires the TOP-LEVEL symbol name in a schematic's embedded
    # lib_symbols cache to be the fully qualified "Library:Name" (matching
    # how it's referenced via lib_id in instances) -- the extracted block
    # has the bare name from the source library file. Sub-unit names
    # (e.g. "NAME_0_1") stay bare, unqualified -- confirmed against
    # /usr/share/kicad/demos/stickhub/StickHub.kicad_sch, which is why
    # this replaces only the exact first occurrence, not a broader
    # substring rename.
    block = block.replace(f'"{symname}"', f'"{lib_id}"', 1)
    raw_blocks[lib_id] = block
    pins, fp = get_pin_defs(path, symname)
    pin_data[lib_id] = pins

# BBQ10KBD custom symbol (real Hirose connector pinout from
# KiCad/FairberryMainboard/library/Keyboard.lib, redrawn as a simple box
# in current-format KiCad syntax -- graphics are original, pin
# names/numbers are the real ones from that file).
BBQ10KBD_RAW_PINS = [
    ('1','GND',-15.24,-15.24,0),('2','MIC_VDD',-15.24,15.24,0),
    ('3','MIC',-15.24,-2.54,0),('4','AGND',-15.24,-12.7,0),
    ('5','GND',-15.24,-15.24,0),('6','ROW1',15.24,0,180),
    ('7','COL1',15.24,15.24,180),('8','ROW2',15.24,-2.54,180),
    ('9','COL2',15.24,12.7,180),('10','COL3',15.24,10.16,180),
    ('11','GND',-15.24,-15.24,0),('12','GND',-15.24,-15.24,0),
    ('13','GND',-15.24,-15.24,0),('14','GND',-15.24,-15.24,0),
    ('15','GND',-15.24,-15.24,0),('16','GND',-15.24,-15.24,0),
    ('17','COL4',15.24,7.62,180),('18','ROW3',15.24,-5.08,180),
    ('19','COL5',15.24,5.08,180),('20','ROW4',15.24,-7.62,180),
    ('21','ROW5',15.24,-10.16,180),('22','ROW6',15.24,-12.7,180),
    ('23','LEDK_2_3',-15.24,2.54,0),('24','LEDA_3_4',-15.24,7.62,0),
    ('25','LEDK_1_4',-15.24,5.08,0),('26','LEDA_1_2',-15.24,10.16,0),
    ('27','ROW7',15.24,-15.24,180),('28','GND',-15.24,-15.24,0),
]
# Note: for GND pins that repeat, we just reuse them all pointing to net GND.
pin_data['Fairberry:BBQ10KBD'] = [
    {'num': n, 'name': nm, 'x': x, 'y': y, 'rot': r} for (n,nm,x,y,r) in BBQ10KBD_RAW_PINS
]

def bbq10kbd_raw_block():
    pin_lines = []
    for num, name, x, y, rot in BBQ10KBD_RAW_PINS:
        pin_lines.append(
            f'    (pin passive line (at {x} {y} {rot}) (length 5.08)\n'
            f'      (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'      (number "{num}" (effects (font (size 1.27 1.27))))\n'
            f'    )'
        )
    pins_txt = '\n'.join(pin_lines)
    return f'''(symbol "Fairberry:BBQ10KBD" (in_bom yes) (on_board yes)
  (property "Reference" "J" (at -17.78 19.05 0) (effects (font (size 1.27 1.27))))
  (property "Value" "BBQ10KBD" (at 0 19.05 0) (effects (font (size 1.27 1.27))))
  (property "Footprint" "Fairberry:BM14B(0.8)-24DS-0.4V(53)" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
  (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
  (property "ki_description" "BlackBerry Q10 keyboard flex connector (real Hirose pinout, reused from KiCad/FairberryMainboard/library/Keyboard.lib)" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
  (symbol "BBQ10KBD_0_1"
    (rectangle (start -15.24 17.78) (end 15.24 -17.78) (stroke (width 0.254) (type default)) (fill (type background)))
  )
  (symbol "BBQ10KBD_1_1"
{pins_txt}
  )
)'''

raw_blocks['Fairberry:BBQ10KBD'] = bbq10kbd_raw_block()

print("Loaded", len(raw_blocks), "symbols")
for k in raw_blocks:
    print(" -", k, len(pin_data.get(k, [])), "pins")
