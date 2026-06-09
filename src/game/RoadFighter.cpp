#include "game/RoadFighter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "display/DisplayManager.h"

// RGB565 palette
static constexpr uint16_t kGrass      = 0x0320;  // dark green
static constexpr uint16_t kRoad       = 0x4208;  // dark gray
static constexpr uint16_t kBarrier    = 0xFFE0;  // yellow edge lines
static constexpr uint16_t kDash       = 0xCE79;  // light gray center dashes
static constexpr uint16_t kBlack      = 0x0000;
static constexpr uint16_t kWhite      = 0xFFFF;
static constexpr uint16_t kRed        = 0xF800;
static constexpr uint16_t kSilver     = 0xCE79;  // player body
static constexpr uint16_t kNavy       = 0x001F;
static constexpr uint16_t kCyan       = 0x07FF;
static constexpr uint16_t kOrange     = 0xFC00;
static constexpr uint16_t kPurple     = 0x780F;
static constexpr uint16_t kWindshield = 0x0010;
static constexpr uint16_t kGameOverBg = 0x2000;

static constexpr uint16_t kEnemyColors[] = { kRed, kNavy, kCyan, kOrange, kPurple };

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void RoadFighter::begin() {
  reset();
}

void RoadFighter::reset() {
  playerX_     = W / 2 - kPlayerW / 2;
  roadOffset_  = 0;
  score_       = 0;
  lastFrameMs_ = 0;
  lastSpawnMs_ = 0;
  gameOver_    = false;
  for (auto& e : enemies_) e.active = false;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

int RoadFighter::currentSpeed() const {
  return kBaseSpeed + (int)(score_ / 300);
}

uint32_t RoadFighter::nextRng() {
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return rng_;
}

void RoadFighter::spawnEnemy() {
  for (auto& e : enemies_) {
    if (e.active) continue;
    // Three road lanes centred at x = 46, 86, 126
    const int lane = (int)(nextRng() % 3);
    const int cx   = kRoadLeft + 20 + lane * 40;
    e.x      = (int16_t)(cx - kEnemyW / 2);
    e.y      = (int16_t)(-kEnemyH);
    e.color  = kEnemyColors[nextRng() % 5];
    e.speed  = (uint8_t)(currentSpeed() + (int)(nextRng() % 3));
    e.active = true;
    return;
  }
}

bool RoadFighter::overlaps(int ax, int ay, int aw, int ah,
                            int bx, int by, int bw, int bh) const {
  return !(ax + aw - 3 <= bx || bx + bw - 3 <= ax ||
           ay + ah - 3 <= by || by + bh - 3 <= ay);
}

// ── Update ────────────────────────────────────────────────────────────────────

void RoadFighter::update(uint32_t nowMs, float steer) {
  if (gameOver_) return;
  if (lastFrameMs_ == 0) { lastFrameMs_ = nowMs; return; }
  if (nowMs - lastFrameMs_ < kFrameMs) return;
  lastFrameMs_ = nowMs;

  // Proportional steering from accelerometer tilt (dead zone ±0.08)
  static constexpr float kDeadZone  = 0.08f;
  static constexpr float kFullTilt  = 0.65f;  // tilt beyond this = max speed
  if (fabsf(steer) > kDeadZone) {
    const float norm = std::min(1.0f, (fabsf(steer) - kDeadZone) / (kFullTilt - kDeadZone));
    const int   px   = (int)(kMaxMoveSpeed * norm);
    if (steer < 0.0f) {
      playerX_ = (int16_t)std::max(kRoadLeft + 3, (int)playerX_ - px);
    } else {
      playerX_ = (int16_t)std::min(kRoadRight - kPlayerW - 3, (int)playerX_ + px);
    }
  }

  // Scroll road
  const int spd = currentSpeed();
  roadOffset_ += spd;

  // Advance enemies
  for (auto& e : enemies_) {
    if (!e.active) continue;
    e.y = (int16_t)(e.y + e.speed);
    if (e.y > H) {
      e.active = false;
      score_ += 10;
    }
  }

  // Spawn on a shrinking interval (min 600 ms)
  const uint32_t interval = (uint32_t)std::max(600, 3000 - (int)score_ * 3);
  if (nowMs - lastSpawnMs_ >= interval) {
    spawnEnemy();
    lastSpawnMs_ = nowMs;
  }

  score_++;

  // Collision
  for (const auto& e : enemies_) {
    if (!e.active) continue;
    if (overlaps(playerX_, kPlayerBaseY, kPlayerW, kPlayerH,
                 e.x, e.y, kEnemyW, kEnemyH)) {
      gameOver_ = true;
      return;
    }
  }
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void RoadFighter::drawRoad(DisplayManager& d) const {
  // Grass
  d.gameFillRect(0,         0, kRoadLeft,         H, kGrass);
  d.gameFillRect(kRoadRight, 0, W - kRoadRight,   H, kGrass);

  // Road surface
  d.gameFillRect(kRoadLeft, 0, kRoadRight - kRoadLeft, H, kRoad);

  // Yellow edge lines
  d.gameFillRect(kRoadLeft,      0, 3, H, kBarrier);
  d.gameFillRect(kRoadRight - 3, 0, 3, H, kBarrier);

  // Scrolling center dashes
  static constexpr int kDashLen    = 24;
  static constexpr int kDashPeriod = 48;
  const int phase = (int)(roadOffset_ % kDashPeriod);
  for (int y = phase - kDashPeriod; y < H; y += kDashPeriod) {
    int dy = y, dh = kDashLen;
    if (dy < 0) { dh += dy; dy = 0; }
    if (dy + dh > H) dh = H - dy;
    if (dh > 0) d.gameFillRect(85, dy, 2, dh, kDash);
  }
}

void RoadFighter::drawCar(DisplayManager& d, int x, int y, int w, int h,
                           uint16_t body, bool player) const {
  d.gameFillRect(x, y, w, h, body);

  // Windshield near front (top for player, bottom for enemy)
  const int winY = player ? y + 4 : y + h - 14;
  d.gameFillRect(x + 2, winY, w - 4, 8, kWindshield);

  // Tires (±2 px outside body)
  d.gameFillRect(x - 2, y + 3,         3, 7, kBlack);
  d.gameFillRect(x + w - 1, y + 3,     3, 7, kBlack);
  d.gameFillRect(x - 2, y + h - 10,    3, 7, kBlack);
  d.gameFillRect(x + w - 1, y + h - 10, 3, 7, kBlack);

  if (player) {
    // Headlights at top
    d.gameFillRect(x + 1,     y + 1, 3, 2, kWhite);
    d.gameFillRect(x + w - 4, y + 1, 3, 2, kWhite);
  } else {
    // Red taillights at bottom (enemy faces toward player)
    d.gameFillRect(x + 1,     y + h - 3, 3, 2, kRed);
    d.gameFillRect(x + w - 4, y + h - 3, 3, 2, kRed);
  }
}

void RoadFighter::render(DisplayManager& display) const {
  display.gameBegin();

  drawRoad(display);

  for (const auto& e : enemies_) {
    if (!e.active) continue;
    drawCar(display, e.x, e.y, kEnemyW, kEnemyH, e.color, false);
  }

  // Player (red if just crashed)
  drawCar(display, playerX_, kPlayerBaseY, kPlayerW, kPlayerH,
          gameOver_ ? kRed : kSilver, true);

  // HUD: opaque black strip so score is always readable
  display.gameFillRect(0, 0, W, 22, kBlack);
  char buf[20];
  snprintf(buf, sizeof(buf), "SCORE %05lu", (unsigned long)score_);
  display.gameDrawText(buf, 4, 5, kWhite, 2);

  if (gameOver_) {
    display.gameFillRect(16, H / 2 - 28, W - 32, 56, kGameOverBg);
    display.gameDrawText("GAME OVER",    28, H / 2 - 22, kRed,   2);
    display.gameDrawText("BOOT RESTART", 14, H / 2 + 2,  kWhite, 1);
    display.gameDrawText("PWR  EXIT",    32, H / 2 + 12, kWhite, 1);
  }

  display.gameCommit();
}
