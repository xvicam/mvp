#pragma once

#include <Arduino.h>

#include "config.h"

enum class Mode : uint8_t { Real, Manual };

enum class State : uint8_t {
    Off,
    GpsWait,
    Idle,
    Moving,
    Alert,
    Warning,
    Urgent
};

struct CyclistData {
    bool is_active = false;
    uint8_t mac[6] = {0};

    double lat = 0.0;
    double lng = 0.0;
    float speed_kmph = 0.0;

    uint32_t last_seen_ms = 0;
};

struct VehicleData {
    bool is_gps_valid = false;
    bool is_moving = false;

    double lat = 0.0;
    double lng = 0.0;
    float speed_kmph = 0.0;
};

struct CyclistsData {
    const CyclistData* cyclists = nullptr;
    uint8_t slot_count = config::max_cyclists;
};

struct CollisionResult {
    State state = State::GpsWait;
    double closest_distance_m = -1.0;
    int closest_cyclist_index = -1;
};