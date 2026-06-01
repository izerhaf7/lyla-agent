#include "audio_playback.h"

#include <SD_MMC.h>
#include <driver/i2s.h>
#include <string.h>

#include "config.h"
#include "tft_face.h"

namespace lyla {

namespace {

bool g_installed = false;
uint32_t g_current_rate = 0;
volatile bool g_busy = false;
bool g_response_talking = false;
unsigned long g_last_talking_frame_at = 0;

void begin_talking_if_needed() {
  if (!g_response_talking) return;
  g_last_talking_frame_at = 0;
  set_talking_active(true);
}

void pump_talking_frame() {
  if (!g_response_talking) return;
  unsigned long now = millis();
  if (g_last_talking_frame_at != 0 && now - g_last_talking_frame_at < 120) return;
  g_last_talking_frame_at = now;
  render_frame();
}

void end_talking_if_needed() {
  if (!g_response_talking) return;
  set_talking_active(false);
  render_frame();
}

bool install_i2s_output(uint32_t sample_rate) {
  if (g_installed && g_current_rate == sample_rate) return true;
  if (g_installed) {
    i2s_driver_uninstall(LYLA_SPK_I2S_NUM);
    g_installed = false;
  }
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sample_rate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;

  if (i2s_driver_install(LYLA_SPK_I2S_NUM, &cfg, 0, nullptr) != ESP_OK) {
    LYLA_ERR("audio_playback: i2s install failed");
    return false;
  }
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = LYLA_SPK_BCLK;
  pins.ws_io_num = LYLA_SPK_LRC;
  pins.data_out_num = LYLA_SPK_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(LYLA_SPK_I2S_NUM, &pins) != ESP_OK) {
    i2s_driver_uninstall(LYLA_SPK_I2S_NUM);
    LYLA_ERR("audio_playback: set_pin failed");
    return false;
  }
  i2s_zero_dma_buffer(LYLA_SPK_I2S_NUM);
  g_installed = true;
  g_current_rate = sample_rate;
  return true;
}

uint32_t parse_wav_sample_rate(const uint8_t* hdr) {
  return (uint32_t)hdr[24] | ((uint32_t)hdr[25] << 8) |
         ((uint32_t)hdr[26] << 16) | ((uint32_t)hdr[27] << 24);
}

uint16_t parse_wav_bits_per_sample(const uint8_t* hdr) {
  return (uint16_t)hdr[34] | ((uint16_t)hdr[35] << 8);
}

uint16_t parse_wav_channels(const uint8_t* hdr) {
  return (uint16_t)hdr[22] | ((uint16_t)hdr[23] << 8);
}

void write_pcm_blocking(const uint8_t* pcm, size_t pcm_bytes) {
  size_t written_total = 0;
  while (written_total < pcm_bytes) {
    size_t written = 0;
    size_t chunk = pcm_bytes - written_total;
    if (chunk > 1024) chunk = 1024;
    esp_err_t err = i2s_write(LYLA_SPK_I2S_NUM, pcm + written_total,
                              chunk, &written, pdMS_TO_TICKS(200));
    if (err != ESP_OK || written == 0) break;
    written_total += written;
    pump_talking_frame();
  }
  i2s_zero_dma_buffer(LYLA_SPK_I2S_NUM);
}

}

bool audio_playback_init() {
  return install_i2s_output(LYLA_MIC_SAMPLE_RATE);
}

bool audio_playback_play_sd(const char* path) {
  if (g_busy) return false;
  if (!path) return false;
  if (!SD_MMC.exists(path)) {
    LYLA_WARN("audio_playback: missing %s", path);
    return false;
  }
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) {
    LYLA_WARN("audio_playback: open failed %s", path);
    return false;
  }
  uint8_t hdr[44];
  if (f.read(hdr, sizeof(hdr)) != sizeof(hdr)) {
    f.close();
    return false;
  }
  if (hdr[0] != 'R' || hdr[1] != 'I' || hdr[2] != 'F' || hdr[3] != 'F') {
    f.close();
    LYLA_WARN("audio_playback: not RIFF: %s", path);
    return false;
  }
  uint32_t rate = parse_wav_sample_rate(hdr);
  uint16_t bits = parse_wav_bits_per_sample(hdr);
  uint16_t channels = parse_wav_channels(hdr);
  if (channels != 1 || bits != 16) {
    f.close();
    LYLA_WARN("audio_playback: unsupported wav %u ch %u bits", channels, bits);
    return false;
  }
  if (!install_i2s_output(rate)) {
    f.close();
    return false;
  }
  g_busy = true;
  begin_talking_if_needed();
  uint8_t chunk[1024];
  while (f.available()) {
    int n = f.read(chunk, sizeof(chunk));
    if (n <= 0) break;
    size_t written = 0;
    i2s_write(LYLA_SPK_I2S_NUM, chunk, (size_t)n, &written,
              pdMS_TO_TICKS(500));
    pump_talking_frame();
  }
  f.close();
  i2s_zero_dma_buffer(LYLA_SPK_I2S_NUM);
  end_talking_if_needed();
  g_busy = false;
  return true;
}

bool audio_playback_play_tone(uint16_t frequency_hz, uint16_t duration_ms) {
  if (g_busy) return false;
  if (frequency_hz == 0 || duration_ms == 0) return false;
  if (!install_i2s_output(LYLA_MIC_SAMPLE_RATE)) return false;

  constexpr size_t kToneChunkSamples = 128;
  constexpr int16_t kToneAmplitude = 2800;
  int16_t samples[kToneChunkSamples];
  uint32_t total_samples = (LYLA_MIC_SAMPLE_RATE * (uint32_t)duration_ms) / 1000UL;
  uint32_t period_samples = LYLA_MIC_SAMPLE_RATE / (uint32_t)frequency_hz;
  if (period_samples < 2) period_samples = 2;

  g_busy = true;
  begin_talking_if_needed();
  uint32_t produced = 0;
  while (produced < total_samples) {
    size_t n = total_samples - produced;
    if (n > kToneChunkSamples) n = kToneChunkSamples;
    for (size_t i = 0; i < n; ++i) {
      uint32_t phase = (produced + i) % period_samples;
      samples[i] = (phase < (period_samples / 2)) ? kToneAmplitude : -kToneAmplitude;
    }
    size_t written = 0;
    esp_err_t err = i2s_write(LYLA_SPK_I2S_NUM, samples, n * sizeof(int16_t),
                              &written, pdMS_TO_TICKS(200));
    if (err != ESP_OK || written == 0) break;
    pump_talking_frame();
    produced += (uint32_t)(written / sizeof(int16_t));
  }
  i2s_zero_dma_buffer(LYLA_SPK_I2S_NUM);
  end_talking_if_needed();
  g_busy = false;
  return produced > 0;
}

bool audio_playback_play_sd_or_tone(const char* path) {
  if (audio_playback_play_sd(path)) return true;
  if (path == nullptr || strcmp(path, "/sounds/err_generic.wav") != 0) {
    if (audio_playback_play_sd("/sounds/err_generic.wav")) return true;
  }
  return audio_playback_play_tone(660, 200);
}

bool audio_playback_play_wav_bytes(const uint8_t* data, size_t len) {
  if (g_busy) return false;
  if (!data || len < 44) return false;
  if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F') {
    LYLA_WARN("audio_playback: bytes not RIFF");
    return false;
  }
  uint32_t rate = parse_wav_sample_rate(data);
  uint16_t bits = parse_wav_bits_per_sample(data);
  uint16_t channels = parse_wav_channels(data);
  if (channels != 1 || bits != 16) {
    LYLA_WARN("audio_playback: unsupported in-memory wav");
    return false;
  }
  if (!install_i2s_output(rate)) return false;
  g_busy = true;
  begin_talking_if_needed();
  write_pcm_blocking(data + 44, len - 44);
  end_talking_if_needed();
  g_busy = false;
  return true;
}

void audio_playback_set_response_talking(bool enabled) {
  g_response_talking = enabled;
  if (!enabled) {
    set_talking_active(false);
  }
}

bool audio_playback_is_busy() {
  return g_busy;
}

void audio_playback_stop() {
  if (g_installed) {
    i2s_zero_dma_buffer(LYLA_SPK_I2S_NUM);
  }
  g_busy = false;
}

}
