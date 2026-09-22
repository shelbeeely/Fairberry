#if BOARD_TYPE == FAIRBERRY_V0_1_1 || BOARD_TYPE == FAIRBERRY_V0_2_0 || BOARD_TYPE == FAIRBERRY_V0_3_0 || BOARD_TYPE == ARDUINO || BOARD_TYPE==BEETLE
  #define CHIP_TYPE CHIP_ATMEGA32U4
#elif BOARD_TYPE == ESP32 || BOARD_TYPE == FAIRBERRY_ESP32S3_SMART
  #define CHIP_ESP32
#else
  #error "No valid BOARD_TYPE selected. Needs to be any of FAIRBERRY_V0_1_1, FAIRBERRY_V0_2_0, FAIRBERRY_V0_3_0, ARDUINO, BEETLE, ESP32, FAIRBERRY_ESP32S3_SMART"
#endif

#if defined(POWERSAVE_ARDUINO_IDLE) && defined(POWERSAVE_ARDUINO_POWERDOWN)
  #error "Cannot have POWERSAVE_ARDUINO_IDLE and POWERSAVE_ARDUINO_POWERDOWN at the same time!"
#endif

#if CHIP_TYPE != CHIP_ATMEGA32U4 && defined(POWERSAVE_ARDUINO_IDLE)
  #error "Only boards with ATMEGA32U4 chips suport POWERSAVE_ARDUINO_IDLE!"
#endif

#if CHIP_TYPE != CHIP_ATMEGA32U4 && defined(POWERSAVE_ARDUINO_POWERDOWN)
  #error "Only boards with ATMEGA32U4 chips suport POWERSAVE_ARDUINO_POWERDOWN!"
#endif

#if defined(POWERSAVE_ARDUINO_CLOCKDOWN) && !defined(ARDUINO_CLOCKDOWN_DIVISION)
  #error "ARDUINO_CLOCKDOWN_DIVISION needs to be set (to 1,2,4,8,16,32,64,128 or 256) if POWERSAVE_ARDUINO_CLOCKDOWN is enabled"
#endif

#ifdef ARDUINO_CLOCKDOWN_DIVISION
  #define ARDUINO_CLOCKDOWN_RESET_VALUE 0b00000000
  #if ARDUINO_CLOCKDOWN_DIVISION == 1
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000000
  #elif ARDUINO_CLOCKDOWN_DIVISION == 2
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000001
  #elif ARDUINO_CLOCKDOWN_DIVISION == 4
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000010
  #elif ARDUINO_CLOCKDOWN_DIVISION == 8
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000011
  #elif ARDUINO_CLOCKDOWN_DIVISION == 16
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000100
  #elif ARDUINO_CLOCKDOWN_DIVISION == 32
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000101
  #elif ARDUINO_CLOCKDOWN_DIVISION == 64
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000110
  #elif ARDUINO_CLOCKDOWN_DIVISION == 128
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00000111
  #elif ARDUINO_CLOCKDOWN_DIVISION == 256
    #define ARDUINO_CLOCKDOWN_DIVISION_VALUE 0b00001000
  #endif
#endif

#if BOARD_TYPE == FAIRBERRY_V0_1_1 || BOARD_TYPE == FAIRBERRY_V0_2_0 || BOARD_TYPE == FAIRBERRY_V0_3_0
  #define F_CPU 16000000
  #define TX_RX_LED_INIT  DDRE |= (1<<6), DDRB |= (1<<0)
  #define TXLED0      PORTE |= (1<<6)
  #define TXLED1      PORTE &= ~(1<<6)
  #define RXLED0      PORTB |= (1<<0)
  #define RXLED1      PORTB &= ~(1<<0)
#endif

#if CHIP_TYPE == CHIP_ATMEGA32U4
  #if !defined(DEBUG_SERIAL_INSTEAD_OF_USB)
    #include "Keyboard.h"
  #endif
  #include "LowPower.h"
  #include <avr/power.h>

  #ifdef DEBUG_SERIAL_INSTEAD_OF_USB
    #define KEYBOARD_BEGIN(layout) Serial.begin()
    #define KEYBOARD_BEGIN(layout) ;
    #define KEYBOARD_PRESS(key) Serial.print("Pressed ");Serial.println(key);
    #define KEYBOARD_RELEASE(key) Serial.print("Released ");Serial.println(key);
  #else
    #define KEYBOARD_BEGIN(layout) Keyboard.begin(layout)
    #define KEYBOARD_END() Keyboard.end()
    #define KEYBOARD_PRESS(key) Keyboard.press(key)
    #define KEYBOARD_RELEASE(key) Keyboard.release(key)
  #endif
#elif BOARD_TYPE == ESP32 || BOARD_TYPE == FAIRBERRY_ESP32S3_SMART
  #include "analogWrite.h"
  #define USE_NIMBLE
  #include <BleKeyboard.h>
  #include "esp_bt.h"
  #include "esp_pm.h"
  #include "esp_wifi.h"
  BleKeyboard bleKeyboard;

  #define SLEEP_DURATION 120000L
  #define KEYBOARD_BEGIN(layout) bleKeyboard.begin()
  #define KEYBOARD_END() bleKeyboard.end()
  #define KEYBOARD_PRESS(key) bleKeyboard.press(key)
  #define KEYBOARD_RELEASE(key) bleKeyboard.release(key)
#endif

#if BOARD_TYPE == FAIRBERRY_V0_3_0
  byte rows[] = {A1,7,8,9,10,5,A0};
  //byte cols[] = {4,30,1,12,6}; // Arduino pinout for reference only
  byte cols[] = {4,5,3,6,7}; // Using PORTD numbering instead of Arduino numbering

  #define KEYBOARD_LIGHT_PIN_1 13
  #define KEYBOARD_LIGHT_PIN_2 11

  #define FASTCHECK
  #define FASTROWCHECK_B 0b01110000
  #define FASTROWCHECK_C 0b01000000
  #define FASTROWCHECK_E 0b01000000
  #define FASTROWCHECK_F 0b11000000

  #define FASTCOLCHECK_D 0b11111000
  
  #define FASTCOLS
  #define FASTCOLS_PORT PORTD
  #define FASTCOLS_PINCONFIG DDRD
#elif BOARD_TYPE == FAIRBERRY_V0_2_0
  byte rows[] = {A1,7,8,9,10,5,A0};
  //byte cols[] = {4,30,1,12,6}; // Arduino pinout for reference only
  byte cols[] = {4,5,3,6,7}; // Using PORTD numbering instead of Arduino numbering

  #define KEYBOARD_LIGHT_PIN_1 13
  #define LED0_PIN 0
  #define LED1_PIN 11

  #define FASTCHECK
  #define FASTROWCHECK_B 0b01110000
  #define FASTROWCHECK_C 0b01000000
  #define FASTROWCHECK_E 0b01000000
  #define FASTROWCHECK_F 0b11000000

  #define FASTCOLCHECK_D 0b11111000
  
  #define FASTCOLS
  #define FASTCOLS_PORT PORTD
  #define FASTCOLS_PINCONFIG DDRD
#elif BOARD_TYPE == FAIRBERRY_V0_1_1
  byte rows[] = {A1,A2,8,9,10,5,A0};
  //byte cols[] = {4,30,1,12,6}; // Arduino pinout for reference only
  byte cols[] = {4,5,3,6,7}; // Using PORTD numbering instead of Arduino numbering

  #define FASTCHECK
  #define FASTROWCHECK_B 0b01110000
  #define FASTROWCHECK_C 0b01000000
  #define FASTROWCHECK_F 0b11100000

  #define FASTCOLCHECK_D 0b11111000

  #define KEYBOARD_LIGHT_PIN_1 13
  #define LED0_PIN 0
  #define LED1_PIN 11
  
  #define FASTCOLS
  #define FASTCOLS_PORT PORTD
  #define FASTCOLS_PINCONFIG DDRD
#elif BOARD_TYPE == ARDUINO
  byte rows[] = {9,8,7,6,5,4,A2};
  byte cols[] = {A1,A0,15,14,16};

  #define KEYBOARD_LIGHT_PIN_1 10
#elif BOARD_TYPE == BEETLE
  byte rows[] = {A5,11,10,9,18,19,20};
  byte cols[] = {3,2,0,14,15};

  #define KEYBOARD_LIGHT_PIN_1 10
#elif BOARD_TYPE == ESP32
  #define RESET_PIN 33
  byte rows[] = {27,25,32,4,0,2,22};
  byte cols[] = {5,23,19,18,26};

  // NOTE: this used to be GPIO35, which is one of the ESP32's input-only
  // ADC pins (34/35/36/39) -- it has no output driver at all, so PWM
  // backlight control on that pin was never actually possible. Moved to
  // GPIO13 (a normal bidirectional pin) so this works, and to free up 35
  // for the trackball's RIGHT line below.
  #define KEYBOARD_LIGHT_PIN_1 13

  // ICSH044A / SparkFun-style BlackBerry trackball, see trackball.h and
  // Documentation/Hardware_ESP32_Trackball_Mainboard.md. Only meaningful
  // if TRACKBALL_ENABLED is defined in configuration.h.
  //
  // UP/DOWN/LEFT/RIGHT are on the ESP32's 4 input-only pins (34/35/36/39)
  // deliberately -- they have no internal pull-up, so these need an
  // external pull-up resistor to 3.3V per line (see the hardware doc).
  // BTN uses a normal pin with the ESP32's internal pull-up instead,
  // since only 4 input-only pins exist and BTN was the 5th line.
  #define TRACKBALL_UP_PIN 34
  #define TRACKBALL_DOWN_PIN 36
  #define TRACKBALL_LEFT_PIN 39
  #define TRACKBALL_RIGHT_PIN 35
  #define TRACKBALL_BTN_PIN 14

  // LEDs are optional (TRACKBALL_LED_ENABLED). WHT isn't wired up here --
  // it wasn't needed for a functional trackball and there wasn't a 4th
  // free flexible pin left in this pin map; RGB driven together
  // approximates white if you want that instead of adding a pin for it.
  #define TRACKBALL_LED_RED_PIN 16
  #define TRACKBALL_LED_GRN_PIN 17
  #define TRACKBALL_LED_BLU_PIN 21
#elif BOARD_TYPE == FAIRBERRY_ESP32S3_SMART
  // Standalone battery-powered board: keyboard + trackball + mic/speaker/SD
  // + WiFi transcription. See
  // Documentation/Hardware_Standalone_Smart_Keyboard.md for the full
  // reasoning and BOM.
  //
  // CORRECTED pin map: the previous version of this table assumed the
  // ESP32-S3-WROOM-1 module exposes GPIO22-25 and GPIO33-34 as ordinary
  // pins (true on the classic ESP32, not true here). Verified against
  // the actual KiCad symbol for this module (RF_Module:ESP32-S3-WROOM-1)
  // in KiCad/FairberryESP32S3Mainboard/ -- the module only brings out
  // GPIO 0-21 and 35-48 (26-34 are used internally for flash/PSRAM on
  // this module). That leaves 28 usable pins after excluding strapping
  // pins (0,3,45,46) and reserving 19/20 for possible native USB, which
  // is exactly enough for this board with the RGB trackball LEDs cut
  // from this revision (see the trackball doc) and SD moved from 4-pin
  // SPI to 3-pin 1-bit SDMMC (ESP32-S3 has a native SDMMC peripheral;
  // storage.h now uses SD_MMC.h instead of SD.h+SPI.h) -- there's no
  // spare pin margin left for anything more.
  //
  // No RESET_PIN here -- that was specific to the FAIRBERRY_* AVR mainboard
  // hardware revisions, not used on the ESP32 board types.
  byte rows[] = {1,2,4,5,6,7,8};
  byte cols[] = {9,10,11,12,13};

  #define KEYBOARD_LIGHT_PIN_1 14

  // Trackball (see trackball.h, TRACKBALL_ENABLED). S3's GPIO 0-21 are
  // all normal bidirectional pins -- no input-only pins forced onto the
  // direction lines here, though external pull-ups are still recommended
  // per the trackball doc's sensor-polarity caveat.
  #define TRACKBALL_UP_PIN 15
  #define TRACKBALL_DOWN_PIN 16
  #define TRACKBALL_LEFT_PIN 17
  #define TRACKBALL_RIGHT_PIN 18
  // Not a deep-sleep wake source in this revision -- GPIO48 isn't
  // RTC-capable, unlike the pin used in the earlier draft. Doubling BTN
  // as the wake button would need it moved back into the 0-21 RTC-capable
  // range, trading away whatever else was there; not done here since
  // there's no spare pin to trade with. A dedicated wake button (or deep
  // sleep entirely via power switch) is open follow-up work.
  #define TRACKBALL_BTN_PIN 48

  // RGB LEDs cut from this revision -- no pins left after fixing the
  // GPIO22-25/33-34 mistake above. TRACKBALL_LED_ENABLED will fail to
  // compile if turned on for this board type until a future revision
  // adds an I2C GPIO expander or similar.

  // Battery monitoring: a DIGITAL low-battery flag, not an analog voltage
  // reading. Reasoning: ESP32-S3's ADC-capable pins are only GPIO1-10
  // (ADC1) and GPIO11-20 (ADC2) -- and tracing through every assignment
  // above, GPIO0-21 is now *entirely* spoken for (keyboard matrix,
  // backlight, trackball, plus 19/20 reserved for native USB), with
  // nothing ADC-capable left over. Rather than reshuffle the whole board
  // again, this uses a
  // charger/supervisor IC's digital low-battery output instead of
  // continuous ADC voltage sensing -- a real capability reduction from
  // the "know the battery percentage" goal in the BOM doc, not a free
  // substitution. A fuel gauge over I2C (2 pins, doesn't need an
  // ADC-capable pin) would restore percentage-level monitoring -- unlike
  // an earlier draft of this comment claimed, this board doesn't
  // actually use the module's real UART0 (the pins named RXD0/TXD0 on
  // the KiCad symbol, i.e. GPIO43/44) for anything; those were free the
  // whole time and are the natural place to add an I2C fuel gauge in a
  // follow-up revision, at the cost of losing UART0 serial debug/flash
  // (native USB on 19/20 would remain as the flashing path).
  #define BATTERY_LOW_PIN 21

  // Mic (I2S MEMS mic, e.g. ICS-43434 -- see the KiCad schematic; more
  // readily available with a real KiCad symbol than the INMP441
  // originally specified in the BOM doc, electrically the same kind of
  // part) -- dedicated I2S port, own clock lines.
  #define MIC_I2S_WS_PIN 35
  #define MIC_I2S_BCLK_PIN 36
  #define MIC_I2S_DIN_PIN 37

  // Speaker (MAX98357A-style I2S amp) -- separate I2S port from the mic,
  // not sharing clock lines (simpler/lower-risk than a shared-clock
  // master/slave setup, at the cost of 2 extra pins -- see the doc).
  #define SPEAKER_I2S_WS_PIN 38
  #define SPEAKER_I2S_BCLK_PIN 39
  #define SPEAKER_I2S_DOUT_PIN 40

  // microSD via the ESP32-S3's native SDMMC peripheral in 1-bit mode
  // (CLK/CMD/D0 -- 3 pins) instead of SPI (CS/MOSI/MISO/SCK -- 4 pins).
  // storage.h uses SD_MMC.h accordingly. This is what makes the pin
  // budget fit at all -- see the comment at the top of this block.
  #define SD_MMC_CLK_PIN 41
  #define SD_MMC_CMD_PIN 42
  #define SD_MMC_D0_PIN 47
#endif

#ifdef DEBUG_SERIAL_INSTEAD_OF_USB
  #define KEY_LEFT_ALT '?'
  #define KEY_LEFT_CTRL '?'
  #define KEY_LEFT_SHIFT '?'
  #define KEY_RIGHT_SHIFT '?'
  #define KEY_RIGHT '?'
  #define KEY_RETURN '?'
  #define KEY_BACKSPACE '?'
  #define KEY_ESC '?'
  #define KEY_UP_ARROW '?'
  #define KEY_LEFT_ARROW '?'
  #define KEY_DOWN_ARROW '?'
  #define KEY_RIGHT_ARROW '?'
  #define KEY_TAB '?'
  #define KEY_F3 '?'
  #define KEY_F4 '?'
  #define KEY_F5 '?'
  #define KEY_F6 '?'
  #define KEY_F7 '?'
  #define KEY_F8 '?'
  #define KEY_F15 '?'
#endif

#define STICKY_STATUS_OPEN 0
#define STICKY_STATUS_STICKY 1
#define STICKY_STATUS_LOCKED 2