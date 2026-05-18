#pragma once

#include <Arduino.h>
#include "types.h"

namespace helpers {
    const char* mode_name(Mode mode);
    const char* state_name(State state);
    const char* vib_mode_name(VibMode mode);

    bool is_same_mac(const uint8_t a[6], const uint8_t b[6]);
    void print_mac(const uint8_t mac[6]);

    bool is_valid_coordinate(double lat, double lng);
}