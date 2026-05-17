#include "risk_calculator.h"
#include "config.h"
#include "cyclist_store.h"

#include <TinyGPS++.h>
#include <math.h>

namespace risk_calculator {
    double closest_distance_m = -1.0;
    int closest_cyclist_index = -1;
    State last_signal_state = State::Idle;
    bool has_signal_state = false;

    CollisionResult make_result(State state) {
        CollisionResult result;
        result.state = state;
        result.closest_distance_m = closest_distance_m;
        result.closest_cyclist_index = closest_cyclist_index;

        return result;
    }

    double estimate_distance_from_rssi_dbm(float rssi_dbm) {
        if (rssi_dbm >= 0.0f) {
            return -1.0;
        }

        double exponent =
            (config::rssi_ref_dbm - static_cast<double>(rssi_dbm)) /
            (10.0 * config::rssi_path_loss);

        return pow(10.0, exponent);
    }

    State signal_state_from_rssi(float rssi_dbm) {
        if (rssi_dbm >= config::signal_rssi_urgent_dbm) {
            return State::Urgent;
        }

        if (rssi_dbm >= config::signal_rssi_warning_dbm) {
            return State::Warning;
        }

        if (rssi_dbm >= config::signal_rssi_alert_dbm) {
            return State::Alert;
        }

        if (rssi_dbm >= config::signal_rssi_idle_dbm) {
            return State::Idle;
        }

        return State::Idle;
    }

    int signal_state_rank(State state) {
        switch (state) {
            case State::Urgent: return 3;
            case State::Warning: return 2;
            case State::Alert: return 1;
            case State::Idle: return 0;
            default: return -1;
        }
    }

    float signal_state_threshold(State state) {
        switch (state) {
            case State::Urgent: return static_cast<float>(config::signal_rssi_urgent_dbm);
            case State::Warning: return static_cast<float>(config::signal_rssi_warning_dbm);
            case State::Alert: return static_cast<float>(config::signal_rssi_alert_dbm);
            case State::Idle: return static_cast<float>(config::signal_rssi_idle_dbm);
            default: return static_cast<float>(config::signal_rssi_idle_dbm);
        }
    }

    State apply_signal_hysteresis(float rssi_dbm, State previous_state) {
        State raw_state = signal_state_from_rssi(rssi_dbm);

        if (signal_state_rank(raw_state) >= signal_state_rank(previous_state)) {
            return raw_state;
        }

        float threshold = signal_state_threshold(previous_state);

        if (rssi_dbm < (threshold - config::signal_rssi_hysteresis_db)) {
            return raw_state;
        }

        return previous_state;
    }

    CollisionResult calculate_collision_risk(
        const VehicleData& vehicle_data,
        const CyclistsData& cyclists_data
    ) {
       closest_distance_m = -1.0;
       closest_cyclist_index = -1;

       if (!vehicle_data.is_gps_valid) {
        return make_result(State::GpsWait);
       }

       cyclist_store::remove_expired_cyclists();

       for (uint8_t i = 0; i < cyclists_data.slot_count; i++) {
        const CyclistData& cyclist = cyclists_data.cyclists[i];

        if (!cyclist.is_active) {
            continue;
        }

        double distance_m = TinyGPSPlus::distanceBetween(
            vehicle_data.lat,
            vehicle_data.lng,
            cyclist.lat,
            cyclist.lng
        );

        if (closest_distance_m < 0 || distance_m < closest_distance_m) {
            closest_distance_m = distance_m;
            closest_cyclist_index = i;
        }
       }

       if (closest_cyclist_index < 0) {
        return make_result(vehicle_data.is_moving ? State::Moving : State::Idle);
       }

       const CyclistData& closest_cyclist = cyclists_data.cyclists[closest_cyclist_index];

       bool is_cyclist_moving = closest_cyclist.speed_kmph >= config::moving_on_kmph;
       bool has_relevant_motion = vehicle_data.is_moving || is_cyclist_moving;

       if (!has_relevant_motion) {
        return make_result(State::Idle);
       }

       if (closest_distance_m <= config::urgent_distance_m) {
        return make_result(State::Urgent);
       }

       if (closest_distance_m <= config::warning_distance_m) {
        return make_result(State::Warning);
       }

       if (closest_distance_m <= config::alert_distance_m) {
        return make_result(State::Alert);
       }

       return make_result(vehicle_data.is_moving ? State::Moving : State::Idle);
    }

    CollisionResult calculate_signal_risk(const CyclistsData& cyclists_data) {
        closest_distance_m = -1.0;
        closest_cyclist_index = -1;

        cyclist_store::remove_expired_cyclists();

        float strongest_rssi_dbm = -127.0f;

        for (uint8_t i = 0; i < cyclists_data.slot_count; i++) {
            const CyclistData& cyclist = cyclists_data.cyclists[i];

            if (!cyclist.is_active || !cyclist.has_rssi) {
                continue;
            }

            float rssi_dbm = cyclist.rssi_smoothed_dbm;
            double distance_m = estimate_distance_from_rssi_dbm(rssi_dbm);

            if (distance_m < 0) {
                continue;
            }

            if (closest_distance_m < 0 || distance_m < closest_distance_m) {
                closest_distance_m = distance_m;
                closest_cyclist_index = i;
            }

            if (rssi_dbm > strongest_rssi_dbm) {
                strongest_rssi_dbm = rssi_dbm;
            }
        }

        if (closest_cyclist_index < 0) {
            last_signal_state = State::Idle;
            has_signal_state = true;
            return make_result(State::Idle);
        }

        State raw_state = signal_state_from_rssi(strongest_rssi_dbm);
        State next_state = raw_state;

        if (has_signal_state && signal_state_rank(last_signal_state) >= 0) {
            next_state = apply_signal_hysteresis(strongest_rssi_dbm, last_signal_state);
        }

        last_signal_state = next_state;
        has_signal_state = true;

        return make_result(next_state);
    }

    double get_closest_distance_m() {
        return closest_distance_m;
    }

    int get_closest_cyclist_index() {
        return closest_cyclist_index;
    }
}