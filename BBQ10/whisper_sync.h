// WiFi + OpenAI Whisper API sync for the standalone smart keyboard
// (FAIRBERRY_ESP32S3_SMART). Scans /recordings on the SD card for .wav
// files with no matching .txt, uploads each to the Whisper API, and
// writes the returned transcript back to the card.
//
// TLS NOTE: this uses WiFiClientSecure::setInsecure(), which encrypts the
// connection but does NOT verify api.openai.com's certificate -- it's
// vulnerable to a MITM attacker on the network path. This is a
// pragmatic default rather than embedding a pinned root CA cert, since a
// wrong or stale pinned cert (they rotate) would fail silently and be
// hard to debug from here without hardware to test against. For a
// production build, pin the actual current root CA (something in the
// ISRG Root X1 / Amazon Root CA chain, whichever OpenAI's endpoint
// presents at the time) instead of leaving this insecure.
//
// Manual sync trigger: hold the trackball's center button for 2+ seconds.

#if defined(WIFI_TRANSCRIPTION_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <time.h>

#define WHISPER_API_HOST "api.openai.com"
#define WHISPER_API_PATH "/v1/audio/transcriptions"

// If transfer mode (web_server.h) is also enabled and currently active,
// don't disconnect WiFi out from under it when a sync finishes -- it's
// included after this file, so this is forward-declared rather than
// included directly.
#ifdef TRANSFER_MODE_ENABLED
  extern bool transferModeActive;
#endif

unsigned long whisperLastSyncCheckMs = 0;
bool whisperBtnHeld = false;
unsigned long whisperBtnHeldSinceMs = 0;
bool whisperSyncRequested = false;

// The manual-sync long-press reads TRACKBALL_BTN_PIN directly rather than
// going through trackball.h, so it works even if TRACKBALL_ENABLED isn't
// defined -- but that also means it needs to set the pin mode itself
// instead of assuming trackballInit() already did it.
void whisperInit() {
  #ifndef TRACKBALL_ENABLED
    pinMode(TRACKBALL_BTN_PIN, INPUT_PULLUP);
  #endif
}

time_t whisperReadLastSyncEpoch() {
  if (!SD.exists("/sync_state.txt")) return 0;
  File f = SD.open("/sync_state.txt", FILE_READ);
  if (!f) return 0;
  String line = f.readStringUntil('\n');
  f.close();
  return (time_t)line.toInt();
}

void whisperWriteLastSyncEpoch(time_t epoch) {
  File f = SD.open("/sync_state.txt", FILE_WRITE);
  if (!f) return;
  f.seek(0);
  f.println((long)epoch);
  f.close();
}

// Uploads one WAV file to the Whisper API and returns the transcript
// text via outText. Returns true on success. Streams the file from SD
// rather than loading it into RAM, since recordings can be several
// hundred KB to a few MB. Shared by the batch sync below and by
// voice_typing.h's interactive flow.
bool whisperTranscribeToString(const String &wavPath, String &outText) {
  File audioFile = SD.open(wavPath, FILE_READ);
  if (!audioFile) return false;
  size_t audioSize = audioFile.size();

  WiFiClientSecure client;
  client.setInsecure(); // see file header note

  if (!client.connect(WHISPER_API_HOST, 443)) {
    audioFile.close();
    return false;
  }

  String boundary = "FairberryBoundary7331";
  String partHeaderModel =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"model\"\r\n\r\n"
    "whisper-1\r\n";
  String partHeaderFile =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"file\"; filename=\"recording.wav\"\r\n"
    "Content-Type: audio/wav\r\n\r\n";
  String partFooter = "\r\n--" + boundary + "--\r\n";

  size_t contentLength = partHeaderModel.length() + partHeaderFile.length() + audioSize + partFooter.length();

  client.print(String("POST ") + WHISPER_API_PATH + " HTTP/1.1\r\n");
  client.print(String("Host: ") + WHISPER_API_HOST + "\r\n");
  client.print(String("Authorization: Bearer ") + OPENAI_API_KEY + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
  client.print("Content-Length: " + String(contentLength) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  client.print(partHeaderModel);
  client.print(partHeaderFile);

  uint8_t buf[1024];
  while (audioFile.available()) {
    int n = audioFile.read(buf, sizeof(buf));
    client.write(buf, n);
  }
  audioFile.close();
  client.print(partFooter);

  // Skip HTTP response headers, keep the body.
  String line;
  while (client.connected() || client.available()) {
    line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
  }
  String body = "";
  while (client.available()) {
    body += (char)client.read();
  }
  client.stop();

  if (body.length() == 0) return false;

  JsonDocument doc; // ArduinoJson v7 API; use StaticJsonDocument<N> instead if your installed version is v6
  DeserializationError err = deserializeJson(doc, body);
  if (err || !doc["text"].is<const char*>()) {
    #ifdef SERIAL_DEBUG_LOG
      Serial.print("Whisper API response didn't parse as expected: ");
      Serial.println(body);
    #endif
    return false;
  }

  outText = doc["text"].as<const char*>();
  return true;
}

// Uploads one WAV file and writes the transcript to the matching .txt
// path -- what the batch sync below uses.
bool whisperTranscribeFile(const String &wavPath, const String &txtPath) {
  String text;
  if (!whisperTranscribeToString(wavPath, text)) return false;

  File txtFile = SD.open(txtPath, FILE_WRITE);
  if (!txtFile) return false;
  txtFile.print(text);
  txtFile.close();
  return true;
}

// Uploads every .wav in /recordings that doesn't have a matching .txt yet.
void whisperSyncNow() {
  #ifdef TRACKBALL_LED_ENABLED
    trackballSetLed(0, 0, 60); // dim blue while syncing
  #endif

  if (!wifiConnect()) {
    #ifdef TRACKBALL_LED_ENABLED
      trackballSetLed(60, 0, 0); // red flash: couldn't connect
    #endif
    return;
  }

  File dir = SD.open("/recordings");
  bool anyFailed = false;
  File entry = dir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    if (name.endsWith(".wav")) {
      String wavPath = "/recordings/" + name;
      String txtPath = storageTranscriptPathFor(wavPath);
      if (!SD.exists(txtPath)) {
        bool ok = whisperTranscribeFile(wavPath, txtPath);
        if (!ok) anyFailed = true;
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();

  time_t now;
  time(&now);
  whisperWriteLastSyncEpoch(now);
  #ifdef TRANSFER_MODE_ENABLED
    if (!transferModeActive) {
      wifiDisconnect();
    }
  #else
    wifiDisconnect();
  #endif

  #ifdef TRACKBALL_LED_ENABLED
    trackballSetLed(anyFailed ? 60 : 0, anyFailed ? 0 : 60, 0); // green if all good, red-ish if something failed
  #endif
}

// Call once per loop() iteration. Cheap when there's nothing to do --
// only touches WiFi when a sync actually needs to run.
void whisperPoll() {
  unsigned long nowMs = millis();

  // Manual trigger: hold trackball BTN for 2+ seconds.
  bool btnPressed = (digitalRead(TRACKBALL_BTN_PIN) == LOW);
  if (btnPressed && !whisperBtnHeld) {
    whisperBtnHeld = true;
    whisperBtnHeldSinceMs = nowMs;
  } else if (!btnPressed) {
    whisperBtnHeld = false;
  } else if (whisperBtnHeld && nowMs - whisperBtnHeldSinceMs > 2000) {
    whisperSyncRequested = true;
    whisperBtnHeld = false; // don't re-trigger every loop while still held
  }

  // Scheduled trigger: check once a minute rather than every loop iteration.
  if (nowMs - whisperLastSyncCheckMs > 60000) {
    whisperLastSyncCheckMs = nowMs;
    time_t now;
    time(&now);
    if (now > 100000 && (now - whisperReadLastSyncEpoch()) * 1000L > WHISPER_SYNC_INTERVAL_MS) {
      // now > 100000 is a sanity check that we actually have real time
      // (SNTP hasn't synced yet right after boot, before any WiFi
      // connection -- without this a fresh device with epoch near 0
      // would treat every boot as "overdue" and try to sync constantly).
      whisperSyncRequested = true;
    }
  }

  if (whisperSyncRequested) {
    whisperSyncRequested = false;
    whisperSyncNow();
  }
}

#endif
