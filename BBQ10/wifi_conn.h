// Shared WiFi connect/disconnect, used by both whisper_sync.h (cloud
// transcription) and web_server.h (local transfer mode) -- factored out
// since both need it independently of each other.

#if (defined(WIFI_TRANSCRIPTION_ENABLED) || defined(TRANSFER_MODE_ENABLED)) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <WiFi.h>

bool wifiConnect(unsigned long timeoutMs = 15000) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC; only used for scheduling, not display
    return true;
  }
  return false;
}

void wifiDisconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

#endif
