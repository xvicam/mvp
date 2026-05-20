#include "output_controller.h"
#include "config.h"

namespace output_controller {

    void init_output() {
        ledcAttach(config::collision_red_pin,   config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::collision_green_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::collision_blue_pin,  config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::motor_pin,           config::pwm_freq_hz, config::pwm_res);

        ledcAttach(config::sys_mode_red_pin,   config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::sys_mode_green_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::sys_mode_blue_pin,  config::pwm_freq_hz, config::pwm_res);

        ledcAttach(config::vib_mode_red_pin,   config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::vib_mode_green_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::vib_mode_blue_pin,  config::pwm_freq_hz, config::pwm_res);

        pinMode(config::gps_led_pin, OUTPUT);
        digitalWrite(config::gps_led_pin, LOW);
    }

    void set_collision_rgb(uint8_t r, uint8_t g, uint8_t b) {
        ledcWrite(config::collision_red_pin,   r);
        ledcWrite(config::collision_green_pin, g);
        ledcWrite(config::collision_blue_pin,  b);
    }

    void set_collision_rgb(const config::Rgb& rgb) {
        set_collision_rgb(rgb.r, rgb.g, rgb.b);
    }

    void set_motor(uint8_t duty) {
        ledcWrite(config::motor_pin, duty);
    }

    // State  → tier:  Alert = low,  Warning = mid,  Danger = full
    // VibMode → style: Strength = constant duty, Pulse = full duty + rate,
    //                  Pattern  = full duty + pattern speed
    uint8_t compute_motor_duty(State state, VibMode vib_mode) {
        if (state == State::Safe) return 0;

        uint32_t t = millis();

        switch (vib_mode) {

            // ── Strength: constant duty, level set by state ───────────────
            case VibMode::Strength:
                switch (state) {
                    case State::Alert:   return config::motor_strength_low;
                    case State::Warning: return config::motor_strength_mid;
                    case State::Danger:  return config::motor_strength_high;
                    default:             return 0;
                }

            // ── Pulse: always full duty, rate set by state ────────────────
            case VibMode::Pulse:
                switch (state) {
                    case State::Alert:
                        // Slow pulses — 200 ms on / 1000 ms off
                        return ((t % 1200) < 200) ? 255 : 0;
                    case State::Warning:
                        // Mid pulses — 400 ms on / 400 ms off
                        return ((t % 800) < 400) ? 255 : 0;
                    case State::Danger:
                        // Full on — no pulsing
                        return 255;
                    default: return 0;
                }

            // ── Pattern: always full duty, speed set by state ─────────────
            case VibMode::Pattern:
                switch (state) {
                    case State::Alert: {
                        // Slow double-tap (2000 ms cycle)
                        uint32_t c = t % 2000;
                        if (c < 150) return 255;
                        if (c < 300) return 0;
                        if (c < 450) return 255;
                        return 0;
                    }
                    case State::Warning: {
                        // Medium double-tap (1000 ms cycle)
                        uint32_t c = t % 1000;
                        if (c < 150) return 255;
                        if (c < 300) return 0;
                        if (c < 450) return 255;
                        return 0;
                    }
                    case State::Danger: {
                        // Rapid triple-tap (800 ms cycle)
                        uint32_t c = t % 800;
                        if (c < 100) return 255;
                        if (c < 200) return 0;
                        if (c < 300) return 255;
                        if (c < 400) return 0;
                        if (c < 500) return 255;
                        return 0;
                    }
                    default: return 0;
                }
        }

        return 0;
    }

    void apply_output(State state, VibMode vib_mode) {
        switch (state) {
            case State::Safe:
                set_collision_rgb(config::rgb_safe);
                set_motor(0);
                break;
            case State::Alert:
                set_collision_rgb(config::rgb_alert);
                set_motor(compute_motor_duty(state, vib_mode));
                break;
            case State::Warning:
                set_collision_rgb(config::rgb_warning);
                set_motor(compute_motor_duty(state, vib_mode));
                break;
            case State::Danger:
                set_collision_rgb(config::rgb_danger);
                set_motor(compute_motor_duty(state, vib_mode));
                break;
        }
    }

    void apply_sys_mode_led(Mode mode) {
        config::Rgb rgb;
        switch (mode) {
            case Mode::GPS:    rgb = config::sys_mode_gps;    break;
            case Mode::RSSI:   rgb = config::sys_mode_rssi;   break;
            case Mode::Remote: rgb = config::sys_mode_remote; break;
            case Mode::Local:  rgb = config::sys_mode_local;  break;
            default:           rgb = {0, 0, 0};               break;
        }
        ledcWrite(config::sys_mode_red_pin,   rgb.r);
        ledcWrite(config::sys_mode_green_pin, rgb.g);
        ledcWrite(config::sys_mode_blue_pin,  rgb.b);
    }

    void apply_vib_mode_led(VibMode vib_mode) {
        config::Rgb rgb;
        switch (vib_mode) {
            case VibMode::Strength: rgb = config::vib_mode_strength; break;
            case VibMode::Pulse:    rgb = config::vib_mode_pulse;    break;
            case VibMode::Pattern:  rgb = config::vib_mode_pattern;  break;
            default:                rgb = {0, 0, 0};                 break;
        }
        ledcWrite(config::vib_mode_red_pin,   rgb.r);
        ledcWrite(config::vib_mode_green_pin, rgb.g);
        ledcWrite(config::vib_mode_blue_pin,  rgb.b);
    }

    void update_gps_led(bool show, bool valid) {
        if (!show) {
            digitalWrite(config::gps_led_pin, LOW);
            return;
        }
        if (valid) {
            digitalWrite(config::gps_led_pin, HIGH);
            return;
        }
        static uint32_t last_blink_ms = 0;
        static bool     blink_state   = false;
        uint32_t now = millis();
        if (now - last_blink_ms >= config::gps_led_blink_ms) {
            last_blink_ms = now;
            blink_state   = !blink_state;
            digitalWrite(config::gps_led_pin, blink_state ? HIGH : LOW);
        }
    }
}