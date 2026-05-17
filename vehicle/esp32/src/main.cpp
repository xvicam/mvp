#include <Arduino.h>

#include "config.h"
#include "debug_log.h"
#include "esp_now_receiver.h"
#include "output_controller.h"
#include "risk_calculator.h"
#include "serial_control.h"
#include "state_controller.h"
#include "types.h"
#include "vehicle_reader.h"
#include "cyclist_store.h"

void setup() {
    Serial.begin(config::serial_baud);
    delay(300);

    output_controller::init_output();
    output_controller::apply_output(State::GpsWait);

    vehicle_reader::init_gps();

    if (!esp_now_receiver::init_esp_now_receiver()) {
        Serial.print("ESP-NOW init failed. Manual mode still available.");

        state_controller::force_manual_off();
        output_controller::apply_output(state_controller::get_current_state());

        return;
    }

    Serial.println("VICAM vehicle receiver started.");
    Serial.println("Press M to switch REAL / SIGNAL / MANUAL (demo) mode.");
    Serial.println("Manual mode: 0=OFF/ON, 1=IDLE, 2=ALERT, 3=WARNING, 4=URGENT");
}

void loop() {
    vehicle_reader::update_gps();

    serial_control::process_serial_input();
    esp_now_receiver::process_pending_packet();

    if (state_controller::get_current_mode() == Mode::Real) {
        const VehicleData vehicle_data = vehicle_reader::get_vehicle_data();
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();

        const CollisionResult collision_risk =
            risk_calculator::calculate_collision_risk(vehicle_data, cyclists_data);

        state_controller::update_state(collision_risk.state);
    } else if (state_controller::get_current_mode() == Mode::Signal) {
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult collision_risk =
            risk_calculator::calculate_signal_risk(cyclists_data);

        state_controller::update_state(collision_risk.state);
    } else {
        state_controller::update_state(state_controller::get_manual_state());
    }

    output_controller::apply_output(state_controller::get_current_state());
    debug_log::print_debug();
}