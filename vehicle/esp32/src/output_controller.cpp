#include "output_controller.h"
#include "config.h"

namespace output_controller {
    void init_output() {
        ledcAttach(config::motor_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::red_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::green_pin, config::pwm_freq_hz, config::pwm_res);
        ledcAttach(config::blue_pin, config::pwm_freq_hz, config::pwm_res);
    }

    void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
        ledcWrite(config::red_pin, r);
        ledcWrite(config::green_pin, g);
        ledcWrite(config::blue_pin, b);
    }

    void set_motor(uint8_t duty) {
        ledcWrite(config::motor_pin, duty);
    }

    void apply_output(State state) {
        switch (state) {
            case State::Off:
                set_rgb(0, 0, 0);
                set_motor(0);
                break;

            case State::GpsWait:
                set_rgb(80, 0, 80);
                set_motor(0);
                break;

            case State::Idle:
                set_rgb(0, 255, 0);
                set_motor(0);
                break;

            case State::Moving:
                set_rgb(0, 0, 255);
                set_motor(0);
                break;

            case State::Alert:
                set_rgb(255, 255, 0);
                set_motor(80);
                break;

            case State::Warning:
                set_rgb(255, 120, 0);
                set_motor(160);
                break;

            case State::Urgent:
                set_rgb(255, 0, 0);
                set_motor(255);
                break;
        }
    }
}