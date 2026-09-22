// ICSH044A / SparkFun-style BlackBerry trackball support.
//
// This part is a clone of SparkFun's BlackBerry Trackballer Breakout
// (COM-09320): 4 hall-effect sensors (one per direction) driven by a
// magnet on the trackball's spindles, a momentary switch under the ball
// (BTN), and 4 LEDs (RED/GRN/BLU/WHT) to light it up.
//
// Sources disagreed on the direction pins' idle polarity (SparkFun's own
// example code implies idle-low/pulse-high via pulseIn(pin, HIGH, ...),
// while other clone listings suggest an open-drain active-low output
// needing a pull-up). Rather than guess, this reads edges on CHANGE and
// counts them regardless of polarity -- a rotation produces a train of
// transitions either way (SparkFun's own spec: "~9 high/low transitions
// per 360 degree rotation" on a single axis), so this treats each axis
// like a low-resolution incremental encoder and fires one arrow-key tap
// per TRACKBALL_EDGES_PER_STEP edges instead of assuming a signal shape.
//
// Direction pins are wired to the ESP32's input-only ADC pins
// (34/35/36/39), which have no internal pull-up/pull-down hardware at
// all -- this needs an external pull-up resistor per line on the PCB (or
// breadboard/perfboard if prototyping), regardless of whether the
// breakout module itself has one, since that wasn't reliably confirmed
// either. See Documentation/Hardware_ESP32_Trackball_Mainboard.md.
//
// Only wired up for BOARD_TYPE ESP32 right now -- IRAM_ATTR and the
// attachInterrupt(..., CHANGE) pattern used here are ESP32 Arduino core
// specifics, an AVR port would need pin-change interrupts instead.

#if defined(TRACKBALL_ENABLED) && BOARD_TYPE == ESP32

// Defined further down in BBQ10.ino; forward-declared here since this file
// is included before that definition and calls it from trackballPoll().
void wakeEverythingUp();

#ifndef TRACKBALL_EDGES_PER_STEP
  #define TRACKBALL_EDGES_PER_STEP 4 // Raw sensor edges per synthesized arrow-key tap. Lower = more sensitive.
#endif

#ifndef TRACKBALL_EDGE_MIN_INTERVAL_US
  #define TRACKBALL_EDGE_MIN_INTERVAL_US 800 // Ignore edges closer together than this (contact bounce guard)
#endif

#ifndef TRACKBALL_BTN_KEY
  #define TRACKBALL_BTN_KEY KEY_RETURN // What the center click sends. BlackBerry OS used the click as "select".
#endif

volatile uint32_t trackballEdgesUp = 0;
volatile uint32_t trackballEdgesDown = 0;
volatile uint32_t trackballEdgesLeft = 0;
volatile uint32_t trackballEdgesRight = 0;
volatile uint32_t trackballLastEdgeUpUs = 0;
volatile uint32_t trackballLastEdgeDownUs = 0;
volatile uint32_t trackballLastEdgeLeftUs = 0;
volatile uint32_t trackballLastEdgeRightUs = 0;

uint32_t trackballStepsUp = 0;
uint32_t trackballStepsDown = 0;
uint32_t trackballStepsLeft = 0;
uint32_t trackballStepsRight = 0;

bool trackballBtnLastState = false;
unsigned long trackballBtnDebounceUntilMs = 0;

void IRAM_ATTR trackballIsrUp() {
  uint32_t now = micros();
  if (now - trackballLastEdgeUpUs >= TRACKBALL_EDGE_MIN_INTERVAL_US) {
    trackballEdgesUp++;
    trackballLastEdgeUpUs = now;
  }
}
void IRAM_ATTR trackballIsrDown() {
  uint32_t now = micros();
  if (now - trackballLastEdgeDownUs >= TRACKBALL_EDGE_MIN_INTERVAL_US) {
    trackballEdgesDown++;
    trackballLastEdgeDownUs = now;
  }
}
void IRAM_ATTR trackballIsrLeft() {
  uint32_t now = micros();
  if (now - trackballLastEdgeLeftUs >= TRACKBALL_EDGE_MIN_INTERVAL_US) {
    trackballEdgesLeft++;
    trackballLastEdgeLeftUs = now;
  }
}
void IRAM_ATTR trackballIsrRight() {
  uint32_t now = micros();
  if (now - trackballLastEdgeRightUs >= TRACKBALL_EDGE_MIN_INTERVAL_US) {
    trackballEdgesRight++;
    trackballLastEdgeRightUs = now;
  }
}

void trackballInit() {
  pinMode(TRACKBALL_UP_PIN, INPUT);
  pinMode(TRACKBALL_DOWN_PIN, INPUT);
  pinMode(TRACKBALL_LEFT_PIN, INPUT);
  pinMode(TRACKBALL_RIGHT_PIN, INPUT);
  pinMode(TRACKBALL_BTN_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(TRACKBALL_UP_PIN), trackballIsrUp, CHANGE);
  attachInterrupt(digitalPinToInterrupt(TRACKBALL_DOWN_PIN), trackballIsrDown, CHANGE);
  attachInterrupt(digitalPinToInterrupt(TRACKBALL_LEFT_PIN), trackballIsrLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(TRACKBALL_RIGHT_PIN), trackballIsrRight, CHANGE);

  #ifdef TRACKBALL_LED_ENABLED
    pinMode(TRACKBALL_LED_RED_PIN, OUTPUT);
    pinMode(TRACKBALL_LED_GRN_PIN, OUTPUT);
    pinMode(TRACKBALL_LED_BLU_PIN, OUTPUT);
  #endif
}

// Sets the trackball LED color. Values are 0-255 per channel.
// NOTE: whether this board's LED lines already have onboard current-limiting
// resistors (like the SparkFun original) or need one added wasn't confirmed
// for this specific clone -- check before driving them at full brightness.
void trackballSetLed(byte red, byte green, byte blue) {
  #ifdef TRACKBALL_LED_ENABLED
    analogWrite(TRACKBALL_LED_RED_PIN, red);
    analogWrite(TRACKBALL_LED_GRN_PIN, green);
    analogWrite(TRACKBALL_LED_BLU_PIN, blue);
  #endif
}

// Drains accumulated sensor edges into arrow-key taps, and handles the
// center button. Call this once per loop() iteration.
void trackballPoll() {
  noInterrupts();
  uint32_t edgesUp = trackballEdgesUp; trackballEdgesUp = 0;
  uint32_t edgesDown = trackballEdgesDown; trackballEdgesDown = 0;
  uint32_t edgesLeft = trackballEdgesLeft; trackballEdgesLeft = 0;
  uint32_t edgesRight = trackballEdgesRight; trackballEdgesRight = 0;
  interrupts();

  trackballStepsUp += edgesUp;
  trackballStepsDown += edgesDown;
  trackballStepsLeft += edgesLeft;
  trackballStepsRight += edgesRight;

  while (trackballStepsUp >= TRACKBALL_EDGES_PER_STEP) {
    trackballStepsUp -= TRACKBALL_EDGES_PER_STEP;
    KEYBOARD_PRESS(KEY_UP_ARROW);
    KEYBOARD_RELEASE(KEY_UP_ARROW);
    wakeEverythingUp();
  }
  while (trackballStepsDown >= TRACKBALL_EDGES_PER_STEP) {
    trackballStepsDown -= TRACKBALL_EDGES_PER_STEP;
    KEYBOARD_PRESS(KEY_DOWN_ARROW);
    KEYBOARD_RELEASE(KEY_DOWN_ARROW);
    wakeEverythingUp();
  }
  while (trackballStepsLeft >= TRACKBALL_EDGES_PER_STEP) {
    trackballStepsLeft -= TRACKBALL_EDGES_PER_STEP;
    KEYBOARD_PRESS(KEY_LEFT_ARROW);
    KEYBOARD_RELEASE(KEY_LEFT_ARROW);
    wakeEverythingUp();
  }
  while (trackballStepsRight >= TRACKBALL_EDGES_PER_STEP) {
    trackballStepsRight -= TRACKBALL_EDGES_PER_STEP;
    KEYBOARD_PRESS(KEY_RIGHT_ARROW);
    KEYBOARD_RELEASE(KEY_RIGHT_ARROW);
    wakeEverythingUp();
  }

  bool btnPressed = (digitalRead(TRACKBALL_BTN_PIN) == LOW); // BTN is pulled low when clicked
  unsigned long nowMs = millis();
  if (btnPressed != trackballBtnLastState && nowMs >= trackballBtnDebounceUntilMs) {
    trackballBtnLastState = btnPressed;
    trackballBtnDebounceUntilMs = nowMs + DEBOUNCE_MS;
    if (btnPressed) {
      KEYBOARD_PRESS(TRACKBALL_BTN_KEY);
    } else {
      KEYBOARD_RELEASE(TRACKBALL_BTN_KEY);
    }
    wakeEverythingUp();
  }
}

#endif
