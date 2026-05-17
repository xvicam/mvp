#pragma once

#include <stdint.h>
#include <stddef.h>

namespace config {
    constexpr uint8_t red_pin = 2;
    constexpr uint8_t green_pin = 13;
    constexpr uint8_t blue_pin = 14;
    constexpr uint8_t motor_pin = 25;
    constexpr uint8_t gps_tx_pin = 17; //blue
    constexpr uint8_t gps_rx_pin = 16; //yellow

    constexpr uint32_t serial_baud = 115200;
    constexpr uint32_t gps_baud = 9600;

    constexpr uint32_t pwm_freq_hz = 5000;
    constexpr uint8_t pwm_res = 8;

    constexpr uint32_t gps_max_age_ms = 3000;
    constexpr uint32_t cyclist_timeout_ms = 3000;
    constexpr uint32_t debug_interval_ms = 500;

    constexpr float moving_on_kmph = 3.0;
    constexpr float moving_off_kmph = 1.0;

    constexpr double alert_distance_m = 30.0;
    constexpr double warning_distance_m = 15;
    constexpr double urgent_distance_m = 8.0;

    constexpr double rssi_ref_dbm = -59.0;
    constexpr double rssi_path_loss = 2.2;

    constexpr int8_t signal_rssi_urgent_dbm = -50;
    constexpr int8_t signal_rssi_warning_dbm = -60;
    constexpr int8_t signal_rssi_alert_dbm = -65;
    constexpr int8_t signal_rssi_idle_dbm = -70;
    constexpr float signal_rssi_hysteresis_db = 2.0f;
    constexpr float rssi_smoothing_alpha = 0.3f;

    constexpr float max_reasonable_speed_kmph = 80.0;

    constexpr uint8_t max_cyclists = 12;
    constexpr size_t max_packet_size = 250;
}