import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_sch2 import raw_blocks, pin_data, LIB_SOURCES, nu

# footprint overrides (component ref -> footprint string), keyed by lib_id
FP = {
    'Power_Supervisor:MCP100-270D': 'Package_TO_SOT_SMD:SOT-23-3',
    'RF_Module:ESP32-S3-WROOM-1': 'RF_Module:ESP32-S3-WROOM-1',
    'Battery_Management:MCP73831-2-OT': 'Package_TO_SOT_SMD:SOT-23-5',
    'Regulator_Linear:MCP1700x-300xxTT': 'Package_TO_SOT_SMD:SOT-23',
    'Audio:MAX98357A': 'Package_DFN_QFN:TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm',
    'Sensor_Audio:ICS-43434': 'Sensor_Audio:InvenSense_ICS-43434-6_3.5x2.65mm',
    'Connector:Micro_SD_Card': 'Connector_Card:Conn_01x08_MicroSD_Card',
    'Connector_Generic:Conn_01x02': 'Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical',
    'Connector_Generic:Conn_01x04': 'Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical',
    'Connector_Generic:Conn_01x07': 'Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical',
    'Device:R': 'Resistor_SMD:R_0603_1608Metric',
    'Device:C': 'Capacitor_SMD:C_0603_1608Metric',
    'Device:LED': 'LED_SMD:LED_0603_1608Metric',
    'Fairberry:BBQ10KBD': 'Fairberry:BM14B(0.8)-24DS-0.4V(53)',
}

# ---- Component instances ----
# Each: ref, lib_id, x, y (mm, schematic sheet coords), value, net_map {pin_name: net or None}
components = []

def pinmap_by_name(lib_id):
    """helper: {pin_name: [pin_num,...]} since some parts reuse a name (e.g. many GND pins)"""
    m = {}
    for p in pin_data[lib_id]:
        m.setdefault(p['name'], []).append(p['num'])
    return m

# U1 ESP32-S3-WROOM-1
esp32_net = {
    'GND': 'GND', '3V3': '3V3', 'EN': 'EN',
    'IO1': 'KBD_ROW1', 'IO2': 'KBD_ROW2', 'IO4': 'KBD_ROW3', 'IO5': 'KBD_ROW4',
    'IO6': 'KBD_ROW5', 'IO7': 'KBD_ROW6', 'IO8': 'KBD_ROW7',
    'IO9': 'KBD_COL1', 'IO10': 'KBD_COL2', 'IO11': 'KBD_COL3', 'IO12': 'KBD_COL4', 'IO13': 'KBD_COL5',
    'IO14': 'KBD_BACKLIGHT_DRV',
    'IO15': 'TRACKBALL_UP', 'IO16': 'TRACKBALL_DOWN', 'IO17': 'TRACKBALL_LEFT', 'IO18': 'TRACKBALL_RIGHT',
    'IO21': 'BATTERY_LOW',
    'IO38': 'SPEAKER_WS', 'IO39': 'SPEAKER_BCLK', 'IO40': 'SPEAKER_DOUT',
    'IO41': 'SD_CLK', 'IO42': 'SD_CMD', 'IO47': 'SD_D0',
    'IO48': 'TRACKBALL_BTN',
    'IO19': 'USB_DM', 'IO20': 'USB_DP',
    # Mic I2S deliberately avoids IO35/36/37: on the octal-PSRAM SKU this
    # board is speced for (WROOM-1-N16R8), those three pins are internally
    # wired to the in-package PSRAM and are not usable as GPIO. TXD0/RXD0
    # are free to reuse for the WS/BCLK outputs because this board's debug
    # console runs over the native USB-C port (USB_DM/USB_DP, IO19/IO20) via
    # USB CDC, not UART0 -- see README. IO46 is a strapping pin but is safe
    # to use for MIC_DIN (an input) as long as nothing pulls it high during
    # a bootloader-mode reset, which the mic's SD line does not.
    'TXD0': 'MIC_WS', 'RXD0': 'MIC_BCLK', 'IO46': 'MIC_DIN',
    'IO35': None, 'IO36': None, 'IO37': None,  # consumed internally by octal PSRAM on the N16R8 SKU
    'IO0': None, 'IO3': None, 'IO45': None,  # strapping pins, left NC
}
components.append(dict(ref='U1', lib_id='RF_Module:ESP32-S3-WROOM-1', x=140, y=140, value='ESP32-S3-WROOM-1', net=esp32_net))

# U2 MCP73831 charger
components.append(dict(ref='U2', lib_id='Battery_Management:MCP73831-2-OT', x=40, y=40, value='MCP73831T-2ACI/OT',
    net={'STAT':'CHG_STAT', 'V_{SS}':'GND', 'V_{BAT}':'VBAT', 'V_{DD}':'USB_VBUS', 'PROG':'PROG_SET'}))

# U3 MCP1700 3.3V regulator (symbol is the 3.0V base part graphics; real BOM
# part is the -330 (3.3V) trim, same pinout -- see doc note)
components.append(dict(ref='U3', lib_id='Regulator_Linear:MCP1700x-300xxTT', x=90, y=40, value='MCP1700T-3302E/TT (3.3V, see note)',
    net={'GND':'GND', 'VO':'3V3', 'VI':'VBAT'}))

# U4 MAX98357A speaker amp
components.append(dict(ref='U4', lib_id='Audio:MAX98357A', x=220, y=40, value='MAX98357A',
    net={'DIN':'SPEAKER_DOUT', 'GAIN_SLOT':'GND', 'GND':'GND', '~{SD_MODE}':'3V3',
         'VDD':'3V3', 'LRCLK':'SPEAKER_WS', 'BCLK':'SPEAKER_BCLK',
         'OUTP':'SPEAKER_P', 'OUTN':'SPEAKER_N', 'NC': None, 'PAD':'GND'}))

# U6 MCP100 voltage supervisor -- drives BATTERY_LOW_PIN. Symbol is the
# base part's graphics (MCP100-270D, 2.70V threshold); the real BOM part
# is the -300 variant (3.00V threshold), same pinout, see the "extends"
# note near LIB_SOURCES. Monitors VBAT directly (raw battery voltage, not
# the regulated 3V3 rail) so the alert reflects actual charge level.
# Open-drain output needs the R12 pull-up below.
components.append(dict(ref='U6', lib_id='Power_Supervisor:MCP100-270D', x=60, y=100, value='MCP100T-300D/TT (3.0V, see note)',
    net={'~{RST}':'BATTERY_LOW', 'VDD':'VBAT', 'VSS':'GND'}))

# U5 ICS-43434 mic
components.append(dict(ref='U5', lib_id='Sensor_Audio:ICS-43434', x=220, y=100, value='ICS-43434',
    net={'WS':'MIC_WS', 'LR':'GND', 'GND':'GND', 'SCK':'MIC_BCLK', 'VDD':'3V3', 'SD':'MIC_DIN'}))

# J1 BBQ10 keyboard connector (real Hirose pinout)
components.append(dict(ref='J1', lib_id='Fairberry:BBQ10KBD', x=40, y=140, value='BBQ10KBD',
    net={'GND':'GND', 'MIC_VDD':'3V3', 'MIC': None, 'AGND':'GND',
         'ROW1':'KBD_ROW1','ROW2':'KBD_ROW2','ROW3':'KBD_ROW3','ROW4':'KBD_ROW4','ROW5':'KBD_ROW5','ROW6':'KBD_ROW6','ROW7':'KBD_ROW7',
         'COL1':'KBD_COL1','COL2':'KBD_COL2','COL3':'KBD_COL3','COL4':'KBD_COL4','COL5':'KBD_COL5',
         'LEDK_2_3':'KBD_BACKLIGHT_CATHODE','LEDK_1_4':'KBD_BACKLIGHT_CATHODE',
         'LEDA_3_4':'3V3','LEDA_1_2':'3V3'}))

# J2 microSD
components.append(dict(ref='J2', lib_id='Connector:Micro_SD_Card', x=280, y=140, value='Micro_SD_Card',
    net={'DAT2':'SD_DAT2', 'DAT3/CD':'SD_DAT3', 'CMD':'SD_CMD', 'VDD':'3V3', 'CLK':'SD_CLK',
         'VSS':'GND', 'DAT0':'SD_D0', 'DAT1':'SD_DAT1', 'SHIELD':'GND'}))

# J3 battery connector
components.append(dict(ref='J3', lib_id='Connector_Generic:Conn_01x02', x=20, y=20, value='JST-PH 2-pin (battery)',
    net={'1':'VBAT', '2':'GND'}))

# J4 speaker connector
components.append(dict(ref='J4', lib_id='Connector_Generic:Conn_01x02', x=260, y=20, value='Speaker 2-pin',
    net={'1':'SPEAKER_P', '2':'SPEAKER_N'}))

# J5 trackball connector (VCC,GND,UP,DOWN,LEFT,RIGHT,BTN -- see trackball doc; external pull-ups R7-R10 below)
components.append(dict(ref='J5', lib_id='Connector_Generic:Conn_01x07', x=180, y=180, value='Trackball (ICSH044A)',
    net={'1':'3V3','2':'GND','3':'TRACKBALL_UP','4':'TRACKBALL_DOWN','5':'TRACKBALL_LEFT','6':'TRACKBALL_RIGHT','7':'TRACKBALL_BTN'}))

# J6 USB connector (SIMPLIFIED -- generic 4-pin VBUS/D-/D+/GND, not a real
# USB-C receptacle footprint with CC1/CC2 resistors. A real build needs an
# actual USB-C receptacle part + CC pull-down resistors (5.1k to GND each)
# for a source to enable VBUS at all -- flagged here, not implemented, see
# the doc's Known Gaps.)
components.append(dict(ref='J6', lib_id='Connector_Generic:Conn_01x04', x=140, y=20, value='USB (SIMPLIFIED, see note)',
    net={'1':'USB_VBUS', '2':'USB_DM', '3':'USB_DP', '4':'GND'}))

# ---- Passives ----
# Resistors
components.append(dict(ref='R1', lib_id='Device:R', x=40, y=70, value='10k', net={'1':'3V3','2':'EN'}))  # EN pull-up
components.append(dict(ref='R2', lib_id='Device:R', x=60, y=40, value='2k', net={'1':'PROG_SET','2':'GND'}))  # charge current set (~500mA)
components.append(dict(ref='R3', lib_id='Device:R', x=20, y=50, value='1k', net={'1':'3V3','2':'CHG_STAT_LED_A'}))  # STAT LED series R
components.append(dict(ref='R4', lib_id='Device:R', x=170, y=170, value='10k', net={'1':'3V3','2':'TRACKBALL_UP'}))
components.append(dict(ref='R5', lib_id='Device:R', x=180, y=170, value='10k', net={'1':'3V3','2':'TRACKBALL_DOWN'}))
components.append(dict(ref='R6', lib_id='Device:R', x=190, y=170, value='10k', net={'1':'3V3','2':'TRACKBALL_LEFT'}))
components.append(dict(ref='R7', lib_id='Device:R', x=200, y=170, value='10k', net={'1':'3V3','2':'TRACKBALL_RIGHT'}))
components.append(dict(ref='R8', lib_id='Device:R', x=270, y=170, value='10k', net={'1':'3V3','2':'SD_DAT1'}))
components.append(dict(ref='R9', lib_id='Device:R', x=280, y=170, value='10k', net={'1':'3V3','2':'SD_DAT2'}))
components.append(dict(ref='R10', lib_id='Device:R', x=290, y=170, value='10k', net={'1':'3V3','2':'SD_DAT3'}))
components.append(dict(ref='R11', lib_id='Device:R', x=170, y=110, value='220', net={'1':'KBD_BACKLIGHT_DRV','2':'KBD_BACKLIGHT_CATHODE'}))  # backlight current limit
components.append(dict(ref='R12', lib_id='Device:R', x=70, y=90, value='10k', net={'1':'3V3','2':'BATTERY_LOW'}))  # U6's open-drain output needs this pull-up

# Capacitors
components.append(dict(ref='C1', lib_id='Device:C', x=80, y=40, value='1uF', net={'1':'VBAT','2':'GND'}))  # regulator input
components.append(dict(ref='C2', lib_id='Device:C', x=100, y=40, value='1uF', net={'1':'3V3','2':'GND'}))  # regulator output
components.append(dict(ref='C3', lib_id='Device:C', x=150, y=110, value='100nF', net={'1':'3V3','2':'GND'}))  # ESP32 decouple
components.append(dict(ref='C4', lib_id='Device:C', x=160, y=110, value='10uF', net={'1':'3V3','2':'GND'}))  # ESP32 bulk decouple
components.append(dict(ref='C5', lib_id='Device:C', x=230, y=40, value='100nF', net={'1':'3V3','2':'GND'}))  # MAX98357A decouple
components.append(dict(ref='C6', lib_id='Device:C', x=230, y=100, value='100nF', net={'1':'3V3','2':'GND'}))  # ICS-43434 decouple

# LED
components.append(dict(ref='LED1', lib_id='Device:LED', x=20, y=60, value='Charge status', net={'K':'CHG_STAT', 'A':'CHG_STAT_LED_A'}))

print(f"{len(components)} components defined")

# sanity: verify every net_map key exists as a real pin name/number on the part
errors = []
for c in components:
    valid_names = set(p['name'] for p in pin_data[c['lib_id']])
    valid_nums = set(p['num'] for p in pin_data[c['lib_id']])
    for k in c['net']:
        if k not in valid_names and k not in valid_nums:
            errors.append(f"{c['ref']} ({c['lib_id']}): net_map key '{k}' is not a real pin name or number")
if errors:
    print("PIN MAP ERRORS:")
    for e in errors:
        print(" -", e)
else:
    print("All net_map keys match real pin names/numbers. OK.")
