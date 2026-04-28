#pragma once

#include <Arduino.h>

namespace gnss {
  struct Fix {
    bool valid;
    double latDeg;
    double lngDeg;
    uint8_t satsUsed;
    uint32_t updatedAtMs;
  };

  // Initialise the GNSS module on I2C.
  // Returns true if the module responded.
  bool init();

  // Poll GNSS
  void update(uint32_t nowMs);

  Fix lastFix();

  inline bool hasFix(uint32_t maxAgeMs = 15000) {
    Fix f = lastFix();
    return f.valid && (millis() - f.updatedAtMs) <= maxAgeMs;
  }
}

