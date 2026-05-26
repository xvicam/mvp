#pragma once

#include <Arduino.h>
#include "config.h"

enum class Mode    : uint8_t { GPS, RSSI, Remote, Local };
enum class VibMode : uint8_t { Strength, Pulse, Pattern };
enum class State   : uint8_t { Safe, Alert, Warning, Danger };

struct CyclistData {
    bool     is_active         = false;
    uint8_t  mac[6]            = {0};
    uint32_t last_seen_ms      = 0;

    // GPS data — populated by "gps" packets
    bool     has_gps_data      = false;
    double   lat               = 0.0;
    double   lng               = 0.0;
    float    speed_kmph        = 0.0f;

    // RSSI data — populated by the receive layer on any packet
    bool     has_rssi          = false;
    int8_t   rssi_dbm          = 0;
    float    rssi_smoothed_dbm = 0.0f;

    // Remote-mode collision state — populated by "remote" packets
    bool     has_cyclist_state = false;
    State    cyclist_state     = State::Safe;
};

struct VehicleData {
    bool   is_gps_valid = false;
    bool   is_moving    = false;
    double lat          = 0.0;
    double lng          = 0.0;
    float  speed_kmph   = 0.0f;
};

struct CyclistsData {
    const CyclistData* cyclists   = nullptr;
    uint8_t            slot_count = config::max_cyclists;
};

struct CollisionResult {
    State  state                 = State::Safe;
    double closest_distance_m    = -1.0;
    int    closest_cyclist_index = -1;
};