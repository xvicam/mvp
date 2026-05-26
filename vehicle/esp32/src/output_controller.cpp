#include "output_controller.h"
#include "config.h"
#include <driver/gpio.h>

namespace output_controller {

    // ── Internal helpers ─────────────────────────────────────────────────────

    static void set_rgb_pins(
        uint8_t r_pin, uint8_t g_pin, uint8_t b_pin,
        const config::Rgb& rgb
    ) {
        ledcWrite(r_pin, rgb.r);
        ledcWrite(g_pin, rgb.g);
        ledcWrite(b_pin, rgb.b);
    }

    static config::Rgb rgb_for_state(State state) {
        switch (state) {
            case State::Alert:   return config::rgb_alert;
            case State::Warning: return config::rgb_warning;
            case State::Danger:  return config::rgb_danger;
            case State::Safe:
            default:             return config::rgb_safe;
        }
    }

    static config::Rgb rgb_for_mode(Mode mode) {
        switch (mode) {
            case Mode::GPS:    return config::sys_mode_gps;
            case Mode::RSSI:   return config::sys_mode_rssi;
            case Mode::Remote: return config::sys_mode_remote;
            case Mode::Local:  return config::sys_mode_local;
        }
        return {0, 0, 0};
    }

    static config::Rgb rgb_for_vib_mode(VibMode mode) {
        switch (mode) {
            case VibMode::Strength: return config::vib_mode_strength;
            case VibMode::Pulse:    return config::vib_mode_pulse;
            case VibMode::Pattern:  return config::vib_mode_pattern;
        }
        return {0, 0, 0};
    }

    // State   → tier:  Alert = low, Warning = mid, Danger = full
    // VibMode → style: Strength = constant duty by state
    //                  Pulse    = full duty, rate set by state
    //                  Pattern  = full duty, pattern speed set by state
    static uint8_t compute_motor_duty(State state, VibMode vib_mode) {
        if (state == State::Safe) return 0;

        const uint32_t t = millis();

        switch (vib_mode) {
            // ── Strength: constant duty, level set by state ──────────────────
            case VibMode::Strength:
                switch (state) {
                    case State::Alert:   return config::motor_strength_low;
                    case State::Warning: return config::motor_strength_mid;
                    case State::Danger:  return config::motor_strength_high;
                    default:             return 0;
                }

            // ── Pulse: always full duty, rate set by state ───────────────────
            case VibMode::Pulse:
                switch (state) {
                    case State::Alert:    // 200 ms on / 1000 ms off
                        return ((t % 1200) < 200) ? 255 : 0;
                    case State::Warning:  // 400 ms on / 400 ms off
                        return ((t % 800) < 400) ? 255 : 0;
                    case State::Danger:   // continuous
                        return 255;
                    default: return 0;
                }

            // ── Pattern: always full duty, speed set by state ────────────────
            case VibMode::Pattern:
                switch (state) {
                    case State::Alert: {  // slow double-tap, 2 s cycle
                        const uint32_t c = t % 2000;
                        if (c < 150) return 255;
                        if (c < 300) return 0;
                        if (c < 450) return 255;
                        return 0;
                    }
                    case State::Warning: {  // medium double-tap, 1 s cycle
                        const uint32_t c = t % 1000;
                        if (c < 150) return 255;
                        if (c < 300) return 0;
                        if (c < 450) return 255;
                        return 0;
                    }
                    case State::Danger: {  // rapid triple-tap, 800 ms cycle
                        const uint32_t c = t % 800;
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

    // ── Public API ───────────────────────────────────────────────────────────

    void init_output() {
        ledcAttach(config::collision_red_pin,   config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::collision_green_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::collision_blue_pin,  config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::motor_pin,           config::pwm_freq_hz, config::pwm_res);

        ledcAttach(config::sys_mode_red_pin,    config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::sys_mode_green_pin,  config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::sys_mode_blue_pin,   config::pwm_freq_hz, config::pwm_res);

        ledcAttach(config::vib_mode_red_pin,    config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::vib_mode_green_pin,  config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::vib_mode_blue_pin,   config::pwm_freq_hz, config::pwm_res);

        pinMode(config::gps_led_pin, OUTPUT);
        digitalWrite(config::gps_led_pin, LOW);

        init_power_led();
    }

    void shutdown_outputs() {
        set_power_led(false);
        set_motor(0);
        set_collision_rgb(0, 0, 0);
        ledcWrite(config::sys_mode_red_pin,   0);
        ledcWrite(config::sys_mode_green_pin, 0);
        ledcWrite(config::sys_mode_blue_pin,  0);
        ledcWrite(config::vib_mode_red_pin,   0);
        ledcWrite(config::vib_mode_green_pin, 0);
        ledcWrite(config::vib_mode_blue_pin,  0);
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

    // Active LOW: GPIO LOW = LED on, GPIO HIGH = LED off.
    // Release any gpio_hold left from a previous deep sleep before configuring.
    void init_power_led() {
        gpio_hold_dis((gpio_num_t)config::pow_led_pin);
        pinMode(config::pow_led_pin, OUTPUT);
        digitalWrite(config::pow_led_pin, LOW);  // LED on — system is awake
    }

    void set_power_led(bool on) {
        digitalWrite(config::pow_led_pin, on ? LOW : HIGH);
    }

    void apply_output(State state, VibMode vib_mode) {
        set_collision_rgb(rgb_for_state(state));
        set_motor(compute_motor_duty(state, vib_mode));
    }

    void apply_sys_mode_led(Mode mode) {
        set_rgb_pins(
            config::sys_mode_red_pin,
            config::sys_mode_green_pin,
            config::sys_mode_blue_pin,
            rgb_for_mode(mode)
        );
    }

    void apply_vib_mode_led(VibMode vib_mode) {
        set_rgb_pins(
            config::vib_mode_red_pin,
            config::vib_mode_green_pin,
            config::vib_mode_blue_pin,
            rgb_for_vib_mode(vib_mode)
        );
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

        const uint32_t now = millis();
        if (now - last_blink_ms >= config::gps_led_blink_ms) {
            last_blink_ms = now;
            blink_state   = !blink_state;
            digitalWrite(config::gps_led_pin, blink_state ? HIGH : LOW);
        }
    }
}