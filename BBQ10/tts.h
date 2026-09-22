// Text-to-speech via the OpenAI TTS API (/v1/audio/speech), for reading
// a transcript back so voice_typing.h can verify it before sending.
// Streams the response directly to the I2S speaker rather than buffering
// it to SD first, for lower latency.
//
// Same TLS caveat as whisper_sync.h: uses setInsecure(), encrypted but
// not certificate-verified. See that file's header comment for why.
//
// Requests response_format=wav (a plain PCM container, not compressed)
// so this doesn't need an MP3/Opus decoder on the ESP32 -- that's a
// deliberate simplicity tradeoff, not a limitation of the API itself.
// This assumes the response is a minimal/canonical 44-byte WAV header
// (same assumption audio.h's audioPlayWav makes for files this device
// writes itself) -- if OpenAI's response ever includes extra chunks
// before the data chunk, the sample-rate parse below would need to
// actually walk the chunks instead of assuming a fixed offset.

#if defined(VOICE_TYPING_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <WiFiClientSecure.h>

#define TTS_API_HOST "api.openai.com"
#define TTS_API_PATH "/v1/audio/speech"
#ifndef TTS_MODEL
  #define TTS_MODEL "tts-1" // faster/lower-quality than tts-1-hd, better fit for a quick confirmation readback
#endif
#ifndef TTS_VOICE
  #define TTS_VOICE "alloy"
#endif

// Synthesizes speech for the given text and plays it through the
// speaker, blocking until playback finishes. Returns false if the
// connection or request failed (nothing played).
bool ttsSpeak(const String &text) {
  if (!wifiConnect()) return false;

  WiFiClientSecure client;
  client.setInsecure(); // see file header note

  if (!client.connect(TTS_API_HOST, 443)) return false;

  String escapedText = text;
  escapedText.replace("\\", "\\\\");
  escapedText.replace("\"", "\\\"");
  escapedText.replace("\n", "\\n");
  String body = String("{\"model\":\"") + TTS_MODEL + "\",\"voice\":\"" + TTS_VOICE +
                "\",\"input\":\"" + escapedText + "\",\"response_format\":\"wav\"}";

  client.print(String("POST ") + TTS_API_PATH + " HTTP/1.1\r\n");
  client.print(String("Host: ") + TTS_API_HOST + "\r\n");
  client.print(String("Authorization: Bearer ") + OPENAI_API_KEY + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Content-Length: " + String(body.length()) + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(body);

  // Skip HTTP response headers.
  String line;
  while (client.connected() || client.available()) {
    line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
  }

  // Read the 44-byte WAV header to get the real sample rate (see file
  // header note on why this assumes a canonical/minimal header).
  uint8_t header[44];
  size_t headerRead = 0;
  unsigned long headerStart = millis();
  while (headerRead < 44 && millis() - headerStart < 10000) {
    if (client.available()) {
      header[headerRead++] = client.read();
    }
  }
  if (headerRead < 44) {
    client.stop();
    return false;
  }
  uint32_t sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | ((uint32_t)header[27] << 24);

  audioSetSpeakerSampleRate(sampleRate);
  i2s_start(AUDIO_I2S_SPEAKER_PORT);

  uint8_t buf[1024];
  unsigned long lastDataMs = millis();
  while (client.connected() || client.available()) {
    if (client.available()) {
      int n = client.read(buf, sizeof(buf));
      if (n > 0) {
        size_t written = 0;
        i2s_write(AUDIO_I2S_SPEAKER_PORT, buf, n, &written, portMAX_DELAY);
        lastDataMs = millis();
      }
    } else if (millis() - lastDataMs > 5000) {
      break; // connection stalled, give up rather than hang forever
    }
  }

  i2s_stop(AUDIO_I2S_SPEAKER_PORT);
  audioResetSpeakerSampleRate();
  client.stop();
  return true;
}

#endif
