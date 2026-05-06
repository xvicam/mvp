#include "state_controller.h"
#include "helpers.h"

namespace state_controller {
    Mode current_mode = Mode::Real;
    State current_state = State::GpsWait;
    State manual_state = State::Off;

    bool is_manual_power_on = false;

    Mode get_current_mode() {
        return current_mode;
    }

    State get_current_state() {
        return current_state;
    }

    State get_manual_state() {
        return manual_state;
    }

    void update_state(State state) {
        current_state = state;
    }

    void toggle_mode() {
        if (current_mode == Mode::Real) {
            current_mode = Mode::Manual;
            current_state = manual_state;
        } else {
            current_mode = Mode::Real;
        }

        Serial.print("Mode switched to: ");
        Serial.println(helpers::mode_name(current_mode));
    }

    void handle_manual_command(char command) {
        switch (command) {
            case '0':
                is_manual_power_on = !is_manual_power_on;
                manual_state = is_manual_power_on ? State::Idle : State::Off;
                break;
            case '1':
                is_manual_power_on = true;
                manual_state = State::Idle;
                break;
            case '2':
                is_manual_power_on = true;
                manual_state = State::Alert;
                break;
            case '3':
                is_manual_power_on = true;
                manual_state = State::Warning;
                break;
            case '4':
                is_manual_power_on = true;
                manual_state = State::Urgent;
                break;
            default:
                return;
        }

        current_state = manual_state;

        Serial.print("Manual state: ");
        Serial.println(helpers::state_name(manual_state));
    }

    void force_manual_off() {
        current_mode = Mode::Manual;
        manual_state = State::Off;
        current_state = State::Off;
        is_manual_power_on = false;
    }
}