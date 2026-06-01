// firmware/src/audio_playback.h
// I2S output to MAX98357A speaker (Contract §7.5, §8.3).
// Plays WAV files from SD (16 kHz mono) or in-memory WAV bytes (16/24 kHz).

#pragma once

#include <Arduino.h>

namespace lyla {

bool audio_playback_init();

bool audio_playback_play_sd(const char* path);

bool audio_playback_play_sd_or_tone(const char* path);

bool audio_playback_play_tone(uint16_t frequency_hz, uint16_t duration_ms);

bool audio_playback_play_wav_bytes(const uint8_t* data, size_t len);

void audio_playback_set_response_talking(bool enabled);

bool audio_playback_is_busy();

void audio_playback_stop();

}
