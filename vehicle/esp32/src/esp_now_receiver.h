#pragma once

namespace esp_now_receiver {
    bool init_esp_now_receiver();
    void deinit_esp_now_receiver();
    void process_pending_packet();
}