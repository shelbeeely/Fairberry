# Fairberry (e-ink fork)

This is a personal fork of [Dakkaron/Fairberry](https://github.com/Dakkaron/Fairberry), which adds a detachable BlackBerry Q10 keyboard to phones. If you want the original phone-focused project (Fairphone 4, Samsung Galaxy A54, etc.), go there instead.

**The goal of this fork is different: get the same physical Q10 keyboard working with e-ink devices, starting with my own [Xteink X4](https://www.xteink.com/products/xteink-x4), and generally with any device that runs the [Free Ink SDK](https://freeink.org/) ([Free-Ink/freeink-sdk](https://github.com/Free-Ink/freeink-sdk)).**

The keyboard hardware and the mainboard firmware don't actually know or care what they're plugged into — the mainboard scans the Q10 keyboard's key matrix and outputs the result as either a standard USB HID keyboard (Arduino/custom-mainboard board types) or a Bluetooth LE HID keyboard (ESP32 board type). Retargeting this project at e-ink readers is mostly about:

1. A case/mount that fits the reader instead of a phone.
2. Confirming (and where needed, contributing) input support on the reader side, since that's device/firmware-specific.

## Status / known limitation

Good news on the software side: FreeInk SDK has a purpose-built BLE keyboard host — [`BleKeyboardHost`](https://freeink.org/docs/lib-ble) (the `lib-ble` library). Per its docs, it's a real BLE HID **host** (central role) that explicitly supports "keyboards, page turners, remote buttons and similar devices that expose the HID service" — not just page-turner remotes as I originally assumed here. It works on both ESP32-C3 and ESP32-S3 (BLE only, no Bluetooth Classic), which covers the plain X4 (C3) as well as the X4 Classic/Pro (S3). Pairing defaults to "Just Works" (no passkey needed), which is what a keyboard-only peripheral like Fairberry's `BOARD_TYPE ESP32` needs.

The catch: it's an **opt-in capability**, gated behind the `FREEINK_CAP_BLE_HID_HOST` build flag (off by default) plus adding the NimBLE stack as a dependency. So the real question isn't "does the ecosystem support external keyboards" (it does) — it's "does the specific firmware image flashed on your X4 have that flag turned on." Stock/vendor Xteink firmware almost certainly doesn't.

Checked directly against [freeink-sdk's `platformio.sample.ini`](https://github.com/Free-Ink/freeink-sdk/blob/main/platformio.sample.ini): the stock `[env:xteink_x4]` build env is just

```ini
[env:xteink_x4]
extends = base
board = esp32-c3-devkitm-1
build_flags =
  ${base.build_flags}
  -DFREEINK_DEVICE_X4=1
```

— no BLE HID host flag. Building a FreeInk-SDK firmware for the X4 that can actually pair with a Fairberry keyboard means adding, at minimum:

```ini
build_flags =
  ${base.build_flags}
  -DFREEINK_DEVICE_X4=1
  -DFREEINK_CAP_BLE_HID_HOST=1
lib_deps =
  ${base.lib_deps}
  h2zero/NimBLE-Arduino@^2.3.8
```

to your own build, then flashing that instead of whatever firmware is on the device now.

I have **not yet confirmed** this end-to-end against a physical X4 — that's the next step. I also haven't found documented USB HID host support in FreeInk SDK for any target, so the USB-attached Fairberry board types (`FAIRBERRY_V0_3_0`, `ARDUINO`) are more speculative than the BLE path right now; BLE (`BOARD_TYPE ESP32`) is the one with an actual matching host-side API to target.

## What can you expect?

Everything from the upstream project still applies at the electronics level: same BBQ10 keyboard, same mainboard, same firmware, same key combos (cursor mode, sticky modifiers, dead keys, backlight controls — see [UX_Shortcuts_and_Apps.md](Documentation/UX_Shortcuts_and_Apps.md)). What's changing here is the case/mount and the target device.

Demo video of the original phone version: [![Demo video showing the functionality of the Fairberry keyboard](https://img.youtube.com/vi/iDb8_ld9gOQ/0.jpg)](https://www.youtube.com/watch?v=iDb8_ld9gOQ)

## How to build it?

Recommended: [Custom mainboard](Documentation/Hardware_Fairberry_Mainboard.md) — use `BOARD_TYPE FAIRBERRY_V0_3_0` (or newer) for a USB-attached keyboard, or `BOARD_TYPE ESP32` for a BLE-attached one.

[Old Arduino-based hardware](Documentation/Hardware_Arduinobased.md) also works, same caveats apply.

For the case, use the [Xteink X4 preset](Case/Generator/presets/XteinkX4.scad) as a starting point — see the note at the top of that file, it's built from published X4 dimensions and has **not** been verified against a physical device yet. The original phone presets (Fairphone 4, Samsung Galaxy A54) are still in the repo under `Case/Generator/presets/` in case they're useful as a reference for another device.

## How to use it?

[Key combinations and apps](Documentation/UX_Shortcuts_and_Apps.md)

## License

Same as upstream: [Attribution-NonCommercial-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-nc-sa/4.0/) for the firmware and case files (see individual file headers), original work © Dakkaron.
