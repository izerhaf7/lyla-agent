// firmware/src/main.cpp
// Boot + main loop. Wires offline emotion engine + online voice integration.
// Boot sequence per Contract §10; coexistence per ADR-13.

#include <Arduino.h>
#include <MPU6050_tockn.h>
#include <Wire.h>

#include "audio_capture.h"
#include "audio_playback.h"
#include "config.h"
#include "directive_dispatcher.h"
#include "network_client.h"
#include "online_state.h"
#include "sd_config.h"
#include "tft_face.h"

namespace {

lyla::DeviceConfig g_cfg;
MPU6050 g_mpu(Wire);
bool g_mpu_ready = false;

float g_calib_x = 0.0f, g_calib_y = 0.0f;
float g_last_ax = 0.0f, g_last_ay = 0.0f, g_last_az = 0.0f;
float g_shake_filtered = 0.0f;
unsigned long g_last_shake_at = 0;
constexpr float kShakeTrigger = 17.0f;
constexpr unsigned long kShakeLockoutMs = 1100;

bool g_last_touch_raw = false;
bool g_last_touch_stable = false;
unsigned long g_touch_changed_at = 0;
unsigned long g_last_touch_at = 0;
constexpr unsigned long kTouchDebounceMs = 45;
constexpr unsigned long kSatisfiedHoldMs = 1400;

bool g_btn_stable_pressed = false;
bool g_btn_last_raw = false;
unsigned long g_btn_last_change_ms = 0;
constexpr unsigned long kButtonDebounceMs = 80;
unsigned long g_btn_flap_count = 0;
unsigned long g_btn_last_flap_log_ms = 0;
unsigned long g_btn_noise_locked_until_ms = 0;

unsigned long g_last_frame_at = 0;
bool g_tilt_active = false;
unsigned long g_tilt_candidate_since = 0;
unsigned long g_last_tilt_done_at = 0;
float g_tilt_amount = 0.0f;
bool g_tilt_started_event = false;
bool g_tilt_relief_event = false;

bool read_touch_stable() {
  bool raw = digitalRead(LYLA_TOUCH_PIN);
#if LYLA_TOUCH_ACTIVE_HIGH
  bool active = raw;
#else
  bool active = !raw;
#endif
  unsigned long now = millis();
  if (active != g_last_touch_raw) {
    g_last_touch_raw = active;
    g_touch_changed_at = now;
  }
  if ((now - g_touch_changed_at) >= kTouchDebounceMs) {
    g_last_touch_stable = active;
  }
  return g_last_touch_stable;
}

void update_mpu() {
  if (!g_mpu_ready) return;
  g_mpu.update();
  float ax = g_mpu.getAccX();
  float ay = g_mpu.getAccY();
  float az = g_mpu.getAccZ();
  float raw_shake = (fabsf(ax - g_last_ax) + fabsf(ay - g_last_ay) +
                    fabsf(az - g_last_az)) * 10.0f;
  g_shake_filtered = g_shake_filtered * 0.70f + raw_shake * 0.30f;
  g_last_ax = ax;
  g_last_ay = ay;
  g_last_az = az;
}

void calibrate_mpu() {
  lyla::show_status_message("BMO", "Calibrating MPU6050...");
  float sx = 0.0f, sy = 0.0f;
  for (int i = 0; i < 100; ++i) {
    g_mpu.update();
    sx += g_mpu.getAngleX();
    sy += g_mpu.getAngleY();
    delay(8);
  }
  g_calib_x = sx / 100.0f;
  g_calib_y = sy / 100.0f;
  g_mpu.update();
  g_last_ax = g_mpu.getAccX();
  g_last_ay = g_mpu.getAccY();
  g_last_az = g_mpu.getAccZ();
  g_shake_filtered = 0.0f;
  g_mpu_ready = true;
  LYLA_LOG("MPU steady baseline set from boot pose: angleX=%.1f angleY=%.1f; tilt is relative to this pose",
           g_calib_x, g_calib_y);
}

float current_tilt_delta() {
  if (!g_mpu_ready) return 0.0f;
  float dx = fabsf(g_mpu.getAngleX() - g_calib_x);
  float dy = fabsf(g_mpu.getAngleY() - g_calib_y);
  return dx > dy ? dx : dy;
}

bool update_tilt_state(bool shake_hit, unsigned long now) {
  if (!g_mpu_ready || shake_hit || lyla::online_is_active()) {
    if (shake_hit && g_tilt_active) {
      g_tilt_active = false;
      g_tilt_relief_event = false;
    }
    g_tilt_candidate_since = 0;
    return g_tilt_active;
  }
  float delta = current_tilt_delta();
  g_tilt_amount = constrain((g_mpu.getAngleX() - g_calib_x) / 45.0f, -1.0f, 1.0f);
  if (g_tilt_active) {
    if (delta <= LYLA_TILT_RECOVER_DEGREE) {
      g_tilt_active = false;
      g_last_tilt_done_at = now;
      g_tilt_relief_event = true;
    }
    return g_tilt_active;
  }
  if (now - g_last_tilt_done_at < LYLA_TILT_COOLDOWN_MS) {
    g_tilt_candidate_since = 0;
    return false;
  }
  if (delta >= LYLA_TILT_TRIGGER_DEGREE) {
    if (g_tilt_candidate_since == 0) g_tilt_candidate_since = now;
    if (now - g_tilt_candidate_since >= LYLA_TILT_MIN_HOLD_MS) {
      g_tilt_active = true;
      g_tilt_started_event = true;
    }
  } else {
    g_tilt_candidate_since = 0;
  }
  return g_tilt_active;
}

void halt_with_message(const char* line1, const char* line2) {
  lyla::show_status_message(line1, line2);
  pinMode(LYLA_LED_PIN, OUTPUT);
  for (;;) {
    digitalWrite(LYLA_LED_PIN, HIGH);
    delay(400);
    digitalWrite(LYLA_LED_PIN, LOW);
    delay(400);
  }
}

enum class BtnEdge : uint8_t { None, Pressed, Released };

bool sample_button_pressed_majority() {
  int low_count = 0;
  for (int i = 0; i < 5; ++i) {
    if (digitalRead(LYLA_PTT_PIN) == LOW) low_count++;
    delayMicroseconds(120);
  }
  return low_count >= 3;
}

BtnEdge poll_button_edge() {
  bool raw = sample_button_pressed_majority();
  unsigned long now = millis();

  if (now < g_btn_noise_locked_until_ms) {
    return BtnEdge::None;
  }

  if (now - g_btn_last_flap_log_ms >= 1000) {
    if (g_btn_flap_count > 0) {
      LYLA_WARN("PTT raw flapped %lux in 1s (stable=%s); check wiring/noise",
                g_btn_flap_count, g_btn_stable_pressed ? "PRESS" : "REL");
      if (g_btn_flap_count >= LYLA_PTT_NOISE_FLAP_THRESHOLD) {
        g_btn_noise_locked_until_ms = now + LYLA_PTT_NOISE_LOCKOUT_MS;
        g_btn_last_raw = raw;
        g_btn_stable_pressed = raw;
        g_btn_last_change_ms = now;
        LYLA_WARN("PTT noise lockout for %ums", (unsigned)LYLA_PTT_NOISE_LOCKOUT_MS);
        lyla::online_on_button_noise_locked();
      }
    }
    g_btn_flap_count = 0;
    g_btn_last_flap_log_ms = now;
  }

  if (raw != g_btn_last_raw) {
    if ((now - g_btn_last_change_ms) < kButtonDebounceMs) {
      g_btn_flap_count++;
    }
    g_btn_last_raw = raw;
    g_btn_last_change_ms = now;
    return BtnEdge::None;
  }
  if ((now - g_btn_last_change_ms) < kButtonDebounceMs) {
    return BtnEdge::None;
  }
  if (raw == g_btn_stable_pressed) {
    return BtnEdge::None;
  }
  g_btn_stable_pressed = raw;
  return raw ? BtnEdge::Pressed : BtnEdge::Released;
}

}

void setup() {
  Serial.begin(115200);
  delay(200);
  LYLA_LOG("boot, firmware=%s protocol=%s", LYLA_FIRMWARE_VERSION, LYLA_PROTOCOL_VERSION);

  pinMode(LYLA_TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(LYLA_PTT_PIN, INPUT_PULLUP);
  pinMode(LYLA_LED_PIN, OUTPUT);
  digitalWrite(LYLA_LED_PIN, LOW);

  Wire.begin(LYLA_I2C_SDA, LYLA_I2C_SCL);

  if (!lyla::init_tft()) {
    halt_with_message("BMO", "TFT init failed");
  }
  lyla::show_status_message("BMO", "Lyla starting...");
  delay(400);

  auto cfg_outcome = lyla::load_device_config(g_cfg);
  bool online_ready = (cfg_outcome.result == lyla::ConfigLoadResult::Ok);
  String online_disabled_reason;
  if (cfg_outcome.result != lyla::ConfigLoadResult::Ok) {
    online_disabled_reason = lyla::config_load_result_message(cfg_outcome);
    if (cfg_outcome.detail.length() > 0) {
      online_disabled_reason += ": ";
      online_disabled_reason += cfg_outcome.detail;
    }
    LYLA_WARN("online disabled: %s", online_disabled_reason.c_str());
    lyla::show_status_message("BMO", online_disabled_reason.c_str());
  } else {
    LYLA_LOG("config ok device_code=%s base_url=%s",
             g_cfg.device_code.c_str(), g_cfg.base_url.c_str());
  }

  if (!lyla::audio_capture_init()) {
    LYLA_WARN("audio capture init failed; online PTT disabled");
    if (online_ready) {
      online_ready = false;
      online_disabled_reason = "Audio init error";
    }
  }
  if (!lyla::audio_playback_init()) {
    LYLA_WARN("audio playback init failed; continuing visual-only");
  }

  if (online_ready) {
    lyla::show_status_message("BMO", "Joining WiFi...");
    lyla::network_init(g_cfg);
    bool wifi_ok = lyla::network_wifi_connect(15000);
    if (wifi_ok) {
      if (!lyla::network_post_heartbeat(g_cfg, true)) {
        LYLA_WARN("first heartbeat failed (non-fatal)");
      }
    } else {
      LYLA_WARN("starting offline-only; wifi will retry in background");
    }
  } else {
    LYLA_WARN("skipping WiFi; online disabled reason=%s", online_disabled_reason.c_str());
  }

  if (!lyla::audio_playback_play_sd_or_tone("/sounds/greet_hello.wav")) {
    LYLA_WARN("greet_hello.wav playback failed; check SD /sounds/ contents");
  } else {
    LYLA_LOG("greeting played; BMO ready");
  }

  g_mpu.begin();
  calibrate_mpu();

  lyla::clear_status_message();
  if (online_ready) {
    lyla::online_init(g_cfg);
  } else {
    if (online_disabled_reason.length() == 0) {
      online_disabled_reason = "Online tidak siap";
    }
    lyla::online_init_disabled(online_disabled_reason.c_str());
    lyla::show_status_message_persistent(online_disabled_reason.c_str());
  }
  g_btn_last_raw = (digitalRead(LYLA_PTT_PIN) == LOW);
  g_btn_stable_pressed = g_btn_last_raw;
  g_btn_last_change_ms = millis();
  randomSeed((uint32_t)esp_random());
  LYLA_LOG("setup complete; entering main loop");
}

void loop() {
  unsigned long now = millis();

  update_mpu();
  bool touched = read_touch_stable();
  if (touched) {
    g_last_touch_at = now;
  }

  bool shake_hit = false;
  if (g_mpu_ready &&
      (now - g_last_shake_at > kShakeLockoutMs) &&
      g_shake_filtered > kShakeTrigger) {
    shake_hit = true;
    g_last_shake_at = now;
  }

  bool tilt_now = update_tilt_state(shake_hit, now);
  bool play_dizzy_sound = false;
  if (shake_hit && !lyla::online_is_active()) {
    play_dizzy_sound = true;
    tilt_now = false;
  } else if (tilt_now) {
    shake_hit = false;
  }

  lyla::offline_dispatch_inputs(touched, shake_hit, tilt_now, g_tilt_amount);
  if (play_dizzy_sound) {
    lyla::audio_playback_play_sd_or_tone("/sounds/act_dizzy.wav");
  }
  if (g_tilt_started_event) {
    g_tilt_started_event = false;
    if (lyla::offline_is_rotating()) {
      lyla::audio_playback_play_sd_or_tone("/sounds/tilt_balance.wav");
    }
  }
  if (g_tilt_relief_event) {
    g_tilt_relief_event = false;
    lyla::offline_show_tilt_relief();
    lyla::audio_playback_play_sd_or_tone("/sounds/tilt_relief.wav");
  }

  BtnEdge edge = poll_button_edge();
  if (edge == BtnEdge::Pressed) {
    lyla::online_on_button_pressed();
  } else if (edge == BtnEdge::Released) {
    lyla::online_on_button_released();
  }

  lyla::online_loop(now);
  lyla::update_offline_inputs();

  if (now - g_last_frame_at >= LYLA_TFT_FRAME_MS) {
    g_last_frame_at = now;
    lyla::render_frame();
  }
}
