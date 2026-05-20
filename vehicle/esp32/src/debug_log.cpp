#include "debug_log.h"
#include "config.h"
#include "cyclist_store.h"
#include "helpers.h"
#include "risk_calculator.h"
#include "state_controller.h"
#include "vehicle_reader.h"

#include <Arduino.h>

namespace debug_log {
    void print_debug() {
        if (state_controller::get_current_mode() == Mode::Local) {
            return;
        }

        static uint32_t last_debug_ms = 0;

        if (millis() - last_debug_ms < config::debug_interval_ms) {
            return;
        }

        last_debug_ms = millis();

        Serial.print("Mode: ");
        Serial.print(helpers::mode_name(state_controller::get_current_mode()));

        Serial.print(" | State: ");
        Serial.print(helpers::state_name(state_controller::get_current_state()));

        Serial.print(" | GPS: ");
        Serial.print(vehicle_reader::is_vehicle_gps_valid() ? "valid" : "invalid");

        Serial.print(" | Vehicle speed: ");
        Serial.print(vehicle_reader::get_vehicle_speed_kmph());
        Serial.print(" km/h");

        Serial.print(" | Moving: ");
        Serial.print(vehicle_reader::is_vehicle_moving() ? "yes" : "no");

        Serial.print(" | Cyclists: ");
        Serial.print(cyclist_store::get_active_cyclist_count());

        Serial.print(" | Closest: ");

        int closest_cyclist_index = risk_calculator::get_closest_cyclist_index();
        int closest_distance_m = risk_calculator::get_closest_distance_m();

        const CyclistData* closest_cyclist = cyclist_store::get_cyclist_at(closest_cyclist_index);

        if (closest_cyclist && closest_distance_m >= 0) {
            Serial.print(closest_distance_m);
            Serial.print(" m from ");
            helpers::print_mac(closest_cyclist->mac);
        } else {
            Serial.print("none");
        }

        char time_buffer[12];

        if (vehicle_reader::get_utc_time(time_buffer, sizeof(time_buffer))) {
            Serial.print(" | UTC: ");
            Serial.print(time_buffer);
        }

        Serial.println();

    }
}