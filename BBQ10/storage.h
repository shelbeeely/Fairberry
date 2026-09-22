// microSD storage for the standalone smart keyboard (FAIRBERRY_ESP32S3_SMART).
// Holds voice recordings (.wav) and their transcripts (.txt, written by
// whisper_sync.h).
//
// Uses the ESP32-S3's native SDMMC peripheral in 1-bit mode (SD_MMC.h),
// not SPI -- 3 pins (CLK/CMD/D0) instead of SPI's 4 (CS/MOSI/MISO/SCK),
// which is what makes this board's pin budget fit at all. See the
// SD_MMC_* pin comments in boards.h.

#if defined(AUDIO_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <SD_MMC.h>

bool storageReady = false;

bool storageInit() {
  SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN);
  storageReady = SD_MMC.begin("/sdcard", true); // true = 1-bit mode
  #ifdef SERIAL_DEBUG_LOG
    Serial.println(storageReady ? "SD card ready" : "SD card init FAILED");
  #endif
  if (storageReady && !SD_MMC.exists("/recordings")) {
    SD_MMC.mkdir("/recordings");
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
    if (!SD_MMC.exists(path)) {
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
