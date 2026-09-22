# Standalone smart keyboard: ESP32-S3 + trackball + voice notes

This supersedes the direction in [Hardware_ESP32_Trackball_Mainboard.md](Hardware_ESP32_Trackball_Mainboard.md) for a bigger pivot: instead of a host-powered accessory, this is a standalone, battery-powered device with its own enclosure that **pairs wirelessly** with the Xteink X4 (or another BLE HID host) rather than physically attaching to it. It keeps the BBQ10 keyboard and the ICSH044A trackball from that earlier doc, and adds a microphone, speaker, microSD storage, a LiPo battery, and Wi-Fi-based voice transcription via the OpenAI Whisper API.

**Status: architecture + firmware, not a fabricated/tested board**, same caveat as the trackball doc before it. Nothing here has touched real hardware. The pin table especially should be treated as a reasoned starting point, not a verified-correct final assignment -- see the note in that section.

## Why ESP32-S3 (changed from the trackball doc's classic ESP32)

The trackball-only doc deliberately stayed on classic ESP32 to avoid an unnecessary chip port. That reasoning doesn't hold anymore:

- **GPIO budget**: mic + speaker (I2S) + SD (SPI) + battery monitoring pushes pin count well past what's comfortable on classic ESP32, especially once the trackball's 4 direction lines are added back in. S3 has significantly more usable GPIO.
- **No more input-only-pin problem**: classic ESP32's GPIO 34/35/36/39 have no internal pull-up/down hardware, which is why the trackball doc needed external pull-up resistors and a workaround pin assignment. S3's GPIO 0-21 are all normal bidirectional pins with full pull capability, which simplifies the trackball wiring too.
- **RAM/PSRAM**: buffering audio for WAV writes and HTTPS JSON responses at the same time wants more RAM than classic ESP32 has to spare. S3 modules with PSRAM give real headroom here.
- **This is also just the chip the earlier "smart ML keyboard" conversation was pointing at** (TFLite Micro / vector instructions for things like trackball gesture recognition), so this consolidates onto one chip instead of maintaining two board types for related features.

This does mean re-deriving parts of the firmware that assumed classic ESP32 (see Firmware section) rather than reusing the trackball board type's `BOARD_TYPE == ESP32` code paths directly.

## Power: LiPo battery, not host-powered

This is a real change of philosophy from the rest of this repo (the USB/Arduino board types are explicitly host-powered with no battery). Get this part right -- LiPo mishandling is a fire risk, not just a design nitpick:

- **Use a protected cell.** A LiPo pouch cell with a built-in protection PCB (over-charge/over-discharge/over-current cutoff) is non-negotiable, not a nice-to-have.
- **Charge IC**: a standard linear charger like the MCP73831 (single-cell, sets charge current via one resistor, JST-PH battery connector) is the well-trodden, low-risk choice here -- it's what the overwhelming majority of small LiPo hobby projects use. Add its STAT pin straight to an LED + resistor for a charge-status indicator; that doesn't need to cost an MCU pin.
- **Fuel gauge (recommended)**: a coulomb-counting gauge like the MAX17048 gives an actual battery-percentage reading over I2C instead of a rough voltage-divider guess, and it's cheap. If skipping it to save cost/pins, fall back to a simple ADC voltage divider on the battery line (~2:1 divider into an ADC-capable GPIO, with the usual caveat that LiPo voltage-to-charge-% curves are nonlinear and this reading will be a rough estimate at best, especially under load).
- **Load switch / deep sleep**: route mic, speaker amp, and SD card power through a MOSFET load switch gated by the MCU, so they can be fully powered down (not just idled) between uses -- this is where the actual battery-life win comes from, more than any peripheral-level low-power mode.
- **Cell size**: pick based on the enclosure size once that's designed; a 500-1000mAh pouch cell is a reasonable starting range for a pocket-sized device with WiFi used briefly once or twice a day, not continuously.

## New peripherals

| Peripheral | Suggested part | Interface |
|---|---|---|
| Microphone | INMP441 (I2S MEMS mic breakout) | I2S, digital output avoids analog noise pickup |
| Speaker amp | MAX98357A (I2S Class-D amp) + small 8Ω speaker | I2S |
| Storage | Standard microSD SPI breakout | SPI |
| Fuel gauge (optional) | MAX17048 | I2C |
| Charger | MCP73831 | Standalone, JST-PH battery connector |

## Pin plan

**This is a starting proposal, not a verified-final assignment.** ESP32-S3 modules vary in which GPIOs are actually free depending on flash/PSRAM configuration (octal PSRAM variants reserve additional pins beyond the always-reserved SPI flash pins) -- confirm against the datasheet for your specific module (e.g. ESP32-S3-WROOM-1 variant) before wiring anything. The plan below avoids the pins that are *always* reserved (SPI flash, UART0, native USB D+/D-, strapping pins 0/3/45/46) but does not assume a specific PSRAM configuration.

32 GPIOs needed against roughly 25-30 usable on a typical S3 module (depending on PSRAM configuration) -- tight, but it fits with nothing left over for surprises:

| Function | Pins | Notes |
|---|---|---|
| Keyboard rows (7) | 1,2,4,5,6,7,8 | |
| Keyboard cols (5) | 9,10,11,12,13 | |
| Keyboard backlight | 14 | |
| Trackball UP/DOWN/LEFT/RIGHT | 15,16,17,18 | All normal GPIOs on S3 -- internal pull-ups usable, but keep external pull-ups too per the trackball doc's polarity caveat |
| Trackball BTN | 21 | Doubles as the deep-sleep wake button (RTC-capable pin, `esp_sleep_enable_ext0_wakeup`) -- no separate wake button needed |
| Trackball LEDs R/G/B | 22,23,24 | Optional |
| Mic I2S (WS/BCLK/DIN) | 25,33,34 | Dedicated I2S port, not shared with speaker |
| Speaker I2S (WS/BCLK/DOUT) | 35,36,37 | Separate I2S port from the mic rather than a shared-clock setup, simpler to get right |
| SD SPI (CS/MOSI/MISO/SCK) | 38,39,40,41 | |
| Battery voltage ADC | 42 | Skip if using the MAX17048 fuel gauge (I2C) instead, freeing this pin |
| *(spare)* | 47,48 | Unused headroom -- e.g. an I2C bus for the fuel gauge, or swap in for 33-37 below |

GPIO 33-37 (used above for mic+speaker I2S) are the ones that may be reserved on octal-PSRAM module variants -- **check this against your specific module's datasheet first.** If they're reserved on your board, swap the mic/speaker I2S pins with the spare 47/48 plus reclaim one more from elsewhere, or drop to a non-PSRAM module (at the cost of the RAM headroom PSRAM was chosen for in the first place).

If you're hand-wiring a prototype rather than laying out a PCB, get the mic+speaker+SD+keyboard+trackball working incrementally on a breadboard against whatever your specific module actually exposes, rather than treating this table as gospel.

## Firmware

New modules, all opt-in and independent of the existing keyboard/trackball code paths:

- `BBQ10/storage.h` -- SD card init and file helpers.
- `BBQ10/audio.h` -- I2S mic capture to WAV on SD, I2S speaker playback, confirmation tones.
- `BBQ10/whisper_sync.h` -- Wi-Fi connection, SNTP time, scanning SD for un-transcribed recordings, uploading to the OpenAI Whisper API, writing the resulting `.txt` next to the `.wav`. Manual trigger (a key combo) and a once-daily scheduled sync.

Enabled via `WIFI_TRANSCRIPTION_ENABLED` in `configuration.h`. WiFi credentials and the OpenAI API key live in `BBQ10/secrets.h`, which is gitignored -- copy `BBQ10/secrets.h.example` and fill in your own values, never commit real credentials.

Required additional Arduino library: **ArduinoJson** (for parsing the Whisper API response), on top of the libraries already listed in [Hardware_Fairberry_Mainboard.md](Hardware_Fairberry_Mainboard.md).

### What this does and doesn't do

- Records audio locally to SD as WAV. This part works fully offline.
- Playback of stored recordings and short confirmation tones, also fully offline.
- Transcription requires Wi-Fi and an OpenAI API key, and happens only when a sync runs (manual trigger or once daily) -- it is not real-time, and it depends on an external paid API. Recordings made with no Wi-Fi in range just wait on the SD card until the next successful sync.
- There's no on-device speech recognition of any kind here -- see the earlier conversation in this repo's history for why (a real ASR model doesn't fit in a microcontroller's RAM budget). If you want offline transcription, that's a fundamentally different, much heavier approach (e.g. relaying audio to a more powerful paired host), not something this firmware attempts.

## Enclosure

Not designed yet. The brief (screwless snap-fit two-half case, internal alignment pins for the board and buttons, rounded edges) is a reasonable, standard approach for a small battery-powered gadget, but the actual dimensions depend on the final PCB size and battery footprint, which don't exist yet -- there's no board layout to design a case around. This is real follow-up work once the electronics are prototyped on a breadboard/perfboard and a rough size is known, not something to draw blind.
