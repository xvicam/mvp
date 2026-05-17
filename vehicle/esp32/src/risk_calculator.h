#pragma once

#include "types.h"

namespace risk_calculator {
    CollisionResult calculate_collision_risk(
        const VehicleData& vehicle_data,
        const CyclistsData& cyclists_data
    );

    CollisionResult calculate_signal_risk(const CyclistsData& cyclists_data);

    double get_closest_distance_m();
    int get_closest_cyclist_index();
}