#pragma once

#include <stdint.h>
#include <stddef.h>

namespace config {
    // ── Inputs ───────────────────────────────────────────────────────────────
    constexpr uint8_t manual_btn_pin = 26;
    constexpr uint8_t mode_btn_pin   = 15;

    // ── GPS UART (UART2) ─────────────────────────────────────────────────────
    constexpr uint8_t gps_serial_rx_pin = 16;
    constexpr uint8_t gps_serial_tx_pin = 17;

    // ── Internal Outputs ─────────────────────────────────────────────────────
    constexpr uint8_t sys_mode_red_pin   = 18;
    constexpr uint8_t sys_mode_green_pin = 23;
    constexpr uint8_t sys_mode_blue_pin  = 4;
    constexpr uint8_t vib_mode_red_pin   = 19;
    constexpr uint8_t vib_mode_green_pin = 22;
    constexpr uint8_t vib_mode_blue_pin  = 21;
    constexpr uint8_t gps_led_pin        = 12;

    // ── External Outputs (steering wheel cover, MOSFET-driven) ────────────────
    constexpr uint8_t motor_pin           = 25;
    constexpr uint8_t collision_red_pin   = 2;
    constexpr uint8_t collision_green_pin = 13;
    constexpr uint8_t collision_blue_pin  = 14;

    // ── Serial ───────────────────────────────────────────────────────────────
    constexpr uint32_t serial_baud = 115200;

    // ── GPS ──────────────────────────────────────────────────────────────────
    constexpr uint32_t gps_baud       = 9600;
    constexpr uint32_t gps_max_age_ms = 3000;

    // ── PWM ──────────────────────────────────────────────────────────────────
    constexpr uint32_t pwm_freq_hz = 5000;
    constexpr uint8_t  pwm_res     = 8;

    // ── Timing ───────────────────────────────────────────────────────────────
    constexpr uint32_t cyclist_timeout_ms = 3000;
    constexpr uint32_t debug_interval_ms  = 500;
    constexpr uint32_t debounce_ms        = 20;
    constexpr uint32_t long_press_ms      = 600;
    constexpr uint32_t gps_led_blink_ms   = 500;

    // ── Vehicle motion (GPS mode only) ────────────────────────────────────────
    constexpr float moving_on_kmph  = 1.5f;
    constexpr float moving_off_kmph = 0.8f;

    // ── GPS proximity thresholds ─────────────────────────────────────────────
    constexpr double alert_distance_m     = 60.0;
    constexpr double collision_distance_m =  3.0;
    constexpr double ttc_danger_s         =  3.0;

    // ── Approach detection ───────────────────────────────────────────────────
    constexpr double   distance_ema_alpha     = 0.3;
    constexpr uint32_t tracking_interval_ms   = 250;
    constexpr double   approach_on_speed_mps  =  0.5;
    constexpr double   approach_off_speed_mps = -0.2;

    // ── Speed sanity caps ─────────────────────────────────────────────────────
    constexpr float max_reasonable_cyclist_speed_kmph = 80.0f;
    constexpr float max_reasonable_vehicle_speed_kmph = 250.0f;

    // ── RSSI thresholds ───────────────────────────────────────────────────────
    constexpr double  rssi_ref_dbm              = -59.0;
    constexpr double  rssi_path_loss            =   2.2;
    constexpr int8_t  signal_rssi_danger_dbm    = -50;
    constexpr int8_t  signal_rssi_warning_dbm   = -60;
    constexpr int8_t  signal_rssi_alert_dbm     = -65;
    constexpr int8_t  signal_rssi_safe_dbm      = -70;
    constexpr float   signal_rssi_hysteresis_db =  2.0f;
    constexpr float   rssi_smoothing_alpha      =  0.3f;

    // ── Remote mode whitelist ─────────────────────────────────────────────────
    constexpr uint8_t remote_whitelist_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // ── Cyclists ─────────────────────────────────────────────────────────────
    constexpr uint8_t max_cyclists    = 12;
    constexpr size_t  max_packet_size = 250;

    // ── RGB helpers ───────────────────────────────────────────────────────────
    struct Rgb { uint8_t r; uint8_t g; uint8_t b; };

    // Collision state → steering wheel RGB
    constexpr Rgb rgb_safe    = {  0, 255,   0};
    constexpr Rgb rgb_alert   = {255, 255,   0};
    constexpr Rgb rgb_warning = {255, 120,   0};
    constexpr Rgb rgb_danger  = {255,   0,   0};

    // Motor duty — Strength mode tiers (Alert=Low, Warning=Mid, Danger=Full)
    // Pulse and Pattern modes always drive the motor at 255 when active;
    // the state (Alert/Warning/Danger) controls the pulse rate/pattern speed instead.
    constexpr uint8_t motor_strength_low  =  80;
    constexpr uint8_t motor_strength_mid  = 160;
    constexpr uint8_t motor_strength_high = 255;

    // System mode → sys_mode LED
    constexpr Rgb sys_mode_gps    = {  0,   0, 255};
    constexpr Rgb sys_mode_rssi   = {  0, 200, 200};
    constexpr Rgb sys_mode_remote = {255, 140,   0};
    constexpr Rgb sys_mode_local  = {200,   0, 200};

    // Vib mode → vib_mode LED
    constexpr Rgb vib_mode_strength = {  0, 255,   0};  // green
    constexpr Rgb vib_mode_pulse    = {  0, 200, 255};  // sky blue
    constexpr Rgb vib_mode_pattern  = {180,   0, 255};  // purple
}