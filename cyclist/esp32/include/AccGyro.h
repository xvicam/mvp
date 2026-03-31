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

private:
  Adafruit_LSM6DSOX _sox;

  // Legacy low-pass magnitude (kept for backward compatibility)
  float _filteredMagnitude{9.81f};

  // Motion detection filters/state
  float _magLp{9.81f};
  float _hpAbsLp{0.0f};
  bool _movingLatched{false};
};
