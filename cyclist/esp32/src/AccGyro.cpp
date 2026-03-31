#include "AccGyro.h"

#include <cmath>

namespace {
  constexpr float kGravity = 9.81f; // m/s^2
  constexpr float kRadToDeg = 180.0f / static_cast<float>(PI);

  // Orientation thresholds (m/s^2). Lower = wider range.
  constexpr float kUprightThreshold = 7.5f;
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

  // Low-pass filter on magnitude to smooth movement detection.
  _filteredMagnitude = (0.2f * o.accelMagnitude) + (0.8f * _filteredMagnitude);
  o.filteredMagnitude = _filteredMagnitude;

  constexpr float movementThreshold = 0.4f;
  o.isMoving = std::fabs(o.filteredMagnitude - kGravity) > movementThreshold;

  return o;
}
