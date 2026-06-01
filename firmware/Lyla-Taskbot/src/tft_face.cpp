#include "tft_face.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <math.h>

#include "config.h"

namespace lyla {

namespace {

enum Emotion : uint8_t {
  EMO_HAPPY = 0,
  EMO_SATISFIED,
  EMO_SHY,
  EMO_DIZZY,
  EMO_ANGRY,
  EMO_ANGRY_IDLE,
  EMO_ROTATING,
  EMO_RELIEVED,
  EMO_SERVER_HAPPY,
  EMO_SERVER_SAD,
  EMO_SERVER_THINKING,
  EMO_SERVER_NEUTRAL,
};

constexpr uint16_t C_BG    = 0xCF38;
constexpr uint16_t C_BLACK = 0x0000;
constexpr uint16_t C_WHITE = 0xFFFF;
constexpr uint16_t C_MOUTH = 0x3B06;
constexpr uint16_t C_TEXT  = 0x1BEA;

Adafruit_ILI9341 g_tft(LYLA_TFT_CS, LYLA_TFT_DC, LYLA_TFT_RST);
GFXcanvas16* g_fb = nullptr;

Emotion g_current = EMO_HAPPY;
Emotion g_from = EMO_HAPPY;
Emotion g_target = EMO_HAPPY;
unsigned long g_emo_started_at = 0;
unsigned long g_transition_started_at = 0;
bool g_in_transition = false;

bool g_blinking = false;
unsigned long g_blink_started_at = 0;
unsigned long g_next_blink_at = 0;
float g_look_x = 0.0f, g_look_y = 0.0f;
float g_look_target_x = 0.0f, g_look_target_y = 0.0f;
unsigned long g_next_look_at = 0;

bool g_server_active = false;
ServerFace g_server_face = ServerFace::None;
String g_server_text;

bool g_status_active = false;
String g_status_text;

bool g_offline_input_suppressed = false;
bool g_talking_active = false;
float g_tilt_amount = 0.0f;
const char* g_last_logged_state = nullptr;

float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

float ease_in_out(float t) {
  t = clamp01(t);
  return t * t * (3.0f - 2.0f * t);
}

float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

uint16_t blend565(uint16_t a, uint16_t b, float t) {
  t = clamp01(t);
  uint8_t ar = (a >> 11) & 0x1F;
  uint8_t ag = (a >> 5)  & 0x3F;
  uint8_t ab = a & 0x1F;
  uint8_t br = (b >> 11) & 0x1F;
  uint8_t bg = (b >> 5)  & 0x3F;
  uint8_t bb = b & 0x1F;
  uint8_t rr = (uint8_t)(ar + (br - ar) * t);
  uint8_t rg = (uint8_t)(ag + (bg - ag) * t);
  uint8_t rb = (uint8_t)(ab + (bb - ab) * t);
  return (rr << 11) | (rg << 5) | rb;
}

uint16_t alpha_color(uint16_t color, float alpha) {
  return blend565(C_BG, color, alpha);
}

bool visible(float alpha) {
  return alpha > 0.025f;
}

void fill_ellipse(int cx, int cy, int rx, int ry, uint16_t color) {
  if (!g_fb || rx <= 0 || ry <= 0) return;
  for (int dy = -ry; dy <= ry; ++dy) {
    float v = 1.0f - (float)(dy * dy) / (float)(ry * ry);
    if (v < 0.0f) continue;
    int xw = (int)(rx * sqrtf(v));
    g_fb->drawFastHLine(cx - xw, cy + dy, xw * 2 + 1, color);
  }
}

void draw_thick_line(int x0, int y0, int x1, int y1, uint16_t color, int thick) {
  if (!g_fb || thick <= 0) return;
  int r = thick / 2;
  for (int d = -r; d <= r; ++d) {
    g_fb->drawLine(x0, y0 + d, x1, y1 + d, color);
    g_fb->drawLine(x0 + d, y0, x1 + d, y1, color);
  }
  g_fb->fillCircle(x0, y0, r, color);
  g_fb->fillCircle(x1, y1, r, color);
}

void draw_quadratic(int x0, int y0, int xc, int yc, int x1, int y1,
                    uint16_t color, int thick) {
  if (!g_fb) return;
  int px = x0;
  int py = y0;
  int steps = 44;
  int r = max(1, thick / 2);
  g_fb->fillCircle(px, py, r, color);
  for (int i = 1; i <= steps; ++i) {
    float t = (float)i / (float)steps;
    float u = 1.0f - t;
    int nx = (int)roundf(u*u*x0 + 2*u*t*xc + t*t*x1);
    int ny = (int)roundf(u*u*y0 + 2*u*t*yc + t*t*y1);
    draw_thick_line(px, py, nx, ny, color, thick);
    px = nx;
    py = ny;
  }
  g_fb->fillCircle(x1, y1, r, color);
}

void draw_cubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                uint16_t color, int thick) {
  if (!g_fb) return;
  int px = x0;
  int py = y0;
  int steps = 52;
  int r = max(1, thick / 2);
  g_fb->fillCircle(px, py, r, color);
  for (int i = 1; i <= steps; ++i) {
    float t = (float)i / (float)steps;
    float u = 1.0f - t;
    int nx = (int)roundf(u*u*u*x0 + 3*u*u*t*x1 + 3*u*t*t*x2 + t*t*t*x3);
    int ny = (int)roundf(u*u*u*y0 + 3*u*u*t*y1 + 3*u*t*t*y2 + t*t*t*y3);
    draw_thick_line(px, py, nx, ny, color, thick);
    px = nx;
    py = ny;
  }
  g_fb->fillCircle(x3, y3, r, color);
}

void push_face_roi() {
  if (!g_fb) return;
  uint16_t* buf = g_fb->getBuffer();
  g_tft.startWrite();
  for (int row = 0; row < LYLA_FACE_ROI_H; ++row) {
    int syy = LYLA_FACE_ROI_Y + row;
    g_tft.setAddrWindow(LYLA_FACE_ROI_X, syy, LYLA_FACE_ROI_W, 1);
    g_tft.writePixels(&buf[syy * LYLA_TFT_WIDTH + LYLA_FACE_ROI_X],
                      LYLA_FACE_ROI_W, true);
  }
  g_tft.endWrite();
}

void push_text_roi() {
  if (!g_fb) return;
  uint16_t* buf = g_fb->getBuffer();
  g_tft.startWrite();
  for (int row = 0; row < LYLA_TEXT_ROI_H; ++row) {
    int syy = LYLA_TEXT_ROI_Y + row;
    g_tft.setAddrWindow(LYLA_TEXT_ROI_X, syy, LYLA_TEXT_ROI_W, 1);
    g_tft.writePixels(&buf[syy * LYLA_TFT_WIDTH + LYLA_TEXT_ROI_X],
                      LYLA_TEXT_ROI_W, true);
  }
  g_tft.endWrite();
}

void push_full_frame() {
  if (!g_fb) return;
  g_tft.drawRGBBitmap(0, 0, g_fb->getBuffer(), LYLA_TFT_WIDTH, LYLA_TFT_HEIGHT);
}

}

}

namespace lyla {
namespace {

void draw_happy_eyes(float alpha, float blink_scale, float lx, float ly, float bob) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t shine = alpha_color(C_WHITE, alpha * 0.82f);
  int eye_ry = max(1, (int)roundf(14.0f * clamp01(blink_scale)));
  int y = 90 + (int)roundf(bob + ly * 0.15f);
  int lcx = 83 + (int)roundf(lx * 0.14f);
  int rcx = 236 + (int)roundf(lx * 0.14f);
  fill_ellipse(lcx, y, 9, eye_ry, black);
  fill_ellipse(rcx, y, 9, eye_ry, black);
  if (blink_scale > 0.35f && alpha > 0.35f) {
    int hx = (int)roundf(lx * 0.50f);
    int hy = (int)roundf(ly * 0.40f);
    g_fb->fillRoundRect(lcx - 4 + hx, y - eye_ry + 5 + hy, 3, 4, 1, shine);
    g_fb->fillRoundRect(rcx - 4 + hx, y - eye_ry + 5 + hy, 3, 4, 1, shine);
  }
}

void draw_idle_mouth(float alpha, float bob) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int b = (int)roundf(bob * 0.20f);
  draw_cubic(131, 146 + b, 140, 162 + b, 170, 171 + b, 189, 146 + b,
             black, max(1, (int)roundf(5.0f * alpha)));
}

void draw_satisfied_eyes(float alpha, float bob) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int b = (int)roundf(bob);
  int thick = max(1, (int)roundf(4.0f * alpha));
  draw_quadratic(67, 77 + b, 83, 113 + b, 100, 77 + b, black, thick);
  draw_quadratic(220, 77 + b, 236, 113 + b, 252, 77 + b, black, thick);
}

void draw_shy_face(float alpha, float bob) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t blush = alpha_color(0x546A, alpha * 0.62f);
  int b = (int)roundf(bob * 0.35f);
  draw_happy_eyes(alpha, 1.0f, 0.0f, 0.0f, b);
  draw_cubic(131, 157 + b, 141, 173 + b, 171, 182 + b, 189, 157 + b,
             black, max(1, (int)roundf(4.0f * alpha)));
  draw_cubic(126, 174 + b, 136, 168 + b, 134, 145 + b, 126, 140 + b,
             black, max(1, (int)roundf(4.0f * alpha)));
  draw_cubic(194, 140 + b, 185, 146 + b, 183, 168 + b, 194, 174 + b,
             black, max(1, (int)roundf(4.0f * alpha)));
  fill_ellipse(58, 131 + b, 16, 12, blush);
  fill_ellipse(262, 131 + b, 16, 12, blush);
}

void draw_open_bmo_mouth(float alpha, float openness, float bob, float live) {
  if (!visible(alpha)) return;
  openness = clamp01(openness);
  float talk = 0.55f + 0.45f * sinf(live * 0.018f);
  openness = clamp01(openness * talk);
  if (openness < 0.035f) {
    uint16_t black = alpha_color(C_BLACK, alpha);
    int b = (int)roundf(bob * 0.25f);
    draw_quadratic(130, 146 + b, 160, 171 + b, 189, 146 + b, black,
                   max(1, (int)roundf(5 * alpha)));
    return;
  }
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t green = alpha_color(C_MOUTH, alpha);
  uint16_t white = alpha_color(C_WHITE, alpha);
  int cx = 160;
  int outerW = 54;
  int maxH = 36;
  int outerH = max(6, (int)roundf(lerpf(6.0f, (float)maxH, ease_in_out(openness))));
  int x = cx - outerW / 2;
  int y = 136 + (int)roundf(bob * 0.40f);
  int r = constrain(outerH / 3, 3, 10);
  g_fb->fillRoundRect(x, y, outerW, outerH, r, black);
  g_fb->fillRect(x + 2, y, outerW - 4, min(8, outerH), black);
  int inset = 4;
  int teethH = max(4, min(12, (int)roundf(lerpf(4.0f, 11.0f, openness))));
  int ix = x + inset;
  int iy = y + 4 + teethH;
  int iw = outerW - inset * 2;
  int ih = max(2, outerH - teethH - 8);
  if (ih > 2) {
    int ir = constrain(ih / 3, 2, 8);
    g_fb->fillRoundRect(ix, iy, iw, ih, ir, green);
    g_fb->fillRect(ix, iy, iw, min(6, ih), green);
  }
  int tx = x + inset;
  int ty = y + 3;
  int tw = outerW - inset * 2;
  g_fb->fillRoundRect(tx, ty, tw, teethH, 2, white);
  g_fb->drawRoundRect(tx, ty, tw, teethH, 2, black);
  g_fb->drawRoundRect(x, y, outerW, outerH, r, black);
  g_fb->drawFastHLine(x + 2, y, outerW - 4, black);
}

void draw_smile_mouth(float alpha, float bob, float live) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int b = (int)roundf(bob + sinf(live * 0.004f) * 0.8f);
  draw_quadratic(130, 146 + b, 160, 171 + b, 189, 146 + b,
                 black, max(1, (int)roundf(5 * alpha)));
}

void draw_dizzy_eyes(float alpha, float spin) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int centers[2][2] = {{83, 92}, {236, 92}};
  for (int e = 0; e < 2; ++e) {
    int cx = centers[e][0];
    int cy = centers[e][1];
    float dir = (e == 0) ? 1.0f : -1.0f;
    for (int ring = 0; ring < 5; ++ring) {
      float r = lerpf(4.0f, 23.0f, (float)(ring + 1) / 5.0f);
      float start = spin * dir + ring * 0.62f;
      float end = start + (float)PI * 1.72f;
      int px = cx + (int)roundf(r * cosf(start));
      int py = cy + (int)roundf(r * sinf(start));
      for (int s = 1; s <= 32; ++s) {
        float a = lerpf(start, end, (float)s / 32.0f);
        int nx = cx + (int)roundf(r * cosf(a));
        int ny = cy + (int)roundf(r * sinf(a));
        draw_thick_line(px, py, nx, ny, black, max(1, (int)roundf(2.0f * alpha)));
        px = nx;
        py = ny;
      }
    }
    g_fb->fillCircle(cx, cy, 2, black);
  }
}

void draw_dizzy_mouth(float alpha, float spin) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t green = alpha_color(C_MOUTH, alpha);
  int cx = 160;
  int cy = 168 + (int)roundf(sinf(spin * 2.4f) * 3.0f);
  int rx = 22;
  int ry = 13 + (int)roundf(sinf(spin * 3.0f) * 3.0f);
  fill_ellipse(cx, cy, rx, max(6, ry), green);
  for (int t = 0; t < 96; ++t) {
    float a = (float)PI * 2.0f * (float)t / 96.0f;
    int x = cx + (int)roundf(rx * cosf(a));
    int y = cy + (int)roundf(max(7, ry + 1) * sinf(a));
    g_fb->drawPixel(x, y, black);
  }
}

void draw_angry_eyes_brows(float alpha, float twitch) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t shine = alpha_color(C_WHITE, alpha * 0.75f);
  int j = (int)roundf(twitch);
  int thick = max(1, (int)roundf(5.0f * alpha));
  draw_thick_line(55, 74 + j, 111, 84, black, thick);
  draw_thick_line(209, 84, 265, 74 + j, black, thick);
  fill_ellipse(83, 96 + j, 12, 18, black);
  fill_ellipse(236, 96 - j, 12, 18, black);
  if (alpha > 0.35f) {
    g_fb->fillRoundRect(86, 84 + j, 3, 4, 1, shine);
    g_fb->fillRoundRect(233, 84 - j, 3, 4, 1, shine);
  }
}

void draw_angry_mouth(float alpha, float twitch) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int j = (int)roundf(twitch);
  int thick = max(1, (int)roundf(4.0f * alpha));
  draw_quadratic(113, 178 + j, 160, 112 - j, 203, 178 + j, black, thick);
}

void draw_server_sad(float alpha, unsigned long now) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int sag = (int)(sinf((float)now * 0.0015f) * 2.0f);
  fill_ellipse(83, 96 + sag, 9, 11, black);
  fill_ellipse(236, 96 + sag, 9, 11, black);
  draw_thick_line(58, 78, 105, 86, black, max(1, (int)roundf(4.0f * alpha)));
  draw_thick_line(214, 86, 262, 78, black, max(1, (int)roundf(4.0f * alpha)));
  draw_quadratic(135, 168, 160, 150, 185, 168, black, max(1, (int)roundf(5.0f * alpha)));
}

void draw_server_thinking(float alpha, unsigned long now) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int bob = (int)roundf(sinf((float)now * 0.0035f) * 1.5f);
  draw_thick_line(55, 78 + bob, 111, 78 + bob, black, max(1, (int)roundf(4.0f * alpha)));
  draw_thick_line(209, 78 + bob, 265, 78 + bob, black, max(1, (int)roundf(4.0f * alpha)));
  fill_ellipse(83, 94 + bob, 12, 16, black);
  fill_ellipse(236, 94 - bob, 12, 16, black);
  draw_idle_mouth(alpha, bob * 0.2f);
  float phase = (float)now * 0.005f;
  for (int i = 0; i < 3; ++i) {
    float t = phase + i * 0.6f;
    float yo = sinf(t) * 6.0f;
    int dx = 130 + i * 30;
    g_fb->fillCircle(dx, 190 + (int)yo, 3, black);
  }
}

void draw_server_neutral(float alpha) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  fill_ellipse(83, 92, 9, 12, black);
  fill_ellipse(236, 92, 9, 12, black);
  draw_thick_line(140, 158, 180, 158, black, max(1, (int)roundf(5.0f * alpha)));
}

void draw_rotating_face(float alpha, unsigned long now) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  uint16_t green = alpha_color(C_MOUTH, alpha);
  uint16_t grey = alpha_color(0xDAD6, alpha);
  float tilt = constrain(g_tilt_amount, -1.0f, 1.0f);
  float wobble = sinf((float)now * 0.007f) * 0.08f;
  float angle = tilt * 0.35f + wobble;
  fill_ellipse(83 + (int)roundf(tilt * 10.0f), 92 - (int)roundf(tilt * 5.0f), 10, 18, black);
  fill_ellipse(236 + (int)roundf(tilt * 10.0f), 92 + (int)roundf(tilt * 5.0f), 10, 18, black);
  int cx = 160;
  int cy = 152;
  int px = cx;
  int py = cy;
  for (int i = 0; i < 28; ++i) {
    float a = angle + ((float)i / 27.0f) * (float)PI * 1.25f;
    float r = 10.0f + (float)i * 0.85f;
    int x = cx + (int)roundf(cosf(a) * r);
    int y = cy + (int)roundf(sinf(a) * r * 0.58f);
    if (i > 0) draw_thick_line(px, py, x, y, i < 8 ? grey : green, 3);
    px = x;
    py = y;
  }
  draw_quadratic(60, 138, 76, 126, 96, 120, black, max(1, (int)roundf(3.0f * alpha)));
  draw_quadratic(224, 80, 244, 68, 268, 60, black, max(1, (int)roundf(3.0f * alpha)));
}

const char* state_label_for(Emotion e) {
  switch (e) {
    case EMO_HAPPY: return "state: idle";
    case EMO_SATISFIED: return "state: touched";
    case EMO_SHY: return "state: shy";
    case EMO_DIZZY: return "state: shaked (dizzy)";
    case EMO_ANGRY: return "state: shaked (angry)";
    case EMO_ANGRY_IDLE: return "state: angry idle";
    case EMO_ROTATING: return "state: tilted";
    case EMO_RELIEVED: return "state: relieved";
    case EMO_SERVER_HAPPY: return g_talking_active ? "state: speaking" : "state: response";
    case EMO_SERVER_SAD: return "state: sad";
    case EMO_SERVER_THINKING: return "state: thinking";
    case EMO_SERVER_NEUTRAL: return g_talking_active ? "state: speaking" : "state: response";
  }
  return "state: unknown";
}

void log_state_if_changed(Emotion e) {
  const char* label = state_label_for(e);
  if (label == g_last_logged_state) return;
  g_last_logged_state = label;
  LYLA_LOG("bmo %s", label);
}

void draw_relieved_face(float alpha, float bob) {
  if (!visible(alpha)) return;
  uint16_t black = alpha_color(C_BLACK, alpha);
  int b = (int)roundf(bob * 0.25f);
  draw_quadratic(66, 86 + b, 83, 102 + b, 100, 86 + b, black, max(1, (int)roundf(4.0f * alpha)));
  draw_quadratic(220, 86 + b, 236, 102 + b, 252, 86 + b, black, max(1, (int)roundf(4.0f * alpha)));
  draw_cubic(130, 149 + b, 145, 166 + b, 177, 166 + b, 190, 149 + b,
             black, max(1, (int)roundf(5.0f * alpha)));
}

}
}

namespace lyla {

namespace {

void render_emotion_solid(Emotion e, float t, float blink, float breath,
                          float satisfied_bob, float spin, float angry_twitch,
                          unsigned long now) {
  switch (e) {
    case EMO_HAPPY:
      if (g_talking_active) {
        float mouth = 0.40f + 0.45f * fabsf(sinf(t * 0.0105f));
        draw_open_bmo_mouth(1.0f, mouth, breath * 0.8f, t);
      } else {
        draw_idle_mouth(1.0f, breath * 0.8f);
      }
      draw_happy_eyes(1.0f, blink, g_look_x, g_look_y, breath * 0.25f);
      break;
    case EMO_SATISFIED:
      draw_smile_mouth(1.0f, satisfied_bob, t);
      draw_satisfied_eyes(1.0f, satisfied_bob);
      break;
    case EMO_SHY:
      draw_shy_face(1.0f, satisfied_bob);
      break;
    case EMO_DIZZY:
      draw_dizzy_eyes(1.0f, spin);
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), 0.0f, t);
      } else {
        draw_dizzy_mouth(1.0f, spin);
      }
      break;
    case EMO_ANGRY:
      draw_angry_eyes_brows(1.0f, angry_twitch);
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), 0.0f, t);
      } else {
        draw_angry_mouth(1.0f, angry_twitch);
      }
      break;
    case EMO_ANGRY_IDLE:
      draw_angry_eyes_brows(1.0f, angry_twitch * 0.35f);
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), 0.0f, t);
      } else {
        draw_angry_mouth(1.0f, angry_twitch * 0.35f);
      }
      break;
    case EMO_ROTATING:
      draw_rotating_face(1.0f, now);
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), 0.0f, t);
      }
      break;
    case EMO_RELIEVED:
      if (g_talking_active) {
        draw_satisfied_eyes(1.0f, satisfied_bob);
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), satisfied_bob, t);
      } else {
        draw_relieved_face(1.0f, satisfied_bob);
      }
      break;
    case EMO_SERVER_HAPPY:
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), breath * 0.30f, t);
      } else {
        draw_smile_mouth(1.0f, breath * 0.30f, t);
      }
      draw_happy_eyes(1.0f, 1.0f, 0.0f, 0.0f, breath * 0.18f);
      break;
    case EMO_SERVER_SAD:
      if (g_talking_active) {
        draw_server_sad(1.0f, now);
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), breath * 0.25f, t);
      } else {
        draw_server_sad(1.0f, now);
      }
      break;
    case EMO_SERVER_THINKING:
      if (g_talking_active) {
        draw_server_thinking(1.0f, now);
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), breath * 0.25f, t);
      } else {
        draw_server_thinking(1.0f, now);
      }
      break;
    case EMO_SERVER_NEUTRAL:
      if (g_talking_active) {
        draw_open_bmo_mouth(1.0f, 0.35f + 0.65f * fabsf(sinf(t * 0.0105f)), breath * 0.25f, t);
        draw_happy_eyes(1.0f, 1.0f, 0.0f, 0.0f, breath * 0.12f);
      } else {
        draw_server_neutral(1.0f);
      }
      break;
  }
}

float current_blink_scale() {
  unsigned long now = millis();
  bool blink_allowed = (g_target == EMO_HAPPY || g_target == EMO_ANGRY_IDLE);
  if (!blink_allowed) {
    g_blinking = false;
    return 1.0f;
  }
  if (!g_blinking && now >= g_next_blink_at) {
    g_blinking = true;
    g_blink_started_at = now;
  }
  if (!g_blinking) return 1.0f;
  const unsigned long close_ms = 85;
  const unsigned long hold_ms = 28;
  const unsigned long open_ms = 80;
  unsigned long e = now - g_blink_started_at;
  if (e < close_ms) {
    return 1.0f - ease_in_out((float)e / (float)close_ms);
  }
  if (e < close_ms + hold_ms) {
    return 0.06f;
  }
  if (e < close_ms + hold_ms + open_ms) {
    return ease_in_out((float)(e - close_ms - hold_ms) / (float)open_ms);
  }
  g_blinking = false;
  g_next_blink_at = now + random(1800, 5200);
  return 1.0f;
}

void update_look() {
  unsigned long now = millis();
  if (now >= g_next_look_at) {
    g_look_target_x = (float)random(-28, 29) / 10.0f;
    g_look_target_y = (float)random(-18, 19) / 10.0f;
    g_next_look_at = now + random(900, 2300);
  }
  g_look_x = g_look_x * 0.82f + g_look_target_x * 0.18f;
  g_look_y = g_look_y * 0.82f + g_look_target_y * 0.18f;
}

void render_text_region() {
  if (!g_fb) return;
  g_fb->fillRect(LYLA_TEXT_ROI_X, LYLA_TEXT_ROI_Y,
                 LYLA_TEXT_ROI_W, LYLA_TEXT_ROI_H, C_BG);
  String text;
  if (g_status_active) {
    text = g_status_text;
  } else if (g_server_active && g_server_text.length() > 0) {
    text = g_server_text;
  } else {
    push_text_roi();
    return;
  }
  g_fb->setTextColor(C_TEXT);
  g_fb->setTextSize(1);
  g_fb->setCursor(LYLA_TEXT_ROI_X + 4, LYLA_TEXT_ROI_Y + 4);
  for (size_t i = 0; i < text.length() && i < 51; ++i) {
    g_fb->print(text[i]);
  }
  push_text_roi();
}

}

bool init_tft() {
  SPI.begin(LYLA_TFT_SCK, LYLA_TFT_MISO, LYLA_TFT_MOSI);
  g_tft.begin(LYLA_TFT_SPI_HZ);
  g_tft.setRotation(1);
  g_tft.fillScreen(C_BG);
  g_fb = new GFXcanvas16(LYLA_TFT_WIDTH, LYLA_TFT_HEIGHT);
  if (!g_fb || !g_fb->getBuffer()) {
    g_tft.fillScreen(0x0000);
    g_tft.setTextColor(0xFFFF);
    g_tft.setTextSize(1);
    g_tft.setCursor(12, 96);
    g_tft.print("Framebuffer allocation failed");
    g_tft.setCursor(12, 114);
    g_tft.print("Enable PSRAM for ESP32-S3");
    return false;
  }
  g_current = EMO_HAPPY;
  g_from = EMO_HAPPY;
  g_target = EMO_HAPPY;
  g_emo_started_at = millis();
  g_next_blink_at = millis() + random(1000, 2400);
  g_next_look_at = millis() + 700;
  return true;
}

void update_offline_inputs() {
  if (g_offline_input_suppressed) return;
  update_look();
}

namespace { void update_transition(); }

void render_frame() {
  unsigned long now = millis();
  float t = (float)now;
  if (!g_fb) return;
  update_transition();
  g_fb->fillRect(LYLA_FACE_ROI_X, LYLA_FACE_ROI_Y,
                 LYLA_FACE_ROI_W, LYLA_FACE_ROI_H, C_BG);
  float blink = current_blink_scale();
  float breath = sinf(t * 0.0021f) * 1.35f;
  float satisfied_bob = sinf(t * 0.0070f) * 1.6f;
  float spin = t * 0.0105f;
  float angry_twitch = sinf(t * 0.040f) * 1.8f;
  Emotion target = g_target;
  if (g_server_active) {
    switch (g_server_face) {
      case ServerFace::Happy:    target = EMO_SERVER_HAPPY;    break;
      case ServerFace::Sad:      target = EMO_SERVER_SAD;      break;
      case ServerFace::Thinking: target = EMO_SERVER_THINKING; break;
      case ServerFace::Neutral:  target = EMO_SERVER_NEUTRAL;  break;
      default: break;
    }
  }
  log_state_if_changed(target);
  render_emotion_solid(target, t, blink, breath, satisfied_bob, spin, angry_twitch, now);
  push_face_roi();
  render_text_region();
}

void set_server_face_override(ServerFace face, const String& screen_text) {
  g_server_active = true;
  g_server_face = face;
  g_server_text = screen_text;
}

void clear_server_face_override() {
  g_server_active = false;
  g_server_face = ServerFace::None;
  g_server_text = "";
}

void show_status_message(const char* line1, const char* line2) {
  if (!g_fb) return;
  g_fb->fillScreen(C_BG);
  g_fb->setTextColor(C_TEXT);
  g_fb->setTextSize(3);
  g_fb->setCursor(108, 86);
  if (line1) g_fb->print(line1);
  g_fb->setTextSize(1);
  g_fb->setCursor(72, 128);
  if (line2) g_fb->print(line2);
  push_full_frame();
}

void show_status_message_persistent(const char* msg) {
  g_status_active = true;
  g_status_text = msg ? msg : "";
  render_text_region();
}

void clear_status_message() {
  g_status_active = false;
  g_status_text = "";
  render_text_region();
}

void set_offline_input_suppressed(bool suppressed) {
  g_offline_input_suppressed = suppressed;
}

void set_talking_active(bool active) {
  g_talking_active = active;
}

}

namespace lyla {

namespace {

unsigned long g_last_touch_at = 0;
unsigned long g_last_shake_at = 0;
constexpr unsigned long kSatisfiedHoldMs = 1400;
constexpr unsigned long kShakeLockoutMs = 900;

void set_emotion(Emotion next) {
  if (next == g_target && !g_in_transition) return;
  if (g_in_transition) {
    g_current = g_target;
  }
  g_from = g_current;
  g_target = next;
  g_transition_started_at = millis();
  g_emo_started_at = millis();
  g_in_transition = true;
}

void update_transition() {
  if (!g_in_transition) return;
  unsigned long now = millis();
  if (now - g_transition_started_at >= LYLA_TFT_TRANSITION_MS) {
    g_current = g_target;
    g_from = g_target;
    g_in_transition = false;
  }
}

float current_transition_progress() {
  if (!g_in_transition) return 1.0f;
  unsigned long now = millis();
  float t = (float)(now - g_transition_started_at) / (float)LYLA_TFT_TRANSITION_MS;
  return ease_in_out(t);
}

bool can_interrupt_for_shake() {
  return g_target == EMO_HAPPY || g_target == EMO_SATISFIED ||
         g_target == EMO_ANGRY_IDLE;
}

bool can_interrupt_for_tilt() {
  return g_target == EMO_HAPPY || g_target == EMO_SATISFIED ||
         g_target == EMO_SHY || g_target == EMO_ANGRY_IDLE;
}

void update_state_machine(bool touched, bool shake_hit, bool tilt_active, float tilt_amount) {
  if (g_offline_input_suppressed) return;
  unsigned long now = millis();
  g_tilt_amount = tilt_amount;

  if (touched) {
    g_last_touch_at = now;
  }

  if (tilt_active && can_interrupt_for_tilt()) {
    set_emotion(EMO_ROTATING);
    return;
  }

  if (can_interrupt_for_shake() &&
      (now - g_last_shake_at > kShakeLockoutMs) && shake_hit) {
    g_last_shake_at = now;
    set_emotion(EMO_DIZZY);
    return;
  }

  switch (g_target) {
    case EMO_HAPPY:
    case EMO_ANGRY_IDLE:
      if (touched) set_emotion(EMO_SATISFIED);
      break;
    case EMO_SATISFIED:
      if (!touched && (now - g_last_touch_at > kSatisfiedHoldMs)) {
        set_emotion(EMO_SHY);
      }
      break;
    case EMO_SHY:
      if (now - g_emo_started_at >= 1600) set_emotion(EMO_HAPPY);
      break;
    case EMO_DIZZY:
      if (now - g_emo_started_at >= 2000) set_emotion(EMO_ANGRY);
      break;
    case EMO_ANGRY:
      if (now - g_emo_started_at >= 2000) set_emotion(EMO_ANGRY_IDLE);
      break;
    case EMO_ROTATING:
      if (!tilt_active) set_emotion(EMO_RELIEVED);
      break;
    case EMO_RELIEVED:
      if (now - g_emo_started_at >= 1300) set_emotion(EMO_HAPPY);
      break;
    default:
      break;
  }
}

}

void offline_dispatch_inputs(bool touched, bool shake_detected,
                             bool tilt_active, float tilt_amount) {
  update_state_machine(touched, shake_detected, tilt_active, tilt_amount);
}

void offline_show_tilt_relief() {
  if (!g_offline_input_suppressed && g_target == EMO_ROTATING) {
    set_emotion(EMO_RELIEVED);
  }
}

bool offline_is_rotating() {
  return !g_offline_input_suppressed && g_target == EMO_ROTATING;
}

}
