# Fairberry (e-ink fork)

This is a personal fork of [Dakkaron/Fairberry](https://github.com/Dakkaron/Fairberry), which adds a detachable BlackBerry Q10 keyboard to phones, physically clamped on and wired over USB. If you want that original phone-focused project, go there instead — this fork has moved away from it.

**The direction here now: a standalone, battery-powered BlackBerry-style device -- Q10 keyboard, a five-way trackball, a mic and speaker, microSD storage -- that pairs *wirelessly* with the [Xteink X4](https://www.xteink.com/products/xteink-x4) (or any BLE HID host) instead of physically attaching to it.** No separate battery / host-powered-only was the original project's whole philosophy; this fork has deliberately dropped that in favor of its own LiPo battery and its own enclosure, built primarily for e-ink devices running the [Free Ink SDK](https://freeink.org/) but not exclusive to them -- anything that speaks BLE HID keyboard is a valid target.

See [Hardware_Standalone_Smart_Keyboard.md](Documentation/Hardware_Standalone_Smart_Keyboard.md) for the current primary build. Two earlier, simpler stages are still in the repo and still valid if you want something less involved:
1. [Hardware_Fairberry_Mainboard.md](Documentation/Hardware_Fairberry_Mainboard.md) -- the original USB/BLE keyboard-only mainboard.
2. [Hardware_ESP32_Trackball_Mainboard.md](Documentation/Hardware_ESP32_Trackball_Mainboard.md) -- adds the trackball, still host-powered, classic ESP32.

The standalone smart keyboard supersedes both for pairing with the X4 specifically, but they're simpler builds if you don't want the battery/mic/speaker/SD/WiFi scope.

## Writer deck: four components, one repo

The keyboard alone is one half of the picture. Combined with an X4 running a FreeInk-SDK writer app, this becomes a full distraction-free writing device -- a "writer deck," in the vein of a Freewrite or AlphaSmart, built from this project's own parts. Four separate-but-interlinked pieces, all in this repo:

| Component | Directory | What it is |
|---|---|---|
| Keyboard firmware | [`BBQ10/`](BBQ10) | The physical keyboard + trackball + voice typing, BLE HID |
| X4 firmware | [`X4Firmware/`](X4Firmware) | The writer app (library + editor screens) running on the X4 |
| Hardware | [`KiCad/`](KiCad), [`Case/`](Case) | PCB design, 3D-printable case |
| Web GUI | [`WebGUI/`](WebGUI) | The keyboard-hosted local browser dashboard (implementation lives in `BBQ10/web_server.h`) |

Start with [Hardware_X4_Writer_Deck.md](Documentation/Hardware_X4_Writer_Deck.md) for how the pieces fit together -- it's also the most honestly unproven part of this repo (see its Status section: I read FreeInk SDK's docs site, but never had the actual SDK source or hardware in front of me to compile/test against, unlike the keyboard firmware).

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

**Standalone smart keyboard (primary build)**: `BOARD_TYPE FAIRBERRY_ESP32S3_SMART` on an ESP32-S3. See [Hardware_Standalone_Smart_Keyboard.md](Documentation/Hardware_Standalone_Smart_Keyboard.md) for the pin plan, BOM (mic, speaker, microSD, LiPo + charger), firmware, and key combo table. Opt-in flags in `configuration.h`: `TRACKBALL_ENABLED`, `AUDIO_ENABLED` (local recording/playback, fully offline), `WIFI_TRANSCRIPTION_ENABLED` (batch-syncs recordings to the OpenAI Whisper API), `TRANSFER_MODE_ENABLED` (local no-cloud web dashboard for browsing/downloading notes over LAN), `VOICE_TYPING_ENABLED` (immediate dictate -> hear it read back via TTS to verify -> Enter to send, like a phone's voice-to-text keyboard). The last three need `BBQ10/secrets.h` -- copy `secrets.h.example`. No enclosure design yet -- see that doc's Enclosure section for why. **None of this is validated against real hardware.**

**Simpler builds**, if you don't want the battery/audio/WiFi scope:
- [Custom mainboard](Documentation/Hardware_Fairberry_Mainboard.md) — `BOARD_TYPE FAIRBERRY_V0_3_0` (USB) or `BOARD_TYPE ESP32` (BLE), keyboard only.
- [+ trackball](Documentation/Hardware_ESP32_Trackball_Mainboard.md) — same `BOARD_TYPE ESP32`, adds `TRACKBALL_ENABLED`.
- [Old Arduino-based hardware](Documentation/Hardware_Arduinobased.md) also still works.

For these simpler, host-attached builds only (not the standalone smart keyboard, which has no case yet), there's a case preset: [Xteink X4 preset](Case/Generator/presets/XteinkX4.scad), built from width/thickness measured off a community-made, to-scale device model ([Zorian22's "Xteink X4 device model" on Thingiverse](https://www.thingiverse.com/thing:7287950), 114.2 x 69.2 x 6.2mm) rather than just the rounded marketing spec — better, but still not a full match to the real device shape. The preset file has step-by-step instructions for swapping in that actual dummy model file for a proper fit instead of the parametric approximation; I couldn't fetch it automatically since Thingiverse's terms block bots, so that step needs a normal browser download on your end. Print a bottom-only test fit before committing to a full case either way. The original phone presets (Fairphone 4, Samsung Galaxy A54) are still in the repo under `Case/Generator/presets/` for reference.

**Writer deck combined case**: [Case/WriterDeck/](Case/WriterDeck) -- a single clamshell enclosing both the keyboard and the X4 as one physical device, reusing a real interlocking hinge mechanism extracted from a user-provided reference model rather than designed from scratch. First-draft OpenSCAD, not print-tested -- see that directory's README for exactly what's confirmed-real (the hinge geometry, the keyboard module dimensions) versus still a placeholder (tray cavity depth, trackball position).

## How to use it?

[Key combinations and apps](Documentation/UX_Shortcuts_and_Apps.md)

## License

Same as upstream: [Attribution-NonCommercial-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-nc-sa/4.0/) for the firmware and case files (see individual file headers), original work © Dakkaron.
