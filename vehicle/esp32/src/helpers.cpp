#include "helpers.h"

#include <string.h>

namespace helpers {
    const char* mode_name(Mode mode) {
        switch (mode) {
            case Mode::Real: return "REAL";
            case Mode::Signal: return "SIGNAL";
            case Mode::Manual: return "MANUAL";
            default: return "UNKNOWN";
        }
    }

    const char* state_name(State state) {
        switch (state) {
            case State::Off: return "OFF";
            case State::GpsWait: return "GPS_WAIT";
            case State::Idle: return "IDLE";
            case State::Moving: return "MOVING";
            case State::Alert: return "ALERT";
            case State::Warning: return "WARNING";
            case State::Urgent: return "URGENT";
            default: return "UNKNOWN";
        }
    }

    bool is_same_mac(const uint8_t a[6], const uint8_t b[6]) {
        return memcmp(a, b, 6) == 0;
    }

    void print_mac(const uint8_t mac[6]) {
        char buffer[18];

        snprintf(
            buffer,
            sizeof(buffer),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0],
            mac[1],
            mac[2],
            mac[3],
            mac[4],
            mac[5]
        );

        Serial.print(buffer);
    }

    bool is_valid_coordinate(double lat, double lng) {
        return lat >= -90.0 && lat <= 90.0 && lng >= -180.0 && lng <= 180.0;
    }
}