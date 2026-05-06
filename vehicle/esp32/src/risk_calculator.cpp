#include "risk_calculator.h"
#include "config.h"
#include "cyclist_store.h"

#include <TinyGPS++.h>

namespace risk_calculator {
    double closest_distance_m = -1.0;
    int closest_cyclist_index = -1;

    CollisionResult make_result(State state) {
        CollisionResult result;
        result.state = state;
        result.closest_distance_m = closest_distance_m;
        result.closest_cyclist_index = closest_cyclist_index;

        return result;
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

    double get_closest_distance_m() {
        return closest_distance_m;
    }

    int get_closest_cyclist_index() {
        return closest_cyclist_index;
    }
}