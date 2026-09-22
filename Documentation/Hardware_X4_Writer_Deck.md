# Writer deck: Fairberry keyboard + X4 running FreeInk SDK

This ties together everything else in this repo into one system: the [standalone smart keyboard](Hardware_Standalone_Smart_Keyboard.md) as the physical input device, and a FreeInk-SDK-based writer app running on an Xteink X4 as the screen/storage/editor -- a distraction-free writing device in the vein of a Freewrite or AlphaSmart, but built from this project's own parts.

**Status: the least proven part of this whole repo.** Everything about the keyboard side (BLE, trackball, voice typing) is firmware I could reason through carefully. The X4 side is different: I read FreeInk SDK's public docs site but never had the actual SDK source, a compiler, or hardware in front of me. `X4Firmware/src/main.cpp` is built against the *documented* API (confirmed: `EInkDisplay`, `BoardConfig`, `SDCardManager`, `BleKeyboardHost`/`BleHid`, and critically FreeInkUI's `textArea` component, which is described almost exactly as what a text editor needs) but several exact field/method names are my best reconstruction from prose docs, not verified against headers -- each is marked `// VERIFY:` in the source. Expect to fix compile errors against your actual `freeink-sdk` checkout, not just flash and go.

## Architecture

Two independent pieces of firmware, paired over BLE:

1. **Keyboard** (`../BBQ10`, `BOARD_TYPE FAIRBERRY_ESP32S3_SMART`) -- physical typing, trackball, mic/voice typing, all already built. It doesn't know or care that the host is running FreeInk SDK specifically; it's a generic BLE HID keyboard.
2. **X4 firmware** (`../X4Firmware`, this doc) -- a FreeInk SDK application that acts as a BLE HID **host** and renders a writer app: a library screen (browse/create documents on the X4's SD card) and an editor screen (`textArea` widget, autosaved).

They're linked at the protocol level (BLE HID) and at the build-flag level: the X4 firmware needs `FREEINK_CAP_BLE_HID_HOST=1` and the NimBLE dependency enabled, exactly as documented in the [main README's Status section](../README.md#status--known-limitation) and reproduced in `X4Firmware/platformio.ini`.

They can also be physically joined into one device instead of two BLE-paired pieces -- see [Case/WriterDeck/](../Case/WriterDeck) for a combined clamshell case, built by reusing a real interlocking hinge mechanism from a user-provided reference model rather than designing one from scratch. Electronics stay independent (separate boards, separate batteries, still talking over BLE) -- it's purely a shared enclosure.

## Setup

1. Clone `shelbeeely/freeink-sdk` next to this repo (or adjust the path in `X4Firmware/platformio.ini`'s `lib_deps`).
2. `cd X4Firmware && pio run -e xteink_x4 -t upload` -- expect to need to fix the `VERIFY:` spots in `src/main.cpp` against your actual checkout first.
3. Build and flash the keyboard firmware per [Hardware_Standalone_Smart_Keyboard.md](Hardware_Standalone_Smart_Keyboard.md).
4. Pair them. Bluetooth pairing UX on the X4 side isn't implemented in this pass -- see Known Gaps.

## What's implemented in `X4Firmware/src/main.cpp`

- Library screen: lists `/documents` on the X4's SD card, `+ New draft` starts a blank numbered file.
- Editor screen: a `textArea`-based distraction-free editor. Typing on the Fairberry keyboard inserts/deletes text at the caret; Left/Right arrows move the caret; Escape saves and returns to the library.
- Autosave every 5 seconds while actively typing.
- A periodic full e-ink refresh every 40 edits to clear ghosting from repeated fast partial updates, rather than full-refreshing on every keystroke (which would be slow and flashy) or never (which would leave visible ghosting).

## Known gaps (not implemented, not guessed at)

- **Up/Down arrow line navigation.** Needs `textArea` to expose which visual line/column the caret is on after word-wrap, which wasn't in the docs I could read. Left/Right plus the editor's own scrolling covers basic editing for now.
- **BLE pairing flow on the X4 side.** `BleHid.begin()` starts the host role, but the actual "scan, show discovered devices, let the user pick and pair" UX isn't built -- lib-ble's docs describe `startScan()`/`device()`/`connect()`/`takePairingPasskey()` for this, but wiring it into an actual pairing screen wasn't attempted here (this needs its own UI, out of scope for this pass).
- **On-screen keyboard fallback.** FreeInkUI has `keyboard`/`qwertyKeyboard` components for text input without the physical keyboard connected -- not wired in, since the entire point of this device is typing on the physical Fairberry keyboard, but it's a reasonable addition later (e.g. renaming a file, entering WiFi credentials).
- **Large documents.** The document buffer is a single in-RAM `String`. Fine for typical short-form writing-deck use, not designed for book-length documents -- a chunked/streaming buffer would be a real rewrite of the editor, not attempted here.
- **Integration with the keyboard's own voice-typing/transfer-mode features.** Those exist entirely on the keyboard side and just inject keystrokes -- from the X4 app's perspective a voice-typed sentence arrives as ordinary keystrokes into whatever's focused (the editor, if that's what's open), no special handling needed or added.
