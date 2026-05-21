#include "debug_log.h"
#include "config.h"
#include "cyclist_store.h"
#include "helpers.h"
#include "risk_calculator.h"
#include "state_controller.h"
#include "vehicle_reader.h"

#include <Arduino.h>

namespace debug_log {

    // ── Closest-cyclist field ──────────────────────────────────────────────
    //
    // GPS / RSSI modes:  "12.34 m from AA:BB:CC:DD:EE:FF"
    // Remote mode:       "AA:BB:CC:DD:EE:FF"  (no distance in remote mode)
    // No cyclist:        "none"
    //
    // Two bugs fixed from original:
    //  1. Distance was cast to int — 2.7 m would print as "2 m".
    //  2. Remote mode always printed "none" because distance is -1 and
    //     the guard required distance >= 0 before printing anything.

    static void print_closest() {
        const int    index    = risk_calculator::get_closest_cyclist_index();
        const double distance = risk_calculator::get_closest_distance_m();

        const CyclistData* cyclist = cyclist_store::get_cyclist_at(index);

        if (!cyclist) {
            Serial.print("none");
            return;
        }

        if (distance >= 0.0) {
            Serial.print(distance);
            Serial.print(" m from ");
        }
        helpers::print_mac(cyclist->mac);
    }

    // ── Public API ─────────────────────────────────────────────────────────

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
        print_closest();

        char time_buffer[12];
        if (vehicle_reader::get_utc_time(time_buffer, sizeof(time_buffer))) {
            Serial.print(" | UTC: ");
            Serial.print(time_buffer);
        }

        Serial.println();
    }
}