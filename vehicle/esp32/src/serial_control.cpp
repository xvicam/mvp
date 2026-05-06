#include "serial_control.h"
#include "state_controller.h"
#include "vehicle_reader.h"
#include "types.h"

#include <Arduino.h>

namespace serial_control {
    void process_serial_input() {
        while (Serial.available()) {
            char c = Serial.read();

            if (c == '\n' || c == '\r' || c == ' ') {
                continue;
            }

            if (c == 'M' || c == 'm') {
                state_controller::toggle_mode();
                continue;
            }

            if (c == 'G' || c == 'g') {
                vehicle_reader::print_gps_data();
                continue;
            }

            if (state_controller::get_current_mode() == Mode::Manual) {
                state_controller::handle_manual_command(c);
            } else {
                Serial.println("Ignored command. Press M to enter MANUAL mode.");
            }
        }
    }
}