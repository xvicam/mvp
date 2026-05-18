#include <Arduino.h>

#include "config.h"
#include "cyclist_store.h"
#include "debug_log.h"
#include "esp_now_receiver.h"
#include "input_controller.h"
#include "output_controller.h"
#include "risk_calculator.h"
#include "serial_control.h"
#include "state_controller.h"
#include "types.h"
#include "vehicle_reader.h"

void setup() {
    Serial.begin(config::serial_baud);
    delay(300);

    input_controller::init_inputs();
    output_controller::init_output();
    output_controller::apply_output(State::Safe);
    output_controller::apply_sys_mode_led(state_controller::get_current_mode());
    output_controller::apply_vib_mode_led(state_controller::get_current_vib_mode());
    output_controller::update_gps_led(false, false);

    vehicle_reader::init_gps();

    if (!esp_now_receiver::init_esp_now_receiver()) {
        Serial.println("ESP-NOW init failed. Falling back to Local mode.");
        state_controller::force_local_mode();
        output_controller::apply_output(
            state_controller::get_current_state(),
            state_controller::get_current_vib_mode()
        );
        return;
    }

    Serial.println("VICAM started.");
    Serial.println("Mode btn: click=vib mode | hold=system mode");
    Serial.println("Modes: GPS / RSSI / REMOTE / LOCAL");
    Serial.println("Local btn: click=collision state (LOCAL only)");
    Serial.println("Serial: M=mode, G=GPS dump | Local: 0=SAFE 1=ALERT 2=WARNING 3=DANGER");
}

void loop() {
    const Mode mode = state_controller::get_current_mode();

    // Always process buttons and serial
    input_controller::process_inputs();
    serial_control::process_serial_input();

    // ── GPS mode ─────────────────────────────────────────────────────────────
    // Vehicle GPS + cyclist GPS packets. All other inputs ignored.
    if (mode == Mode::GPS) {
        vehicle_reader::update_gps();
        esp_now_receiver::process_pending_packet();

        const VehicleData  vehicle_data  = vehicle_reader::get_vehicle_data();
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_collision_risk(vehicle_data, cyclists_data);
        state_controller::update_state(result.state);
    }

    // ── RSSI mode ─────────────────────────────────────────────────────────────
    // Cyclist RSSI packets only. GPS disabled on both sides.
    else if (mode == Mode::RSSI) {
        esp_now_receiver::process_pending_packet();

        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_signal_risk(cyclists_data);
        state_controller::update_state(result.state);
    }

    // ── Remote mode ───────────────────────────────────────────────────────────
    // Cyclist sends collision state directly. No calculations. Whitelist enforced.
    else if (mode == Mode::Remote) {
        esp_now_receiver::process_pending_packet();

        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_remote_risk(cyclists_data);
        state_controller::update_state(result.state);
    }

    // ── Local mode ────────────────────────────────────────────────────────────
    // Everything disabled. Vehicle button controls state locally.
    else {
        state_controller::update_state(state_controller::get_manual_state());
    }

    // ── Outputs ───────────────────────────────────────────────────────────────
    const State   current_state = state_controller::get_current_state();
    const VibMode vib_mode      = state_controller::get_current_vib_mode();

    output_controller::apply_output(current_state, vib_mode);
    output_controller::apply_sys_mode_led(mode);
    output_controller::apply_vib_mode_led(vib_mode);
    output_controller::update_gps_led(
        mode == Mode::GPS,
        vehicle_reader::is_vehicle_gps_valid()
    );

    debug_log::print_debug();
}