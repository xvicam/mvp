#include "state_controller.h"
#include "helpers.h"

namespace state_controller {
    Mode    current_mode     = Mode::GPS;
    State   current_state    = State::Safe;
    State   manual_state     = State::Safe;
    VibMode current_vib_mode = VibMode::Strength;

    Mode    get_current_mode()     { return current_mode; }
    State   get_current_state()    { return current_state; }
    State   get_manual_state()     { return manual_state; }
    VibMode get_current_vib_mode() { return current_vib_mode; }

    void update_state(State state) { current_state = state; }

    void cycle_system_mode() {
        switch (current_mode) {
            case Mode::GPS:    current_mode = Mode::RSSI;   break;
            case Mode::RSSI:   current_mode = Mode::Remote; break;
            case Mode::Remote: current_mode = Mode::Local;
                               current_state = manual_state; break;
            case Mode::Local:
            default:           current_mode = Mode::GPS;    break;
        }
        Serial.print("Mode: ");
        Serial.println(helpers::mode_name(current_mode));
    }

    void cycle_vib_mode() {
        switch (current_vib_mode) {
            case VibMode::Strength: current_vib_mode = VibMode::Pulse;    break;
            case VibMode::Pulse:    current_vib_mode = VibMode::Pattern;  break;
            case VibMode::Pattern:
            default:                current_vib_mode = VibMode::Strength; break;
        }
        Serial.print("Vib mode: ");
        Serial.println(helpers::vib_mode_name(current_vib_mode));
    }

    void cycle_manual_state() {
        if (current_mode != Mode::Local) return;
        switch (manual_state) {
            case State::Safe:    manual_state = State::Alert;   break;
            case State::Alert:   manual_state = State::Warning; break;
            case State::Warning: manual_state = State::Danger;  break;
            case State::Danger:
            default:             manual_state = State::Safe;    break;
        }
        current_state = manual_state;
        Serial.print("Local state: ");
        Serial.println(helpers::state_name(manual_state));
    }

    void toggle_mode() { cycle_system_mode(); }

    void handle_manual_command(char command) {
        switch (command) {
            case '0': manual_state = State::Safe;    break;
            case '1': manual_state = State::Alert;   break;
            case '2': manual_state = State::Warning; break;
            case '3': manual_state = State::Danger;  break;
            default: return;
        }
        current_state = manual_state;
        Serial.print("Local state: ");
        Serial.println(helpers::state_name(manual_state));
    }

    void force_local_mode() {
        current_mode  = Mode::Local;
        manual_state  = State::Safe;
        current_state = State::Safe;
    }
}