#include "esp_now_receiver.h"
#include "config.h"
#include "cyclist_store.h"
#include "helpers.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <mbedtls/aes.h>
#include <string.h>

namespace esp_now_receiver {
    struct PendingPacket {
        bool is_available = false;
        bool has_overflow = false;
        uint8_t mac[6] = {0};
        char data[config::max_packet_size] = {0};
        int len = 0;
        bool has_rssi = false;
        int8_t rssi_dbm = 0;
    };

    PendingPacket pending_packet;
    portMUX_TYPE packet_mux = portMUX_INITIALIZER_UNLOCKED;

    // ── AES-128-CBC decryption ────────────────────────────────────────────────
    // Key and IV must match send_encrypted() on the cyclist side exactly.
    static const unsigned char kAesKey[16] = {
        'V','I','C','A','M','_','E','S','P','_','S','E','C','R','E','T'
    };
    static const unsigned char kAesIv[16] = {
        'v','i','c','a','m','_','i','v','_','0','0','0','0','0','0','0'
    };

    // Decrypts `len` bytes of ciphertext (must be a positive multiple of 16)
    // into `output` (null-terminated on success). Returns true on success.
    static bool decrypt_packet(
        const uint8_t* ciphertext, int len,
        char* output, size_t output_size
    ) {
        if (len <= 0 || len % 16 != 0 || static_cast<size_t>(len) > output_size) {
            return false;
        }

        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes, kAesKey, 128);

        // CBC modifies the IV in-place; copy so kAesIv stays pristine for the next call.
        unsigned char iv[16];
        memcpy(iv, kAesIv, 16);

        int ret = mbedtls_aes_crypt_cbc(
            &aes, MBEDTLS_AES_DECRYPT, len,
            iv, ciphertext, reinterpret_cast<unsigned char*>(output)
        );
        mbedtls_aes_free(&aes);

        if (ret != 0) return false;

        // The cyclist null-terminates the plaintext before encrypting,
        // so the decrypted buffer is already a valid C string.
        // Force the final byte as a belt-and-braces guard.
        output[len - 1] = '\0';
        return true;
    }

    void on_receive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
        if (!info || !data || len <= 0) return;

        portENTER_CRITICAL_ISR(&packet_mux);

        if (len >= config::max_packet_size) {
            pending_packet.has_overflow = true;
        } else {
            memcpy(pending_packet.mac, info->src_addr, 6);
            memcpy(pending_packet.data, data, len);  // raw ciphertext
            pending_packet.len = len;
            pending_packet.is_available = true;
            if (info->rx_ctrl) {
                pending_packet.has_rssi = true;
                pending_packet.rssi_dbm = info->rx_ctrl->rssi;
            } else {
                pending_packet.has_rssi = false;
                pending_packet.rssi_dbm = 0;
            }
        }

        portEXIT_CRITICAL_ISR(&packet_mux);
    }

    bool init_esp_now_receiver() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();

        if (esp_now_init() != ESP_OK) return false;

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

        if (!local.is_available) return;

        // Encrypted packets are always a multiple of 16 bytes (AES block size).
        if (local.len <= 0 || local.len % 16 != 0) {
            Serial.print("Rejected: invalid ciphertext length (");
            Serial.print(local.len);
            Serial.println(")");
            return;
        }

        // Decrypt into a fresh plaintext buffer — never touch local.data for parsing.
        char plaintext[config::max_packet_size] = {0};
        if (!decrypt_packet(
            reinterpret_cast<const uint8_t*>(local.data),
            local.len,
            plaintext,
            sizeof(plaintext)
        )) {
            Serial.println("Rejected: decryption failed");
            return;
        }

        Serial.print("RX from ");
        helpers::print_mac(local.mac);
        Serial.print(": ");
        Serial.print(plaintext);

        if (local.has_rssi) {
            Serial.print(" | RSSI ");
            Serial.print(local.rssi_dbm);
            Serial.print(" dBm");
        }

        Serial.println();

        cyclist_store::parse_cyclist_packet(
            local.mac, plaintext, local.has_rssi, local.rssi_dbm
        );
    }
}