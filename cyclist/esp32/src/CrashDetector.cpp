#include "../include/CrashDetector.h"

#include <cmath>
#include <cstring>

namespace crash {

void CrashDetector::resetToArmed(uint32_t nowMs) {
  _state = State::Armed;
  _stateSinceMs = nowMs;
  _peakDynamic = 0;
  _peakMag = 0;
}

bool CrashDetector::movingRecently(uint32_t nowMs) const {
  return _movingSeenAtMs != 0 && (nowMs - _movingSeenAtMs) <= movingGraceMs;
}

CrashEvent CrashDetector::update(uint32_t nowMs, const ImuSample& /*s*/, const ImuOrientation& o) {
  CrashEvent ev{};

  // dynamic derived from magnitude HP relative to magnitude LP baseline
  const float dynamic = std::fabs(o.accelMagnitude - o.accelMagLp);

  if (o.isMoving) _movingSeenAtMs = nowMs;

  // track peaks regardless of state (for diagnostics)
  if (dynamic > _peakDynamic) _peakDynamic = dynamic;
  if (o.accelMagnitude > _peakMag) _peakMag = o.accelMagnitude;

  switch (_state) {
    case State::Armed: {
      // moving gate
      if (requireMoving && !movingRecently(nowMs)) {
        // still update peaks but don't arm a candidate yet
        _peakDynamic = dynamic; // keep it from accumulating forever
        _peakMag = o.accelMagnitude;
        return ev;
      }

      if (dynamic >= triggerDynamicMps2) {
        _state = State::Candidate;
        _stateSinceMs = nowMs;
        _peakDynamic = dynamic;
        _peakMag = o.accelMagnitude;
      }
      return ev;
    }

    case State::Candidate: {
      // If spike persists long enough, move to confirmation window.
      if (dynamic < (0.7f * triggerDynamicMps2)) {
        // dropped too quickly -> likely a bump
        resetToArmed(nowMs);
        return ev;
      }

      if (nowMs - _stateSinceMs >= candidateHoldMs) {
        _state = State::Confirmed;
        _stateSinceMs = nowMs;
      }
      return ev;
    }

    case State::Confirmed: {
      // Confirm crash if we observe post-impact stillness OR non-upright orientation.
      // This filters some high-vibration false positives while still triggering quickly.
      const bool isStill = dynamic <= confirmStillDynMps2;
      const bool notUpright = (std::strcmp(o.orientationLabel, "Upright") != 0);

      if (isStill || notUpright) {
        _crashId++;
        ev.triggered = true;
        ev.crashId = _crashId;
        ev.peakDynamicMps2 = _peakDynamic;

        _state = State::Cooldown;
        _stateSinceMs = nowMs;
        return ev;
      }

      if (nowMs - _stateSinceMs >= postConfirmWindowMs) {
        // didn't confirm within the window
        resetToArmed(nowMs);
      }
      return ev;
    }

    case State::Cooldown: {
      if (nowMs - _stateSinceMs >= cooldownMs) {
        resetToArmed(nowMs);
      }
      return ev;
    }
  }

  return ev;
}

} // namespace crash

