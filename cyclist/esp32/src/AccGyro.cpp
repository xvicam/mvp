#include "AccGyro.h"

#include <cmath>

namespace {
  constexpr float kGravity = 9.81f; // m/s^2
  constexpr float kRadToDeg = 180.0f / static_cast<float>(PI);

  // Orientation thresholds (m/s^2). Lower = wider range.
  constexpr float kUprightThreshold = 7.5f;

  // Motion detection tuning.
  // We track accel magnitude high-pass energy so "tilt" doesn't look like motion.
  // These numbers assume you're sampling around ~50Hz.
  constexpr float kMagLpAlpha = 0.08f;     // lower = slower baseline
  constexpr float kHpAbsLpAlpha = 0.25f;   // smoothing for motion score
  constexpr float kMoveOn = 0.35f;         // motion score threshold to latch moving (m/s^2)
  constexpr float kMoveOff = 0.20f;        // lower threshold to unlatch (hysteresis)
}

bool AccGyro::beginI2C(TwoWire &wire) {
  // Adafruit driver uses the global Wire by default, but accepts a pointer.
  return _sox.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &wire);
}

bool AccGyro::readSample(ImuSample &out) {
  sensors_event_t accel, gyro, temp;
  _sox.getEvent(&accel, &gyro, &temp);

  out.ax = accel.acceleration.x;
  out.ay = accel.acceleration.y;
  out.az = accel.acceleration.z;

  out.gx = gyro.gyro.x;
  out.gy = gyro.gyro.y;
  out.gz = gyro.gyro.z;

  out.temperatureC = temp.temperature;
  return true;
}

ImuOrientation AccGyro::computeOrientation(const ImuSample &s) {
  ImuOrientation o;

  o.pitchDeg = std::atan2(s.ax, std::sqrt((s.ay * s.ay) + (s.az * s.az))) * kRadToDeg;
  o.rollDeg = std::atan2(s.ay, std::sqrt((s.ax * s.ax) + (s.az * s.az))) * kRadToDeg;

  if (s.az > kUprightThreshold)
    o.orientationLabel = "Upright";
  else if (s.az < -kUprightThreshold)
    o.orientationLabel = "Downwards";
  else
    o.orientationLabel = "Sideways";

  o.accelMagnitude = std::sqrt((s.ax * s.ax) + (s.ay * s.ay) + (s.az * s.az));

  // Legacy low-pass filter on magnitude (kept because other code prints it).
  _filteredMagnitude = (0.2f * o.accelMagnitude) + (0.8f * _filteredMagnitude);
  o.filteredMagnitude = _filteredMagnitude;

  // Improved motion detection:
  // - Low-pass the magnitude to estimate the gravity/slow baseline
  // - High-pass component captures vibration/bumps/vehicle dynamics
  // - Smooth |HP| into a motion score and apply hysteresis
  _magLp = (kMagLpAlpha * o.accelMagnitude) + ((1.0f - kMagLpAlpha) * _magLp);
  const float hp = o.accelMagnitude - _magLp;
  _hpAbsLp = (kHpAbsLpAlpha * std::fabs(hp)) + ((1.0f - kHpAbsLpAlpha) * _hpAbsLp);

  if (_movingLatched) {
    if (_hpAbsLp < kMoveOff) _movingLatched = false;
  } else {
    if (_hpAbsLp > kMoveOn) _movingLatched = true;
  }

  o.accelMagLp = _magLp;
  o.accelMagHp = hp;
  o.motionScore = _hpAbsLp;
  o.isMoving = _movingLatched;

  return o;
}
