#pragma once

#include <Arduino.h>

#include "AccGyro.h"

struct CrashEvent {
  bool triggered{false};
  float accelMag{0};
  float jerk{0};          // m/s^3 (approx)
  float gyroMag{0};       // rad/s
  float peakAccelMag{0};  // m/s^2
  float peakGyroMag{0};   // rad/s
  uint32_t timestampMs{0};
};

// Simple heuristic crash detector intended for a bike-mounted/helmet-mounted IMU.
// It looks for a combination of high acceleration spike (impact), high jerk,
// and high rotation rate within a short time window.
class CrashDetector {
 public:
  struct Config {
    float impactAccelThreshold = 25.0f;   // m/s^2  (~2.5g)
    float minFreeFallThreshold = 3.0f;    // m/s^2  (optional: brief unloading)
    float jerkThreshold = 120.0f;         // m/s^3
    float gyroThreshold = 5.0f;           // rad/s  (~286 deg/s)

    uint32_t impactWindowMs = 350;        // look for rotation within this window
    uint32_t cooldownMs = 3000;           // ignore new triggers for a while
    uint32_t stationaryAfterMs = 1200;    // optional: confirm by low motion

    float stationaryAccelEps = 1.2f;      // m/s^2 difference from gravity
    float stationaryGyroMax = 1.0f;       // rad/s
  };

  static Config defaultConfig() { return Config{}; }

  explicit CrashDetector(const Config &cfg = defaultConfig()) : _cfg(cfg) {}

  CrashEvent update(const ImuSample &s, uint32_t nowMs);
  void reset();

 private:
  Config _cfg;

  bool _hasPrev{false};
  ImuSample _prev{};
  uint32_t _prevMs{0};

  // State for impact->rotation confirmation.
  bool _armed{false};
  uint32_t _armedAtMs{0};
  float _peakAccel{0};
  float _peakGyro{0};

  uint32_t _lastTriggerMs{0};

  float accelMag(const ImuSample &s) const;
  float gyroMag(const ImuSample &s) const;
  float jerkFrom(const ImuSample &prev, const ImuSample &cur, float dtSec) const;
};
