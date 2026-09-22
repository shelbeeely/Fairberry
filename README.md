# Fairberry (e-ink fork)

This is a personal fork of [Dakkaron/Fairberry](https://github.com/Dakkaron/Fairberry), which adds a detachable BlackBerry Q10 keyboard to phones. If you want the original phone-focused project (Fairphone 4, Samsung Galaxy A54, etc.), go there instead.

**The goal of this fork is different: get the same physical Q10 keyboard working with e-ink devices, starting with my own [Xteink X4](https://www.xteink.com/products/xteink-x4), and generally with any device that runs the [Free Ink SDK](https://freeink.org/) ([Free-Ink/freeink-sdk](https://github.com/Free-Ink/freeink-sdk)).**

The keyboard hardware and the mainboard firmware don't actually know or care what they're plugged into — the mainboard scans the Q10 keyboard's key matrix and outputs the result as either a standard USB HID keyboard (Arduino/custom-mainboard board types) or a Bluetooth LE HID keyboard (ESP32 board type). Retargeting this project at e-ink readers is mostly about:

1. A case/mount that fits the reader instead of a phone.
2. Confirming (and where needed, contributing) input support on the reader side, since that's device/firmware-specific.

## Status / known limitation

As of writing, I have **not confirmed** that a Free Ink SDK device will accept input from an external keyboard out of the box:

- FreeInk SDK's documented BLE HID Central ("host") support is scoped to page-turner remotes on the OnePage Reader hardware target — it is not documented as generic BLE HID keyboard support, and I haven't verified it works with a full keyboard.
- I have not found documentation of USB HID host support in FreeInk SDK for any target, including the Xteink X4 / X4 Classic.
- The Xteink X4 Classic uses an ESP32-S3 (which has a USB OTG peripheral capable of host mode in hardware), while the plain Xteink X4 uses an ESP32-C3 (no full USB host peripheral) — so USB HID host support, if it ever lands, would likely be Classic-only.

So getting a Fairberry keyboard fully working with an X4 currently means either testing what actually happens when you pair/plug one in, or contributing keyboard-host support to freeink-sdk. This fork tracks that work; PRs/issues on input support are welcome upstream too.

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
