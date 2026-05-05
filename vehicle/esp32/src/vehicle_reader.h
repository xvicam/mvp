#pragma once

#include <Arduino.h>

#include "types.h"

namespace vehicle_reader {
    void init_gps();
    void update_gps();

    bool is_vehicle_gps_valid();
    float get_vehicle_speed_kmph();

    void update_vehicle_moving(float speed_kmph);
    bool is_vehicle_moving();

    VehicleData get_vehicle_data();

    bool get_utc_time(char* buffer, size_t buffer_size);
}