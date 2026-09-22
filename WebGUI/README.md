# Web GUI

The fourth component: a local browser dashboard for browsing/downloading voice notes and transcripts, served directly by the keyboard's own firmware over WiFi (no cloud, no separate server to run).

**The implementation lives in `../BBQ10/web_server.h`, not in this directory** -- it has to be compiled into the keyboard firmware to be served by it (the ESP32 *is* the web server), so it can't be a truly separate deployable app the way this directory's name might suggest. This README exists to document it as its own logical component per the repo's four-part split (keyboard firmware / X4 firmware / hardware / web GUI), and as the place to grow it if it outgrows a single header file.

See [Hardware_Standalone_Smart_Keyboard.md's transfer mode section](../Documentation/Hardware_Standalone_Smart_Keyboard.md#local-transfer-mode-no-cloud) for what it does, how to enable it (`TRANSFER_MODE_ENABLED`), and its known gaps (no tag filtering, no HTTP Range support for audio scrubbing).

## How it interlinks with the other three components

- **Keyboard firmware** (`../BBQ10`): hosts this. `SYM + T` toggles it on/off.
- **X4 firmware** (`../X4Firmware`): not directly connected to this -- the dashboard is reached from any browser on the same WiFi network (a phone, a laptop), not through the X4. Since the keyboard has no screen of its own, activating it types the dashboard's URL as keystrokes into whatever's focused on the paired host (see `../BBQ10/web_server.h`), which in the writer-deck setup would typically be the X4's editor.
- **Hardware** (`../KiCad`, `../Case`): no direct relationship -- this is pure firmware/software.

## If this grows into something bigger

Right now the dashboard is a single generated HTML string in `web_server.h` -- fine for its current scope (a file list with download/stream links). If it grows into something with real interactivity (editing tags, renaming files, a nicer UI), the natural next step is extracting real `.html`/`.css`/`.js` files into this directory and either serving them from SD (simple, but uses SD card space and a file read per request) or embedding them into the firmware binary at build time (keeps them servable without SD access, but needs a build step to generate the embedded data). Neither is done yet -- not needed for the current feature set.
