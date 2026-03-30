#include "AccGyro.h"

#include <math.h>

namespace {
constexpr float kGravity = 9.81f; // m/s^2
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

  o.pitchDeg = atan2f(s.ax, sqrtf(s.ay * s.ay + s.az * s.az)) * 180.0f / PI;
  o.rollDeg = atan2f(s.ay, sqrtf(s.ax * s.ax + s.az * s.az)) * 180.0f / PI;

  if (s.az > 8.0f)
    o.orientationLabel = "Upright";
  else if (s.az < -8.0f)
    o.orientationLabel = "Downwards";
  else
    o.orientationLabel = "Sideways";

  o.accelMagnitude = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);

  // Low-pass filter on magnitude to smooth movement detection.
  _filteredMagnitude = (0.2f * o.accelMagnitude) + (0.8f * _filteredMagnitude);
  o.filteredMagnitude = _filteredMagnitude;

  constexpr float movementThreshold = 0.4f;
  o.isMoving = fabsf(o.filteredMagnitude - kGravity) > movementThreshold;

  return o;
}
