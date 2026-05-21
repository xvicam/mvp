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

// ── Deep sleep ────────────────────────────────────────────────────────────────

// ── Deep sleep ────────────────────────────────────────────────────────────────

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
            stable_since = millis();
        }
        delay(5);
    }
}

static void enter_deep_sleep() {
    Serial.println("Entering deep sleep.");
    Serial.flush();


    // Turn everything off immediately (visual feedback on press)
    esp_now_receiver::deinit_esp_now_receiver();


    output_controller::set_power_led(false);
    ledcWrite(config::motor_pin,           0);
    ledcWrite(config::collision_red_pin,   0);
    ledcWrite(config::collision_green_pin, 0);
    ledcWrite(config::collision_blue_pin,  0);
    ledcWrite(config::sys_mode_red_pin,    0);
    ledcWrite(config::sys_mode_green_pin,  0);
    ledcWrite(config::sys_mode_blue_pin,   0);
    ledcWrite(config::vib_mode_red_pin,    0);
    ledcWrite(config::vib_mode_green_pin,  0);
    ledcWrite(config::vib_mode_blue_pin,   0);
    digitalWrite(config::gps_led_pin, LOW);

    uint32_t stable_since = millis();

    while (true) {
        if (digitalRead(config::pow_btn_pin) == HIGH) {
            if (millis() - stable_since >= 100) break;
        } else {
            stable_since = millis(); // reset on bounce
        }
        delay(5);
    }


    gpio_hold_en((gpio_num_t)config::pow_led_pin);

    esp_sleep_enable_ext0_wakeup((gpio_num_t)config::pow_btn_pin, 0);

    esp_deep_sleep_start();
    // Never returns — ESP32 resets on wake, setup() runs again
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(config::serial_baud);
    delay(300);

    pinMode(config::pow_btn_pin, INPUT); // external pull-up 10k required

    const auto wake_cause = esp_sleep_get_wakeup_cause();
    if (wake_cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke from deep sleep via power button.");
        // Button is still pressed (it triggered EXT0) — wait for release
        // before entering loop, otherwise the press detector fires
        // immediately and puts the device straight back to sleep.
        while (digitalRead(config::pow_btn_pin) == LOW) {
            delay(5);
        }
        delay(100); // debounce
    } else {
        Serial.println("Fresh boot.");
    }

    // DO NOT block here waiting for button release.
    // The 2-second grace period in loop() handles the button-still-held case.

    input_controller::init_inputs();
    output_controller::init_output();   // calls init_power_led() → releases hold, LED ON
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
    Serial.println("Power btn: click = sleep | Mode btn: click=vib, hold=system mode");
    Serial.println("Modes: GPS / RSSI / REMOTE / LOCAL");
    Serial.println("Local btn: click = collision state (LOCAL only)");
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
    const Mode mode = state_controller::get_current_mode();

    // ── Power button (falling edge = press to sleep) ──────────────────────────
    {
        static bool booted   = false;
        static uint32_t boot_ms  = 0;
        static bool pow_prev = false;

        if (!booted) { boot_ms = millis(); booted = true; }

        const bool pow_now = is_pow_btn_pressed();

        if (pow_now && !pow_prev) {
            // Falling edge: button just pressed
            // Grace period stops an immediate re-sleep right after a wake press
            if ((millis() - boot_ms) >= config::pow_btn_wake_grace_ms) {
                enter_deep_sleep();
                // enter_deep_sleep() waits for release internally,
                // so we never reach the line below until next boot
            }
        }

        pow_prev = pow_now;
    }

    // ── Operational buttons ───────────────────────────────────────────────────
    input_controller::process_inputs();
    serial_control::process_serial_input();

    // ── ESP-NOW ───────────────────────────────────────────────────────────────
    if (mode == Mode::GPS || mode == Mode::RSSI || mode == Mode::Remote) {
        esp_now_receiver::process_pending_packet();
    }

    // ── Risk calculation ──────────────────────────────────────────────────────
    if (mode == Mode::GPS) {
        vehicle_reader::update_gps();
        const VehicleData  vehicle_data  = vehicle_reader::get_vehicle_data();
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_collision_risk(vehicle_data, cyclists_data);
        state_controller::update_state(result.state);

    } else if (mode == Mode::RSSI) {
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_signal_risk(cyclists_data);
        state_controller::update_state(result.state);

    } else if (mode == Mode::Remote) {
        const CyclistsData cyclists_data = cyclist_store::get_cyclists_data();
        const CollisionResult result =
            risk_calculator::calculate_remote_risk(cyclists_data);
        state_controller::update_state(result.state);

    } else {
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