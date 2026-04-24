#include "AccGyro.h"

#include <cmath>

// LSM6DSOX register map (subset used here)
namespace {
  constexpr uint8_t REG_TAP_CFG2 = 0x58;
  constexpr uint8_t REG_WAKE_UP_THS = 0x5B;
  constexpr uint8_t REG_WAKE_UP_DUR = 0x5C;
  constexpr uint8_t REG_FREE_FALL = 0x5D;
  constexpr uint8_t REG_MD1_CFG = 0x5E;
  constexpr uint8_t REG_MD2_CFG = 0x5F;
  constexpr uint8_t REG_WAKE_UP_SRC = 0x1B;
  constexpr uint8_t REG_TAP_SRC = 0x1C;
}

namespace {
  constexpr float kGravity = 9.81f; // m/s^2
  constexpr float kRadToDeg = 180.0f / static_cast<float>(PI);

  // Orientation thresholds (m/s^2). Lower = easier to classify as not-upright.
  constexpr float kUprightThreshold = 8.0f; // was 8.5f

  // Motion detection tuning.
  // track accel magnitude high-pass energy so "tilt" doesn't look like motion.

  constexpr float kMagLpAlpha = 0.08f;
  constexpr float kHpAbsLpAlpha = 0.25f;
  constexpr float kMoveOn = 0.45f;   // was 0.55f
  constexpr float kMoveOff = 0.28f;  // was 0.35f
}

bool AccGyro::beginI2C(TwoWire &wire) {
  // Adafruit driver uses the global Wire by default, but accepts a pointer.
  return _sox.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &wire);
}

bool AccGyro::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LSM6DS_I2CADDR_DEFAULT);
  Wire.write(reg);
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

bool AccGyro::readReg(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(LSM6DS_I2CADDR_DEFAULT);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(static_cast<uint8_t>(LSM6DS_I2CADDR_DEFAULT), static_cast<uint8_t>(1)) != 1) return false;
  value = Wire.read();
  return true;
}

bool AccGyro::configureWakeInactivity(uint8_t inactivitySeconds,
                                      uint8_t wakeThresholdMg,
                                      bool routeToInt1,
                                      bool activeHigh,
                                      bool pushPull) {

  // Interrupt pin configuration (push-pull/open-drain and polarity)
  constexpr uint8_t REG_CTRL3_C = 0x12;
  uint8_t ctrl3 = 0;
  if (!readReg(REG_CTRL3_C, ctrl3)) return false;
  if (pushPull) ctrl3 &= ~(1u << 4); else ctrl3 |= (1u << 4);
  // H_LACTIVE: 0 = active high, 1 = active low
  if (activeHigh) ctrl3 &= ~(1u << 5); else ctrl3 |= (1u << 5);
  if (!writeReg(REG_CTRL3_C, ctrl3)) return false;

  // Latch interrupt so the ESP32 sees a stable level when sleeping.
  uint8_t tapCfg2 = 0;
  if (!readReg(REG_TAP_CFG2, tapCfg2)) return false;
  tapCfg2 |= 0x81;
  if (!writeReg(REG_TAP_CFG2, tapCfg2)) return false;

  // Set wake-up threshold. WAKE_UP_THS lower 6 bits are threshold.
  const uint8_t thsSteps = static_cast<uint8_t>(std::min<uint16_t>(63, (wakeThresholdMg + 31) / 62));
  uint8_t wakeThs = 0;
  if (!readReg(REG_WAKE_UP_THS, wakeThs)) return false;
  wakeThs = static_cast<uint8_t>((wakeThs & 0xC0) | (thsSteps & 0x3F));
  if (!writeReg(REG_WAKE_UP_THS, wakeThs)) return false;

  // Wake-up duration / sleep duration for inactivity generation.
  const uint8_t sleepDur = static_cast<uint8_t>(std::min<uint8_t>(15, static_cast<uint8_t>((inactivitySeconds + 7) / 8)));
  const uint8_t wakeUpDur = static_cast<uint8_t>((sleepDur << 4) | 0x00);
  if (!writeReg(REG_WAKE_UP_DUR, wakeUpDur)) return false;

  uint8_t md1 = 0, md2 = 0;
  if (!readReg(REG_MD1_CFG, md1)) return false;
  if (!readReg(REG_MD2_CFG, md2)) return false;

  // Clear existing routing for both pins.
  md1 &= static_cast<uint8_t>(~((1u << 5) | (1u << 4))); // WU, SLEEP_CHANGE
  md2 &= static_cast<uint8_t>(~((1u << 5) | (1u << 4)));

  if (routeToInt1) {
    md1 |= static_cast<uint8_t>((1u << 5) | (1u << 4));
  } else {
    md2 |= static_cast<uint8_t>((1u << 5) | (1u << 4));
  }
  if (!writeReg(REG_MD1_CFG, md1)) return false;
  if (!writeReg(REG_MD2_CFG, md2)) return false;

  // Read sources once to clear any pending latched interrupt before use.
  bool wu = false, inact = false;
  (void)readWakeInactivitySources(wu, inact);
  return true;
}

bool AccGyro::readWakeInactivitySources(bool &wakeUp, bool &inactivity) {
  wakeUp = false;
  inactivity = false;
  uint8_t wakeSrc = 0;
  if (!readReg(REG_WAKE_UP_SRC, wakeSrc)) return false;
  // WAKE_UP_SRC bits:
  // 0 = Z_WU, 1 = Y_WU, 2 = X_WU
  // 3 = WU_IA (wake-up event)
  // 4 = SLEEP_STATE
  // 5 = FF_IA
  // 6 = SLEEP_CHANGE_IA
  if (wakeSrc & (1u << 3)) wakeUp = true;
  if (wakeSrc & (1u << 6)) inactivity = true;

  // TAP_SRC is commonly read to clear latched logic in some configs.
  uint8_t tapSrc = 0;
  (void)readReg(REG_TAP_SRC, tapSrc);
  return wakeUp || inactivity;
}

void AccGyro::disableWakeInactivity() {
  uint8_t md1 = 0, md2 = 0;
  if (readReg(REG_MD1_CFG, md1)) {
    md1 &= static_cast<uint8_t>(~((1u << 5) | (1u << 4)));
    (void)writeReg(REG_MD1_CFG, md1);
  }
  if (readReg(REG_MD2_CFG, md2)) {
    md2 &= static_cast<uint8_t>(~((1u << 5) | (1u << 4)));
    (void)writeReg(REG_MD2_CFG, md2);
  }
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
