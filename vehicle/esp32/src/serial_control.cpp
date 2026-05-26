#include "serial_control.h"
#include "cyclist_store.h"
#include "helpers.h"
#include "risk_calculator.h"
#include "state_controller.h"
#include "vehicle_reader.h"
#include "types.h"

#include <Arduino.h>

namespace serial_control {

    // ── Help ─────────────────────────────────────────────────────────────────

    static void print_help() {
        Serial.println();
        Serial.println("========== COMMANDS ==========");
        Serial.println("  m / M   Cycle system mode (GPS → RSSI → REMOTE → LOCAL)");
        Serial.println("  g / G   Print full GPS diagnostic data");
        Serial.println("  c / C   Print active cyclist list");
        Serial.println("  h / ?   Print this help");
        Serial.println("  --- LOCAL mode only ---");
        Serial.println("  s / S   Cycle collision state");
        Serial.println("  0       Set state: Safe");
        Serial.println("  1       Set state: Alert");
        Serial.println("  2       Set state: Warning");
        Serial.println("  3       Set state: Danger");
        Serial.println("==============================");
        Serial.println();
    }

    // ── Cyclist list ──────────────────────────────────────────────────────────

    static void print_cyclist_list() {
        const CyclistsData data  = cyclist_store::get_cyclists_data();
        const uint8_t      count = cyclist_store::get_active_cyclist_count();

        Serial.println();
        Serial.print("========== CYCLISTS (");
        Serial.print(count);
        Serial.println(" active) ==========");

        bool any = false;
        for (uint8_t i = 0; i < data.slot_count; i++) {
            const CyclistData& c = data.cyclists[i];
            if (!c.is_active) continue;
            any = true;

            Serial.print("  [");
            Serial.print(i);
            Serial.print("] ");
            helpers::print_mac(c.mac);

            Serial.print("  age=");
            Serial.print(millis() - c.last_seen_ms);
            Serial.print("ms");

            if (c.has_gps_data) {
                Serial.print("  gps=(");
                Serial.print(c.lat, 6);
                Serial.print(", ");
                Serial.print(c.lng, 6);
                Serial.print(")");
                Serial.print("  speed=");
                Serial.print(c.speed_kmph);
                Serial.print("km/h");
            }

            if (c.has_rssi) {
                Serial.print("  rssi=");
                Serial.print(c.rssi_smoothed_dbm);
                Serial.print("dBm");
            }

            if (c.has_cyclist_state) {
                Serial.print("  remote=");
                Serial.print(helpers::state_name(c.cyclist_state));
            }

            Serial.println();
        }

        if (!any) Serial.println("  (none)");
        Serial.println("==========================================");
        Serial.println();
    }

    // ── Command dispatch ──────────────────────────────────────────────────────

    void process_serial_input() {
        while (Serial.available()) {
            char c = Serial.read();

            if (c == '\n' || c == '\r' || c == ' ') {
                continue;
            }

            if (c == 'M' || c == 'm') {
                state_controller::cycle_system_mode();
                continue;
            }

            if (c == 'G' || c == 'g') {
                vehicle_reader::print_gps_data();
                continue;
            }

            if (c == 'C' || c == 'c') {
                print_cyclist_list();
                continue;
            }

            if (c == 'H' || c == 'h' || c == '?') {
                print_help();
                continue;
            }

            if (state_controller::get_current_mode() == Mode::Local) {
                if (c == 'S' || c == 's') {
                    state_controller::cycle_manual_state();
                } else {
                    state_controller::handle_manual_command(c);
                }
            } else {
                Serial.println("Ignored command. Press M to cycle into LOCAL mode.");
            }
        }
    }
}