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

  float triggerDynamicMps2 = 8.0f;     // ~0.82 g dynamic spike
  float confirmStillDynMps2 = 1.2f;    // "still" when dynamic below this

  uint32_t candidateHoldMs = 40;       // require spike for at least this long
  uint32_t postConfirmWindowMs = 800;  // after candidate, look for stillness/orientation
  uint32_t cooldownMs = 8000;          // suppression after a crash

  // only trigger if we were moving recently.
  // Helps avoid "bike picked up and dropped" false positives.
  bool requireMoving = true;
  uint32_t movingGraceMs = 2000;

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

