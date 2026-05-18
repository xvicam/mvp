#include "input_controller.h"
#include "config.h"
#include "state_controller.h"

#include <Arduino.h>

namespace input_controller {
    struct ButtonState {
        bool last_raw         = true;   // HIGH = not pressed (INPUT_PULLUP)
        bool is_pressed       = false;
        bool long_press_fired = false;
        uint32_t debounce_start_ms = 0;
        uint32_t press_start_ms    = 0;
    };

    ButtonState mode_btn;
    ButtonState manual_btn;

    void init_inputs() {
        pinMode(config::mode_btn_pin,   INPUT_PULLUP);
        pinMode(config::manual_btn_pin, INPUT_PULLUP);
    }

    void process_button(
        ButtonState& btn,
        uint8_t pin,
        void (*on_click)(),
        void (*on_long_press)()
    ) {
        bool raw = digitalRead(pin);
        uint32_t now = millis();

        // Start debounce timer on any raw change
        if (raw != btn.last_raw) {
            btn.last_raw = raw;
            btn.debounce_start_ms = now;
        }

        // Ignore until signal has been stable for debounce_ms
        if (now - btn.debounce_start_ms < config::debounce_ms) {
            return;
        }

        bool pressed = (raw == LOW);  // INPUT_PULLUP: LOW = pressed

        if (pressed && !btn.is_pressed) {
            // Falling edge
            btn.is_pressed       = true;
            btn.press_start_ms   = now;
            btn.long_press_fired = false;

        } else if (pressed && btn.is_pressed) {
            // Held — fire long press once when threshold is crossed
            if (!btn.long_press_fired &&
                on_long_press != nullptr &&
                now - btn.press_start_ms >= config::long_press_ms)
            {
                on_long_press();
                btn.long_press_fired = true;
            }

        } else if (!pressed && btn.is_pressed) {
            // Rising edge — fire click only if long press did not already fire
            btn.is_pressed = false;
            if (!btn.long_press_fired && on_click != nullptr) {
                on_click();
            }
        }
    }

    void on_mode_click()       { state_controller::cycle_vib_mode(); }
    void on_mode_long_press()  { state_controller::cycle_system_mode(); }
    void on_manual_click()     { state_controller::cycle_manual_state(); }

    void process_inputs() {
        process_button(mode_btn,   config::mode_btn_pin,   on_mode_click,   on_mode_long_press);
        process_button(manual_btn, config::manual_btn_pin, on_manual_click, nullptr);
    }
}