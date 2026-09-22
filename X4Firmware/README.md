# X4 firmware

The screen/storage/editor half of the [writer deck](../Documentation/Hardware_X4_Writer_Deck.md) -- a FreeInk SDK application for the Xteink X4 that pairs with the [Fairberry keyboard](../BBQ10) over BLE HID and provides a distraction-free writing app (library + editor screens, built on FreeInkUI's `textArea` component, autosaved to the X4's SD card).

Read [Documentation/Hardware_X4_Writer_Deck.md](../Documentation/Hardware_X4_Writer_Deck.md) first -- it covers the architecture, setup steps, and (important) exactly which parts of `src/main.cpp` are confirmed against FreeInk SDK's documented API versus best-effort reconstruction that needs checking against your actual `freeink-sdk` checkout (marked `// VERIFY:` in the source).

This is a separate PlatformIO project from `freeink-sdk` itself -- see the setup instructions at the top of `platformio.ini`.
