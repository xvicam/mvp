// risk_calculator
// ───────────────
// GPS mode:    Three-tier proximity with approach detection. Only considers
//              cyclists who have actually reported GPS coordinates.
//              Safe = no cyclists nearby (or GPS invalid).
//              Alert / Warning / Danger based on distance + approach + TTC.
//
// RSSI mode:   RSSI-band thresholds with hysteresis. No GPS required.
//
// Remote mode: Cyclist sends state directly (cri 0-3 in JSON). Returns the
//              highest severity among whitelisted active cyclists.

#include "risk_calculator.h"
#include "config.h"
#include "cyclist_store.h"
#include "helpers.h"

#include <TinyGPS++.h>
#include <math.h>
#include <string.h>

namespace risk_calculator {

    // ── Per-slot approach tracking (GPS mode) ────────────────────────────────

    struct SlotTracking {
        bool     is_initialized    = false;
        uint8_t  known_mac[6]      = {0};
        bool     has_history       = false;
        double   smoothed_distance = 0.0;
        uint32_t last_update_ms    = 0;
        double   closing_speed_mps = 0.0;
        bool     is_approaching    = false;
    };

    static SlotTracking slot_trackings[config::max_cyclists];

    static double closest_distance_m    = -1.0;
    static int    closest_cyclist_index = -1;

    static State  last_signal_state = State::Safe;
    static bool   has_signal_state  = false;

    static CollisionResult make_result(State state) {
        CollisionResult r;
        r.state                 = state;
        r.closest_distance_m    = closest_distance_m;
        r.closest_cyclist_index = closest_cyclist_index;
        return r;
    }

    static void reset_slot(SlotTracking& t, const uint8_t mac[6]) {
        t = SlotTracking{};
        t.is_initialized = true;
        memcpy(t.known_mac, mac, 6);
    }

    static void update_slot_tracking(
        uint8_t slot,
        const uint8_t mac[6],
        double raw_distance_m,
        uint32_t now
    ) {
        SlotTracking& t = slot_trackings[slot];

        if (!t.is_initialized || !helpers::is_same_mac(t.known_mac, mac)) {
            reset_slot(t, mac);
        }

        if (!t.has_history) {
            t.smoothed_distance = raw_distance_m;
            t.last_update_ms    = now;
            t.has_history       = true;
            return;
        }

        const uint32_t dt_ms = now - t.last_update_ms;
        if (dt_ms < config::tracking_interval_ms) return;

        const double prev = t.smoothed_distance;
        t.smoothed_distance =
            config::distance_ema_alpha * raw_distance_m +
            (1.0 - config::distance_ema_alpha) * t.smoothed_distance;

        const double dt_s = dt_ms / 1000.0;
        t.closing_speed_mps = (prev - t.smoothed_distance) / dt_s;

        if (!t.is_approaching &&
            t.closing_speed_mps >= config::approach_on_speed_mps)
        {
            t.is_approaching = true;
        } else if (t.is_approaching &&
                   t.closing_speed_mps <= config::approach_off_speed_mps)
        {
            t.is_approaching = false;
        }

        t.last_update_ms = now;
    }

    // ── GPS mode ─────────────────────────────────────────────────────────────

    CollisionResult calculate_collision_risk(
        const VehicleData& vehicle_data,
        const CyclistsData& cyclists_data
    ) {
        closest_distance_m    = -1.0;
        closest_cyclist_index = -1;

        if (!vehicle_data.is_gps_valid) {
            return make_result(State::Safe);  // GPS LED conveys acquiring status
        }

        cyclist_store::remove_expired_cyclists();
        const uint32_t now = millis();

        for (uint8_t i = 0; i < cyclists_data.slot_count; i++) {
            const CyclistData& cyclist = cyclists_data.cyclists[i];
            if (!cyclist.is_active)    continue;
            if (!cyclist.has_gps_data) continue;  // skip remote-only cyclists

            const double raw_m = TinyGPSPlus::distanceBetween(
                vehicle_data.lat, vehicle_data.lng,
                cyclist.lat,      cyclist.lng
            );

            update_slot_tracking(i, cyclist.mac, raw_m, now);

            const double eff_m = slot_trackings[i].smoothed_distance;
            if (closest_distance_m < 0 || eff_m < closest_distance_m) {
                closest_distance_m    = eff_m;
                closest_cyclist_index = i;
            }
        }

        if (closest_cyclist_index < 0) {
            return make_result(State::Safe);
        }

        const SlotTracking& t = slot_trackings[closest_cyclist_index];

        if (t.smoothed_distance <= config::collision_distance_m) {
            return make_result(State::Danger);
        }

        if (t.is_approaching && t.closing_speed_mps > 0.0) {
            const double ttc_s = t.smoothed_distance / t.closing_speed_mps;
            if (ttc_s <= config::ttc_danger_s) {
                return make_result(State::Danger);
            }
        }

        if (t.smoothed_distance <= config::alert_distance_m) {
            return make_result(t.is_approaching ? State::Warning : State::Alert);
        }

        return make_result(State::Safe);
    }

    // ── RSSI mode ─────────────────────────────────────────────────────────────

    static double estimate_distance_from_rssi(float rssi_dbm) {
        if (rssi_dbm >= 0.0f) return -1.0;
        const double exponent =
            (config::rssi_ref_dbm - static_cast<double>(rssi_dbm)) /
            (10.0 * config::rssi_path_loss);
        return pow(10.0, exponent);
    }

    static State signal_state_from_rssi(float rssi_dbm) {
        if (rssi_dbm >= config::signal_rssi_danger_dbm)  return State::Danger;
        if (rssi_dbm >= config::signal_rssi_warning_dbm) return State::Warning;
        if (rssi_dbm >= config::signal_rssi_alert_dbm)   return State::Alert;
        return State::Safe;
    }

    static int state_rank(State state) {
        switch (state) {
            case State::Danger:  return 3;
            case State::Warning: return 2;
            case State::Alert:   return 1;
            case State::Safe:    return 0;
        }
        return 0;
    }

    static float signal_threshold(State state) {
        switch (state) {
            case State::Danger:  return static_cast<float>(config::signal_rssi_danger_dbm);
            case State::Warning: return static_cast<float>(config::signal_rssi_warning_dbm);
            case State::Alert:   return static_cast<float>(config::signal_rssi_alert_dbm);
            default:             return static_cast<float>(config::signal_rssi_safe_dbm);
        }
    }

    static State apply_signal_hysteresis(float rssi_dbm, State previous) {
        const State raw = signal_state_from_rssi(rssi_dbm);

        // Stepping up (more severe): take the new state immediately.
        if (state_rank(raw) >= state_rank(previous)) return raw;

        // Stepping down: require the signal to drop below the previous band's
        // threshold by the hysteresis margin before releasing.
        const float threshold = signal_threshold(previous);
        if (rssi_dbm < (threshold - config::signal_rssi_hysteresis_db)) return raw;
        return previous;
    }

    CollisionResult calculate_signal_risk(const CyclistsData& cyclists_data) {
        closest_distance_m    = -1.0;
        closest_cyclist_index = -1;

        cyclist_store::remove_expired_cyclists();

        float strongest_rssi = -127.0f;

        for (uint8_t i = 0; i < cyclists_data.slot_count; i++) {
            const CyclistData& cyclist = cyclists_data.cyclists[i];
            if (!cyclist.is_active || !cyclist.has_rssi) continue;

            const float  rssi = cyclist.rssi_smoothed_dbm;
            const double dist = estimate_distance_from_rssi(rssi);
            if (dist < 0) continue;

            if (closest_distance_m < 0 || dist < closest_distance_m) {
                closest_distance_m    = dist;
                closest_cyclist_index = i;
            }
            if (rssi > strongest_rssi) strongest_rssi = rssi;
        }

        if (closest_cyclist_index < 0) {
            last_signal_state = State::Safe;
            has_signal_state  = true;
            return make_result(State::Safe);
        }

        const State next = has_signal_state
            ? apply_signal_hysteresis(strongest_rssi, last_signal_state)
            : signal_state_from_rssi(strongest_rssi);

        last_signal_state = next;
        has_signal_state  = true;
        return make_result(next);
    }

    // ── Remote mode ───────────────────────────────────────────────────────────

    static bool is_whitelisted(const uint8_t mac[6]) {
        static const uint8_t zero[6] = {0, 0, 0, 0, 0, 0};
        // Empty whitelist (all zeros) means "accept any sender".
        if (memcmp(config::remote_whitelist_mac, zero, 6) == 0) return true;
        return helpers::is_same_mac(mac, config::remote_whitelist_mac);
    }

    CollisionResult calculate_remote_risk(const CyclistsData& cyclists_data) {
        closest_distance_m    = -1.0;
        closest_cyclist_index = -1;

        cyclist_store::remove_expired_cyclists();

        State best_state = State::Safe;
        int   best_rank  = -1;

        for (uint8_t i = 0; i < cyclists_data.slot_count; i++) {
            const CyclistData& cyclist = cyclists_data.cyclists[i];
            if (!cyclist.is_active)           continue;
            if (!cyclist.has_cyclist_state)   continue;
            if (!is_whitelisted(cyclist.mac)) continue;

            const int rank = state_rank(cyclist.cyclist_state);
            if (rank > best_rank) {
                best_state            = cyclist.cyclist_state;
                best_rank             = rank;
                closest_cyclist_index = i;
            }
        }

        return make_result(best_state);
    }

    // ── Getters ───────────────────────────────────────────────────────────────

    double get_closest_distance_m()    { return closest_distance_m; }
    int    get_closest_cyclist_index() { return closest_cyclist_index; }

    double get_closest_closing_speed_mps() {
        if (closest_cyclist_index < 0) return 0.0;
        return slot_trackings[closest_cyclist_index].closing_speed_mps;
    }

    bool is_closest_approaching() {
        if (closest_cyclist_index < 0) return false;
        return slot_trackings[closest_cyclist_index].is_approaching;
    }
}