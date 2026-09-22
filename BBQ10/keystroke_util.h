// Shared helper for typing an arbitrary string as keystrokes over the
// existing BLE HID keyboard connection. Used by web_server.h (typing out
// the transfer-mode URL) and voice_typing.h (typing out a confirmed
// transcript) -- both need "type this text as if a person typed it"
// rather than any specialized HID report.

#if BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

void typeString(const char *s) {
  for (const char *c = s; *c != '\0'; c++) {
    KEYBOARD_PRESS(*c);
    KEYBOARD_RELEASE(*c);
    delay(10); // small gap so the host doesn't drop characters typed too fast
  }
}

#endif
