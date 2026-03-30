#include "CrashDetector.h"
#include <math.h>
namespace {
constexpr float kGravity = 9.81f;
}
float CrashDetector::accelMag(const ImuSample &s) const {
  return sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
}
float CrashDetector::gyroMag(const ImuSample &s) const {
  return sqrtf(s.gx * s.gx + s.gy * s.gy + s.gz * s.gz);
}
float CrashDetector::jerkFrom(const ImuSample &prev, const ImuSample &cur, float dtSec) const {
  if (dtSec <= 0.0f) return 0.0f;
  const float dax = (cur.ax - prev.ax) / dtSec;
  const float day = (cur.ay - prev.ay) / dtSec;
  const float daz = (cur.az - prev.az) / dtSec;
  return sqrtf(dax * dax + day * day + daz * daz);
}
void CrashDetector::reset() {
  _hasPrev = false;
  _armed = false;
  _armedAtMs = 0;
  _peakAccel = 0;
  _peakGyro = 0;
}
CrashEvent CrashDetector::update(const ImuSample &s, uint32_t nowMs) {
  CrashEvent ev;
  // Cooldown
  if (_lastTriggerMs != 0 && (nowMs - _lastTriggerMs) < _cfg.cooldownMs) {
    _prev = s;
    _prevMs = nowMs;
    _hasPrev = true;
    return ev;
  }
  const float aMag = accelMag(s);
  const float gMag = gyroMag(s);
  float jerk = 0.0f;
  if (_hasPrev) {
    const float dt = (nowMs - _prevMs) / 1000.0f;
    jerk = jerkFrom(_prev, s, dt);
  }
  // Track peaks while armed.
  if (_armed) {
    if (aMag > _peakAccel) _peakAccel = aMag;
    if (gMag > _peakGyro) _peakGyro = gMag;
  }
  // 1) Arm on impact-like event: high accel spike OR very high jerk.
  const bool impact = (aMag >= _cfg.impactAccelThreshold) || (jerk >= _cfg.jerkThreshold);
  // Optional: sometimes crashes include a brief unloading/free-fall, then impact.
  const bool freeFallLike = (aMag <= _cfg.minFreeFallThreshold);
  if (!_armed && (impact || freeFallLike)) {
    _armed = true;
    _armedAtMs = nowMs;
    _peakAccel = aMag;
    _peakGyro = gMag;
  }
  // 2) Confirm within window: significant rotation.
  if (_armed) {
    const uint32_t age = nowMs - _armedAtMs;
    const bool rotation = (gMag >= _cfg.gyroThreshold);
    // 3) Optional extra confidence: after the event, device becomes mostly still.
    const bool inStillnessTime = age >= _cfg.stationaryAfterMs;
    const bool accelNearGravity = fabsf(aMag - kGravity) <= _cfg.stationaryAccelEps;
    const bool gyroLow = gMag <= _cfg.stationaryGyroMax;
    const bool stationary = inStillnessTime && accelNearGravity && gyroLow;
    if ((impact && rotation) || (rotation && (_peakAccel >= _cfg.impactAccelThreshold)) || stationary) {
      // Trigger.
      ev.triggered = true;
      ev.accelMag = aMag;
      ev.jerk = jerk;
      ev.gyroMag = gMag;
      ev.peakAccelMag = _peakAccel;
      ev.peakGyroMag = _peakGyro;
      ev.timestampMs = nowMs;
      _lastTriggerMs = nowMs;
      _armed = false;
    } else if (age > _cfg.impactWindowMs) {
      // Window expired.
      _armed = false;
    }
  }
  _prev = s;
  _prevMs = nowMs;
  _hasPrev = true;
  return ev;
}
