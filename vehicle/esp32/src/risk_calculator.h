#pragma once

#include "types.h"

namespace risk_calculator {
    // Real mode: GPS proximity + approach detection
    CollisionResult calculate_collision_risk(
        const VehicleData& vehicle_data,
        const CyclistsData& cyclists_data
    );

    // Demo mode: RSSI-based proximity
    CollisionResult calculate_signal_risk(const CyclistsData& cyclists_data);

    // Remote mode: cyclist sends state directly; returns highest severity
    CollisionResult calculate_remote_risk(const CyclistsData& cyclists_data);

    double get_closest_distance_m();
    int    get_closest_cyclist_index();
    double get_closest_closing_speed_mps();
    bool   is_closest_approaching();
}