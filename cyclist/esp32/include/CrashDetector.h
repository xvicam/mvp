#pragma once

#include <Arduino.h>

#include "AccGyro.h"

namespace crash {

enum class State : uint8_t { Armed, Candidate, Confirmed, Cooldown };

struct CrashEvent {
  bool triggered{false};
  uint32_t crashId{0};
  float peakDynamicMps2{0};
};

class CrashDetector {
public:

  // Easier-to-trigger defaults for testing/simulation.
  float triggerDynamicMps2 = 9.0f;      // was 11.0f
  float confirmStillDynMps2 = 1.0f;     // was 0.8f

  uint32_t candidateHoldMs = 50;        // was 80
  uint32_t postConfirmWindowMs = 1200; // was 900 (more time to meet conditions)
  uint32_t cooldownMs = 8000;

  bool requireMoving = true;
  uint32_t movingGraceMs = 2500;       // was 1200 (more forgiving for testing)

  CrashDetector() = default;

  State state() const { return _state; }
  uint32_t crashId() const { return _crashId; }
  float lastPeakDynamic() const { return _peakDynamic; }

  // Returns an event that is triggered exactly once per confirmed crash.
  CrashEvent update(uint32_t nowMs, const ImuSample& s, const ImuOrientation& o);

  void resetToArmed(uint32_t nowMs);

private:
  State _state{State::Armed};
  uint32_t _stateSinceMs{0};

  uint32_t _movingSeenAtMs{0};

  float _peakDynamic{0};
  float _peakMag{0};

  uint32_t _crashId{0};

  bool movingRecently(uint32_t nowMs) const;
};

} // namespace crash

