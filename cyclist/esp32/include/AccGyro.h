#pragma once

#include <Arduino.h>
#include <Adafruit_LSM6DSOX.h>

struct ImuSample {
  float ax{0};
  float ay{0};
  float az{0};
  float gx{0};
  float gy{0};
  float gz{0};
  float temperatureC{0};
};

struct ImuOrientation {
  float pitchDeg{0};
  float rollDeg{0};
  const char *orientationLabel{"Unknown"};
  float accelMagnitude{0};
  float filteredMagnitude{9.81f};
  bool isMoving{false};

  // Extra debug-friendly motion metrics
  float accelMagLp{9.81f};
  float accelMagHp{0};
  float motionScore{0};
};

class AccGyro {
public:
  bool beginI2C(TwoWire &wire = Wire);
  bool readSample(ImuSample &out);
  ImuOrientation computeOrientation(const ImuSample &s);

  // Configure the LSM6DSOX embedded wake-up / inactivity engine.
  bool configureWakeInactivity(uint8_t inactivitySeconds = 120,
                               uint8_t wakeThresholdMg = 62,
                               bool routeToInt1 = true,
                               bool activeHigh = true,
                               bool pushPull = true);

  // Read and clear latched interrupt sources (if latched enabled).
  // Returns true if at least one of the flags is set.
  bool readWakeInactivitySources(bool &wakeUp, bool &inactivity);

  // disable routed wake/inactivity interrupts.
  void disableWakeInactivity();

private:
  Adafruit_LSM6DSOX _sox;

  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &value);

  // Legacy low-pass magnitude (kept for backward compatibility)
  float _filteredMagnitude{9.81f};

  // Motion detection filters/state
  float _magLp{9.81f};
  float _hpAbsLp{0.0f};
  bool _movingLatched{false};
};
