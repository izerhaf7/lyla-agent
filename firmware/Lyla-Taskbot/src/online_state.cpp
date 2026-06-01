#include "online_state.h"

#include <ArduinoJson.h>

#include "audio_capture.h"
#include "audio_playback.h"
#include "config.h"
#include "directive_dispatcher.h"
#include "network_client.h"
#include "tft_face.h"

namespace lyla {

namespace {

const DeviceConfig* g_cfg = nullptr;
bool g_online_disabled = false;
String g_disabled_msg;

OnlineState g_state = OnlineState::Idle;
unsigned long g_state_entered_at = 0;
unsigned long g_record_started_at = 0;
unsigned long g_last_heartbeat_at = 0;
unsigned long g_next_ptt_allowed_at = 0;
unsigned long g_rate_window_started_at = 0;
uint8_t g_rate_window_count = 0;

bool g_button_held = false;

const char* g_halt_msg = nullptr;

void transition(OnlineState next) {
  g_state = next;
  g_state_entered_at = millis();
  set_offline_input_suppressed(next != OnlineState::Idle);
}

void show_status_for_seconds(const char* msg, unsigned long ms) {
  show_status_message_persistent(msg);
  (void)ms;
}

void enter_error(const char* msg) {
  audio_playback_play_sd_or_tone("/sounds/err_generic.wav");
  set_server_face_override(ServerFace::Sad, String());
  show_status_for_seconds(msg, 3000);
  transition(OnlineState::ShowingError);
}

void enter_offline_notice(const char* msg = "Tidak ada internet") {
  audio_playback_play_sd_or_tone("/sounds/err_generic.wav");
  set_server_face_override(ServerFace::Sad, String());
  show_status_for_seconds(msg, LYLA_OFFLINE_NOTICE_MS);
  transition(OnlineState::ShowingOfflineNotice);
}

bool ptt_request_allowed(unsigned long now, const char** reason) {
  if (now < g_next_ptt_allowed_at) {
    *reason = "Tunggu sebentar";
    return false;
  }
  if (g_rate_window_started_at == 0 ||
      now - g_rate_window_started_at >= LYLA_PTT_RATE_WINDOW_MS) {
    g_rate_window_started_at = now;
    g_rate_window_count = 0;
  }
  if (g_rate_window_count >= LYLA_PTT_RATE_MAX_REQUESTS) {
    *reason = "Terlalu sering";
    g_next_ptt_allowed_at = now + LYLA_PTT_COOLDOWN_MS;
    return false;
  }
  g_rate_window_count++;
  g_next_ptt_allowed_at = now + LYLA_PTT_COOLDOWN_MS;
  return true;
}

const char* indonesian_for_status(int http_status) {
  if (http_status == 401) return "Device tidak terdaftar";
  if (http_status == 404) return "Akun belum siap";
  if (http_status == 413) return "Rekaman terlalu panjang";
  if (http_status == 422) return "Permintaan ditolak";
  if (http_status == 400) return "Rekaman bermasalah";
  if (http_status == 502) return "Coba lagi sebentar";
  if (http_status >= 500) return "Server bermasalah, coba lagi";
  if (http_status >= 400) return "Permintaan ditolak";
  return "Coba lagi sebentar";
}

bool is_halt_status(int http_status) {
  return http_status == 401 || http_status == 404;
}

void send_audio_and_play(uint32_t recording_duration_ms) {
  if (g_cfg == nullptr) {
    enter_error("Config error");
    return;
  }
  if (!network_wifi_is_connected()) {
    audio_capture_release();
    enter_offline_notice();
    return;
  }

  audio_playback_play_sd_or_tone("/sounds/ack_thinking.wav");

  AudioRequestTelemetry tele = {};
  tele.client_request_id = network_generate_uuid_v4();
  tele.wifi_rssi_dbm = network_wifi_rssi();
  tele.battery_pct = -1;
  tele.recording_duration_ms = recording_duration_ms;

  AudioPostResult res = network_post_audio(*g_cfg,
                                           audio_capture_buffer(),
                                           audio_capture_size_bytes(),
                                           audio_capture_sample_rate(),
                                           tele);

  audio_capture_release();

  if (res.http_status == 200) {
    if (!res.protocol_version_ok) {
      enter_error("Versi server beda");
      return;
    }
    Directive d;
    if (!directive_parse(res.body, d)) {
      enter_error("Respon tidak valid");
      return;
    }
    transition(OnlineState::PlayingResponse);
    audio_playback_set_response_talking(true);
    directive_dispatch(*g_cfg, d);
    audio_playback_set_response_talking(false);
    transition(OnlineState::Idle);
    clear_server_face_override();
    clear_status_message();
    return;
  }

  if (res.http_status <= 0) {
    if (res.error.length() > 0) {
      LYLA_WARN("audio post network err: %s", res.error.c_str());
    }
    enter_error("Server tidak responsif");
    return;
  }

  if (is_halt_status(res.http_status)) {
    online_request_halt(indonesian_for_status(res.http_status));
    return;
  }

  enter_error(indonesian_for_status(res.http_status));
}

void finish_recording(const char* reason) {
  unsigned long t = millis();
  uint32_t dur = (t >= g_record_started_at) ? (uint32_t)(t - g_record_started_at) : 0;
  audio_capture_stop();
  g_button_held = false;
  uint16_t max_peak = audio_capture_session_max_peak();
  uint32_t voice_ms = audio_capture_voice_active_ms();
  if (audio_capture_size_bytes() > 0 && dur >= LYLA_MIN_RECORD_MS &&
      max_peak >= LYLA_SILENCE_REJECT_PEAK && voice_ms >= LYLA_MIN_VOICE_ACTIVE_MS) {
    LYLA_LOG("PTT release stop (%s) after %ums (%u bytes)",
             reason, (unsigned)dur, (unsigned)audio_capture_size_bytes());
    transition(OnlineState::Sending);
    send_audio_and_play(dur);
  } else {
    LYLA_WARN("recording discarded (%ums peak=%u voice=%ums); %s",
              (unsigned)dur, (unsigned)max_peak, (unsigned)voice_ms, reason);
    audio_capture_release();
    clear_server_face_override();
    enter_offline_notice("Suara tidak terdengar");
  }
}

}

void online_init(const DeviceConfig& cfg) {
  g_cfg = &cfg;
  g_online_disabled = false;
  g_disabled_msg = String();
  g_state = OnlineState::Idle;
  g_state_entered_at = millis();
  g_last_heartbeat_at = 0;
  g_next_ptt_allowed_at = 0;
  g_rate_window_started_at = 0;
  g_rate_window_count = 0;
  g_button_held = false;
}

void online_init_disabled(const char* indonesian_msg) {
  g_cfg = nullptr;
  g_online_disabled = true;
  g_disabled_msg = indonesian_msg ? String(indonesian_msg) : String("Fitur online tidak siap");
  g_state = OnlineState::Idle;
  g_state_entered_at = millis();
  g_last_heartbeat_at = 0;
  g_next_ptt_allowed_at = 0;
  g_rate_window_started_at = 0;
  g_rate_window_count = 0;
  g_button_held = false;
}

void online_on_button_pressed() {
  if (g_state != OnlineState::Idle) return;
  if (g_online_disabled || g_cfg == nullptr) {
    enter_offline_notice(g_disabled_msg.length() > 0 ? g_disabled_msg.c_str() : "Fitur online tidak siap");
    return;
  }
  if (!network_wifi_is_connected()) {
    enter_offline_notice();
    return;
  }
  if (!audio_capture_init()) {
    enter_error("Audio init error");
    return;
  }
  const char* reject_reason = nullptr;
  unsigned long now = millis();
  if (!ptt_request_allowed(now, &reject_reason)) {
    enter_offline_notice(reject_reason ? reject_reason : "Tunggu sebentar");
    return;
  }
  LYLA_LOG("PTT press; recording while held (max %ums)...",
           (unsigned)LYLA_MAX_RECORD_MS);
  audio_capture_start();
  g_record_started_at = now;
  g_button_held = true;
  set_server_face_override(ServerFace::Thinking, String("Mendengarkan..."));
  transition(OnlineState::Recording);
}

void online_on_button_noise_locked() {
  if (g_state != OnlineState::Idle) return;
  g_next_ptt_allowed_at = millis() + LYLA_PTT_NOISE_LOCKOUT_MS;
  enter_offline_notice("Tombol tidak stabil");
}

void online_on_button_released() {
  g_button_held = false;
  if (g_state == OnlineState::Recording) {
    finish_recording("button released");
  }
}

void online_loop(unsigned long now) {
  if (!g_online_disabled) {
    network_wifi_loop();
  }

  switch (g_state) {
    case OnlineState::Idle: {
      if (now - g_last_heartbeat_at >= LYLA_HEARTBEAT_INTERVAL_MS) {
        g_last_heartbeat_at = now;
        if (!g_online_disabled && g_cfg != nullptr && network_wifi_is_connected()) {
          HeartbeatResult hb = network_post_heartbeat_with_commands(*g_cfg, true);
          if (hb.ok && hb.command_count > 0) {
            for (size_t i = 0; i < hb.command_count; ++i) {
              const PendingCommand& cmd = hb.commands[i];
              if (cmd.command_type == "play_reminder") {
                Directive d;
                if (directive_parse(cmd.payload_json, d)) {
                  directive_dispatch(*g_cfg, d);
                } else {
                  LYLA_WARN("reminder directive parse failed cmd=%s",
                            cmd.command_id.c_str());
                }
              } else {
                LYLA_WARN("unknown command_type=%s id=%s",
                          cmd.command_type.c_str(), cmd.command_id.c_str());
              }
              network_ack_command(*g_cfg, cmd.command_id);
            }
          }
        }
      }
      break;
    }
    case OnlineState::Recording: {
      bool ok = audio_capture_pump();
      unsigned long t = millis();
      if (t < g_record_started_at) break;
      uint32_t dur = (uint32_t)(t - g_record_started_at);

      // True push-to-talk: the user controls the recording window by
      // holding the button; release (online_on_button_released) is the
      // normal stop. These are only safety caps so a stuck button or a
      // very long hold cannot overflow the PSRAM buffer.
      if (!ok) {
        finish_recording("buffer full");
      } else if (dur >= LYLA_MAX_RECORD_MS) {
        finish_recording("max duration");
      }
      break;
    }
    case OnlineState::Sending:
    case OnlineState::PlayingResponse:
      break;
    case OnlineState::ShowingError:
      if (now - g_state_entered_at >= 3000) {
        clear_server_face_override();
        clear_status_message();
        transition(OnlineState::Idle);
      }
      break;
    case OnlineState::ShowingOfflineNotice:
      if (now - g_state_entered_at >= LYLA_OFFLINE_NOTICE_MS) {
        clear_server_face_override();
        clear_status_message();
        transition(OnlineState::Idle);
      }
      break;
    case OnlineState::Halted:
      break;
  }
}

OnlineState online_current_state() {
  return g_state;
}

bool online_is_active() {
  return g_state != OnlineState::Idle && g_state != OnlineState::Halted;
}

void online_request_halt(const char* indonesian_msg) {
  g_halt_msg = indonesian_msg;
  set_server_face_override(ServerFace::Sad, String());
  show_status_message_persistent(indonesian_msg ? indonesian_msg : "Halt");
  transition(OnlineState::Halted);
  pinMode(LYLA_LED_PIN, OUTPUT);
  digitalWrite(LYLA_LED_PIN, HIGH);
}

}
