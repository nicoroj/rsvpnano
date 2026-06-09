#pragma once

#include <Arduino.h>

class DisplayManager;

class RoadFighter {
 public:
  static constexpr int W = 172;  // portrait game width  (panel native width)
  static constexpr int H = 640;  // portrait game height (panel native height)

  void begin();
  void reset();

  // steer: -1.0 = full left, +1.0 = full right (from accelerometer y-axis)
  void update(uint32_t nowMs, float steer);
  void render(DisplayManager& display) const;

  bool     isGameOver() const { return gameOver_; }
  uint32_t score()      const { return score_; }

 private:
  struct Enemy {
    int16_t  x      = 0;
    int16_t  y      = 0;
    uint16_t color  = 0;
    uint8_t  speed  = 0;
    bool     active = false;
  };

  static constexpr int kRoadLeft    = 26;
  static constexpr int kRoadRight   = 146;
  static constexpr int kPlayerW     = 22;
  static constexpr int kPlayerH     = 36;
  static constexpr int kPlayerBaseY = H - 70;
  static constexpr int kEnemyW      = 20;
  static constexpr int kEnemyH      = 32;
  static constexpr int kMaxEnemies  = 5;
  static constexpr int kBaseSpeed   = 5;
  static constexpr int kMaxMoveSpeed = 7;  // px/frame at full tilt
  static constexpr uint32_t kFrameMs = 60;

  int16_t  playerX_     = 0;
  int32_t  roadOffset_  = 0;
  uint32_t score_       = 0;
  uint32_t lastFrameMs_ = 0;
  uint32_t lastSpawnMs_ = 0;
  uint32_t rng_         = 98317;
  bool     gameOver_    = false;
  Enemy    enemies_[kMaxEnemies] = {};

  uint32_t nextRng();
  void     spawnEnemy();
  int      currentSpeed() const;
  void     drawRoad(DisplayManager& d) const;
  void     drawCar(DisplayManager& d, int x, int y, int w, int h,
                   uint16_t body, bool player) const;
  bool     overlaps(int ax, int ay, int aw, int ah,
                    int bx, int by, int bw, int bh) const;
};
