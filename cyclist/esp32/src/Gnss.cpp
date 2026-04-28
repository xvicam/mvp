#include "../include/Gnss.h"

#include <Wire.h>
#include <DFRobot_GNSS.h>

namespace {
  // TEL0157 default I2C address per DFRobot_GNSS library.
  constexpr uint8_t kGnssAddr = 0x20;

  // Polling interval. The module updates internally at 1Hz by default.
  constexpr uint32_t kPollMs = 500;

  DFRobot_GNSS_I2C g_gnss(&Wire, kGnssAddr);
  bool g_inited = false;
  uint32_t g_lastPollMs = 0;
  gnss::Fix g_last{false, 0.0, 0.0, 0, 0};

  static bool isValidCoord(double lat, double lng) {
    // filters out 0,0 before a fix.
    if (lat < -90.0 || lat > 90.0) return false;
    if (lng < -180.0 || lng > 180.0) return false;
    if (lat == 0.0 && lng == 0.0) return false;
    return true;
  }
}

namespace gnss {
  bool init() {
    // If another module already started I2C, Wire.begin() is harmless on ESP32.
    Wire.begin();
    Wire.setClock(100000);

    g_inited = g_gnss.begin();
    if (!g_inited) {
      g_last.valid = false;
      return false;
    }

    // Use GPS + BeiDou by default (matches your module).
    g_gnss.setGnss(eGPS_BeiDou);
    g_gnss.enablePower();
    g_gnss.setRgbOn();
    return true;
  }

  void update(uint32_t nowMs) {
    if (!g_inited) return;
    if (nowMs - g_lastPollMs < kPollMs) return;
    g_lastPollMs = nowMs;

    // Read degree-form coordinates (already converted by the library).
    const sLonLat_t lat = g_gnss.getLat();
    const sLonLat_t lon = g_gnss.getLon();
    const uint8_t sats = g_gnss.getNumSatUsed();

    const double latDeg = lat.latitudeDegree;
    const double lngDeg = lon.lonitudeDegree;

    const bool ok = (sats > 0) && isValidCoord(latDeg, lngDeg);
    if (ok) {
      g_last.valid = true;
      g_last.latDeg = latDeg;
      g_last.lngDeg = lngDeg;
      g_last.satsUsed = sats;
      g_last.updatedAtMs = nowMs;
    } else {
      // Keep last known coordinates, but mark invalid if we've never had a fix.
      if (!g_last.valid) {
        g_last.valid = false;
        g_last.satsUsed = sats;
      }
    }
  }

  Fix lastFix() {
    return g_last;
  }
}


