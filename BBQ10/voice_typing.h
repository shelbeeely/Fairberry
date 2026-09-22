// Interactive voice typing: record -> transcribe (Whisper) -> read the
// transcript back (TTS) so you can verify it heard you right -> confirm
// (Enter) to type it into the host, or discard (Backspace) and try again.
//
// This is a separate mode from the async batch Whisper sync in
// whisper_sync.h -- that one is for longer recordings you don't want to
// wait on; this one is for short dictated text you want to send right
// now, like a phone's voice-to-text keyboard.
//
// Toggle recording with SYM + V (press once to start, again to stop).
// Stopping triggers the transcribe+readback+confirm flow immediately.
//
// HONEST LIMITATION: this whole flow (WiFi connect + Whisper upload +
// TTS request + playback + waiting for your confirm keypress) runs
// blocking/synchronous -- the keyboard and trackball don't respond to
// anything else while it's in progress. Making this properly
// non-blocking would need a real async state machine across multiple
// network round-trips, which is meaningfully more firmware complexity
// than this pass attempts. Expect a real, noticeable pause (WiFi
// connect alone can take several seconds, plus two API round-trips)
// between stopping the recording and hearing it read back.

#if defined(VOICE_TYPING_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#define VOICE_TYPING_TEMP_PATH "/recordings/_voice_typing_tmp.wav"
#ifndef VOICE_TYPING_CONFIRM_TIMEOUT_MS
  #define VOICE_TYPING_CONFIRM_TIMEOUT_MS 15000 // Auto-discard if no confirm/discard keypress in time
#endif

// Same reasoning as whisper_sync.h: don't disconnect WiFi out from under
// an active transfer-mode session. web_server.h is included after this
// file, so this is forward-declared.
#ifdef TRANSFER_MODE_ENABLED
  extern bool transferModeActive;
#endif

// readMatrix()/keyPressed() are defined further down in BBQ10.ino
// (after this file is included) -- forward-declared here rather than
// relying on Arduino's automatic prototype generation, same reasoning as
// trackball.h's forward declaration of wakeEverythingUp().
boolean readMatrix(byte debouceMsSinceLast);
bool keyPressed(int colIndex, int rowIndex);

bool voiceTypingRecording = false;

// Blocks until Enter (confirm) or Backspace (discard) is pressed, or the
// timeout elapses (treated as discard). Polls the matrix directly rather
// than going through the normal loop() cadence, since this is a modal
// wait nested inside handling the key combo that triggered it.
bool voiceTypingWaitForConfirm() {
  unsigned long start = millis();
  unsigned long last = start;
  while (millis() - start < VOICE_TYPING_CONFIRM_TIMEOUT_MS) {
    unsigned long now = millis();
    readMatrix(now - last);
    last = now;
    if (keyPressed(K_ENTER)) return true;
    if (keyPressed(K_BACKSPACE)) return false;
    delay(10);
  }
  return false; // timed out -- treat as discard
}

void voiceTypingStop() {
  audioStopRecording();

  #ifdef TRACKBALL_LED_ENABLED
    trackballSetLed(0, 0, 60); // dim blue: transcribing
  #endif

  if (!wifiConnect()) {
    #ifdef TRACKBALL_LED_ENABLED
      trackballSetLed(60, 0, 0); // red: couldn't connect
    #endif
    return;
  }

  String text;
  bool ok = whisperTranscribeToString(VOICE_TYPING_TEMP_PATH, text);
  if (!ok || text.length() == 0) {
    #ifdef TRACKBALL_LED_ENABLED
      trackballSetLed(60, 0, 0); // red: transcription failed
    #endif
    return;
  }

  #ifdef TRACKBALL_LED_ENABLED
    trackballSetLed(0, 60, 60); // cyan: reading it back
  #endif
  ttsSpeak(text);

  #ifdef TRACKBALL_LED_ENABLED
    trackballSetLed(60, 60, 0); // yellow: waiting for Enter/Backspace
  #endif
  bool confirmed = voiceTypingWaitForConfirm();

  if (confirmed) {
    typeString(text.c_str());
    #ifdef TRACKBALL_LED_ENABLED
      trackballSetLed(0, 60, 0); // green: sent
    #endif
  } else {
    #ifdef TRACKBALL_LED_ENABLED
      trackballSetLed(0, 0, 0); // off: discarded
    #endif
  }

  #ifdef TRANSFER_MODE_ENABLED
    if (!transferModeActive) {
      wifiDisconnect();
    }
  #else
    wifiDisconnect();
  #endif
}

void voiceTypingToggle() {
  if (voiceTypingRecording) {
    voiceTypingRecording = false;
    voiceTypingStop(); // runs the whole transcribe/TTS/confirm flow before returning
  } else {
    if (audioStartRecording(VOICE_TYPING_TEMP_PATH)) {
      voiceTypingRecording = true;
      audioPlayTone(1800, 100);
    } else {
      audioPlayTone(400, 200); // low beep: couldn't start (e.g. a batch recording is already active)
    }
  }
}

#endif
