#pragma once

#include "types.h"

#include <Arduino.h>

namespace state_controller {
    Mode get_current_mode();
    State get_current_state();
    State get_manual_state();

    void update_state(State state);

    void toggle_mode();
    void handle_manual_command(char command);

    void force_manual_off();
}