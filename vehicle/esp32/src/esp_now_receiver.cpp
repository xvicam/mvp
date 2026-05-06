#include "esp_now_receiver.h"
#include "config.h"
#include "cyclist_store.h"
#include "helpers.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <string.h>

namespace esp_now_receiver {
    struct PendingPacket {
        bool is_available = false;
        bool has_overflow = false;
        uint8_t mac[6] = {0};
        char data[config::max_packet_size] = {0};
        int len = 0;
    };

    PendingPacket pending_packet;
    portMUX_TYPE packet_mux = portMUX_INITIALIZER_UNLOCKED;

    void on_receive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
        if (!info || !data || len <= 0) {
            return;
        }

        portENTER_CRITICAL_ISR(&packet_mux);

        if (len >= config::max_packet_size) {
            pending_packet.has_overflow = true;
        } else {
            memcpy(pending_packet.mac, info->src_addr, 6);
            memcpy(pending_packet.data, data, len);
            pending_packet.len = len;
            pending_packet.is_available = true;
        }

        portEXIT_CRITICAL_ISR(&packet_mux);
    }

    bool init_esp_now_receiver() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();

        if (esp_now_init() != ESP_OK) {
            return false;
        }

        esp_now_register_recv_cb(on_receive);
        return true;
    }

    void process_pending_packet() {
        PendingPacket local;

        portENTER_CRITICAL(&packet_mux);

        local = pending_packet;
        pending_packet.is_available = false;
        pending_packet.has_overflow = false;

        portEXIT_CRITICAL(&packet_mux);

        if (local.has_overflow) {
            Serial.println("Rejected ESP-NOW packet: too large");
            return;
        }

        if(!local.is_available) {
            return;
        }

        local.data[local.len] = '\0';

        Serial.print("RX from ");
        helpers::print_mac(local.mac);
        Serial.print(": ");
        Serial.println(local.data);

        cyclist_store::parse_cyclist_packet(local.mac, local.data);
    }

}