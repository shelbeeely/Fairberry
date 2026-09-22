// microSD storage for the standalone smart keyboard (FAIRBERRY_ESP32S3_SMART).
// SPI-based microSD card, holding voice recordings (.wav) and their
// transcripts (.txt, written by whisper_sync.h).

#if defined(AUDIO_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <SPI.h>
#include <SD.h>

bool storageReady = false;

bool storageInit() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  storageReady = SD.begin(SD_CS_PIN);
  #ifdef SERIAL_DEBUG_LOG
    Serial.println(storageReady ? "SD card ready" : "SD card init FAILED");
  #endif
  if (storageReady && !SD.exists("/recordings")) {
    SD.mkdir("/recordings");
  }
  return storageReady;
}

// Returns a not-yet-used filename like /recordings/REC00042.wav, so
// recordings never overwrite each other. Numbering is scanned from
// what's already on the card rather than kept in RAM, since this device
// can lose power/go to deep sleep between recordings.
String storageNextRecordingPath() {
  for (int i = 0; i < 100000; i++) {
    char path[32];
    snprintf(path, sizeof(path), "/recordings/REC%05d.wav", i);
    if (!SD.exists(path)) {
      return String(path);
    }
  }
  return ""; // Card is unreasonably full of recordings; caller should handle empty string.
}

String storageTranscriptPathFor(const String &wavPath) {
  String txtPath = wavPath;
  txtPath.replace(".wav", ".txt");
  return txtPath;
}

#endif
