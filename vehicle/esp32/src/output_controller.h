#pragma once

#include <Arduino.h>
#include "types.h"

namespace output_controller {
    void init_output();

    void apply_output(State state, VibMode vib_mode = VibMode::Strength);

    void apply_sys_mode_led(Mode mode);
    void apply_vib_mode_led(VibMode vib_mode);
    void update_gps_led(bool show, bool valid);

    void set_collision_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_collision_rgb(const config::Rgb& rgb);
    void set_motor(uint8_t duty);
}