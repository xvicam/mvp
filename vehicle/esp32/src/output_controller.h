#pragma once

#include <Arduino.h>

#include "types.h"

namespace output_controller {
    void init_output();

    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_motor(uint8_t duty);

    void apply_output(State state);
}