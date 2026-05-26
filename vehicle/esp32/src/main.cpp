#include <Arduino.h>
#include <driver/gpio.h>

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

// ── Power button helpers ──────────────────────────────────────────────────────

static bool is_pow_btn_pressed() {
    const int level = digitalRead(config::pow_btn_pin);
    return config::pow_btn_active_low ? (level == LOW) : (level == HIGH);
}

static void wait_for_pow_btn_release(uint32_t stable_ms) {
    uint32_t stable_since = millis();
    while (true) {
        if (!is_pow_btn_pressed()) {
            if (millis() - stable_since >= stable_ms) break;
        } else {
            stable_since = millis();  // reset on bounce or still-held
        }
        delay(5);
    }
}

// ── Deep sleep ────────────────────────────────────────────────────────────────

static void enter_deep_sleep() {
    Serial.println("Entering deep sleep.");
    Serial.flush();

    // Visual feedback: kill all outputs immediately.
    esp_now_receiver::deinit_esp_now_receiver();
    output_controller::shutdown_outputs();

    // Wait for the user to release the button before arming wake — otherwise
    // EXT0 fires the instant we re-enter sleep.
    wait_for_pow_btn_release(config::pow_btn_release_ms);

    // Hold the power LED state across deep sleep.
    gpio_hold_en((gpio_num_t)config::pow_led_pin);

    esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)config::pow_btn_pin,
        config::pow_btn_wakeup_level
    );
    esp_deep_sleep_start();
    // Never returns — ESP32 resets on wake, setup() runs again.
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(config::serial_baud);
    delay(300);

    pinMode(config::pow_btn_pin, INPUT);  // external 10k pull-up required

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke from deep sleep via power button.");
        // Button is still pressed (it triggered EXT0). Wait for release so the
        // press detector in loop() doesn't fire immediately and put us straight
        // back to sleep. The grace period in loop() is a backup.
        wait_for_pow_btn_release(config::pow_btn_release_ms);
    } else {
        Serial.println("Fresh boot.");
    }

    input_controller::init_inputs();
    output_controller::init_output();   // releases pow_led hold, LED on
    output_controller::apply_output(State::Safe);
    output_controller::apply_sys_mode_led(state_controller::get_current_mode());
    output_controller::apply_vib_mode_led(state_controller::get_current_vib_mode());
    output_controller::update_gps_led(false, false);

    vehicle_reader::init_gps();

    if (!esp_now_receiver::init_esp_now_receiver()) {
        Serial.println("ESP-NOW init failed. Falling back to LOCAL mode.");
        state_controller::force_local_mode();
        output_controller::apply_sys_mode_led(state_controller::get_current_mode());
        output_controller::apply_output(
            state_controller::get_current_state(),
            state_controller::get_current_vib_mode()
        );
    }

    Serial.println("VICAM started.");
    Serial.println("Power btn: click = sleep | Mode btn: click=vib, hold=system mode");
    Serial.println("Modes: GPS / RSSI / REMOTE / LOCAL");
    Serial.println("Local btn: click = collision state (LOCAL only)");
}

// ── Loop helpers ──────────────────────────────────────────────────────────────

static void check_power_button() {
    static bool     booted       = false;
    static uint32_t boot_ms      = 0;
    static bool     prev_pressed = false;

    if (!booted) {
        boot_ms = millis();
        booted  = true;
    }

    const bool pressed = is_pow_btn_pressed();

    // Falling edge: button just pressed. The grace window prevents an immediate
    // re-sleep when the button is still held from the wake event.
    if (pressed && !prev_pressed &&
        (millis() - boot_ms) >= config::pow_btn_wake_grace_ms)
    {
        enter_deep_sleep();
        // Never returns.
    }

    prev_pressed = pressed;
}

static State compute_state_for_mode(Mode mode) {
    switch (mode) {
        case Mode::GPS: {
            vehicle_reader::update_gps();
            const VehicleData  vehicle_data  = vehicle_reader::get_vehicle_data();
            const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
            return risk_calculator::calculate_collision_risk(
                vehicle_data, cyclists_data
            ).state;
        }
        case Mode::RSSI: {
            const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
            return risk_calculator::calculate_signal_risk(cyclists_data).state;
        }
        case Mode::Remote: {
            const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
            return risk_calculator::calculate_remote_risk(cyclists_data).state;
        }
        case Mode::Local:
        default:
            return state_controller::get_manual_state();
    }
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
    check_power_button();

    input_controller::process_inputs();
    serial_control::process_serial_input();

    const Mode mode = state_controller::get_current_mode();

    // ESP-NOW packets are only relevant for cyclist-aware modes.
    if (mode != Mode::Local) {
        esp_now_receiver::process_pending_packet();
    }

    state_controller::update_state(compute_state_for_mode(mode));

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