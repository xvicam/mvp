#pragma once

#include "types.h"
#include <Arduino.h>

namespace state_controller {
    Mode    get_current_mode();
    State   get_current_state();
    State   get_manual_state();
    VibMode get_current_vib_mode();

    void update_state(State state);

    void cycle_system_mode();
    void cycle_vib_mode();
    void cycle_manual_state();

    void handle_manual_command(char command);

    void force_local_mode();
}