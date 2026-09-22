// Mic (INMP441-style I2S MEMS mic) and speaker (MAX98357A-style I2S amp)
// for the standalone smart keyboard (FAIRBERRY_ESP32S3_SMART).
//
// Uses the ESP32 Arduino core's I2S driver (driver/i2s.h). This is written
// against the common/legacy i2s.h API available across most Arduino-ESP32
// core releases; if your installed core has moved to the newer
// driver/i2s_std.h API only, the i2s_driver_install/i2s_set_pin/i2s_read/
// i2s_write calls below will need updating to match, though the overall
// structure (configure RX+TX ports, read/write PCM frames, WAV framing)
// stays the same.
//
// Sample gain/shift values here (the ">> 14" when reading mic samples)
// are the commonly-used starting point for INMP441-style mics feeding a
// 16-bit PCM pipeline, not a value tuned against this specific hardware
// -- expect to adjust it once you can actually hear a test recording.

#if defined(AUDIO_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <driver/i2s.h>
#include <SD.h>

#define AUDIO_SAMPLE_RATE 16000 // 16kHz mono is plenty for voice and keeps files small; also what Whisper expects internally
#define AUDIO_I2S_MIC_PORT I2S_NUM_0
#define AUDIO_I2S_SPEAKER_PORT I2S_NUM_1
#define AUDIO_READ_BUF_SAMPLES 512

bool audioMicRunning = false;

void audioInit() {
  i2s_config_t micConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 delivers 24-bit data in a 32-bit slot
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = AUDIO_READ_BUF_SAMPLES,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t micPins = {
    .bck_io_num = MIC_I2S_BCLK_PIN,
    .ws_io_num = MIC_I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_I2S_DIN_PIN
  };
  i2s_driver_install(AUDIO_I2S_MIC_PORT, &micConfig, 0, NULL);
  i2s_set_pin(AUDIO_I2S_MIC_PORT, &micPins);

  i2s_config_t speakerConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = AUDIO_READ_BUF_SAMPLES,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t speakerPins = {
    .bck_io_num = SPEAKER_I2S_BCLK_PIN,
    .ws_io_num = SPEAKER_I2S_WS_PIN,
    .data_out_num = SPEAKER_I2S_DOUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(AUDIO_I2S_SPEAKER_PORT, &speakerConfig, 0, NULL);
  i2s_set_pin(AUDIO_I2S_SPEAKER_PORT, &speakerPins);
}

void audioWriteWavHeader(File &f, uint32_t dataBytes) {
  uint32_t byteRate = AUDIO_SAMPLE_RATE * 2; // mono, 16-bit
  uint16_t blockAlign = 2;
  uint32_t chunkSize = 36 + dataBytes;

  f.seek(0);
  f.write((const uint8_t*)"RIFF", 4);
  f.write((const uint8_t*)&chunkSize, 4);
  f.write((const uint8_t*)"WAVE", 4);
  f.write((const uint8_t*)"fmt ", 4);
  uint32_t fmtChunkSize = 16;
  f.write((const uint8_t*)&fmtChunkSize, 4);
  uint16_t audioFormat = 1; // PCM
  f.write((const uint8_t*)&audioFormat, 2);
  uint16_t numChannels = 1;
  f.write((const uint8_t*)&numChannels, 2);
  uint32_t sampleRate = AUDIO_SAMPLE_RATE;
  f.write((const uint8_t*)&sampleRate, 4);
  f.write((const uint8_t*)&byteRate, 4);
  f.write((const uint8_t*)&blockAlign, 2);
  uint16_t bitsPerSample = 16;
  f.write((const uint8_t*)&bitsPerSample, 2);
  f.write((const uint8_t*)"data", 4);
  f.write((const uint8_t*)&dataBytes, 4);
}

// Records until stopRecording() is called, saving to path as a mono
// 16kHz/16-bit WAV file. Call from loop() repeatedly while recording is
// active -- this reads one chunk per call rather than blocking, so the
// keyboard/trackball scan loop keeps running during a recording.
File audioRecordFile;
bool audioRecordingActive = false;
uint32_t audioRecordedBytes = 0;

bool audioStartRecording(const String &path) {
  audioRecordFile = SD.open(path, FILE_WRITE);
  if (!audioRecordFile) {
    return false;
  }
  // Reserve space for the header, filled in properly once we know the
  // final size (audioStopRecording backfills it via audioWriteWavHeader).
  uint8_t placeholderHeader[44] = {0};
  audioRecordFile.write(placeholderHeader, 44);
  audioRecordedBytes = 0;
  audioRecordingActive = true;
  i2s_start(AUDIO_I2S_MIC_PORT);
  return true;
}

// Call once per loop() iteration while audioRecordingActive is true.
void audioRecordChunk() {
  if (!audioRecordingActive) return;

  int32_t rawSamples[AUDIO_READ_BUF_SAMPLES];
  size_t bytesRead = 0;
  i2s_read(AUDIO_I2S_MIC_PORT, rawSamples, sizeof(rawSamples), &bytesRead, 0);
  int samplesRead = bytesRead / sizeof(int32_t);
  if (samplesRead == 0) return;

  int16_t pcmSamples[AUDIO_READ_BUF_SAMPLES];
  for (int i = 0; i < samplesRead; i++) {
    pcmSamples[i] = (int16_t)(rawSamples[i] >> 14); // see file header note on gain tuning
  }
  audioRecordFile.write((const uint8_t*)pcmSamples, samplesRead * sizeof(int16_t));
  audioRecordedBytes += samplesRead * sizeof(int16_t);
}

void audioStopRecording() {
  if (!audioRecordingActive) return;
  i2s_stop(AUDIO_I2S_MIC_PORT);
  audioWriteWavHeader(audioRecordFile, audioRecordedBytes);
  audioRecordFile.close();
  audioRecordingActive = false;
}

// Plays a WAV file (blocking -- fine for short confirmation clips, but
// note it stalls the keyboard scan loop while playing).
void audioPlayWav(const String &path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return;
  f.seek(44); // skip the header, this doesn't re-parse it -- assumes 16kHz/16-bit/mono like what we write
  i2s_start(AUDIO_I2S_SPEAKER_PORT);
  int16_t buf[AUDIO_READ_BUF_SAMPLES];
  while (f.available()) {
    int n = f.read((uint8_t*)buf, sizeof(buf));
    size_t written = 0;
    i2s_write(AUDIO_I2S_SPEAKER_PORT, buf, n, &written, portMAX_DELAY);
  }
  i2s_stop(AUDIO_I2S_SPEAKER_PORT);
  f.close();
}

// Short beep for confirmation sounds (recording started/stopped, sync
// done, etc.) without needing a pre-recorded file.
void audioPlayTone(int frequencyHz, int durationMs) {
  i2s_start(AUDIO_I2S_SPEAKER_PORT);
  int totalSamples = (AUDIO_SAMPLE_RATE * durationMs) / 1000;
  int samplesPerCycle = AUDIO_SAMPLE_RATE / frequencyHz;
  int16_t buf[AUDIO_READ_BUF_SAMPLES];
  int samplesWritten = 0;
  while (samplesWritten < totalSamples) {
    int chunk = min(AUDIO_READ_BUF_SAMPLES, totalSamples - samplesWritten);
    for (int i = 0; i < chunk; i++) {
      int phase = (samplesWritten + i) % samplesPerCycle;
      buf[i] = (phase < samplesPerCycle / 2) ? 8000 : -8000; // simple square wave, not full-scale to avoid clipping/pop
    }
    size_t written = 0;
    i2s_write(AUDIO_I2S_SPEAKER_PORT, buf, chunk * sizeof(int16_t), &written, portMAX_DELAY);
    samplesWritten += chunk;
  }
  i2s_stop(AUDIO_I2S_SPEAKER_PORT);
}

#endif
