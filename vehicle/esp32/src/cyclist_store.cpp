#include "cyclist_store.h"
#include "config.h"
#include "helpers.h"

#include <ArduinoJson.h>
#include <string.h>

namespace cyclist_store {
    CyclistData cyclists[config::max_cyclists];

    int find_cyclist_by_mac(const uint8_t mac[6]) {
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (cyclists[i].is_active && helpers::is_same_mac(cyclists[i].mac, mac))
                return i;
        }
        return -1;
    }

    int find_free_cyclist_slot() {
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (!cyclists[i].is_active) return i;
        }
        return -1;
    }

    int find_oldest_cyclist_slot() {
        uint8_t  oldest_index = 0;
        uint32_t oldest_time  = cyclists[0].last_seen_ms;
        for (uint8_t i = 1; i < config::max_cyclists; i++) {
            if (cyclists[i].last_seen_ms < oldest_time) {
                oldest_time  = cyclists[i].last_seen_ms;
                oldest_index = i;
            }
        }
        return oldest_index;
    }

    void remove_expired_cyclists() {
        uint32_t now = millis();
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (cyclists[i].is_active &&
                now - cyclists[i].last_seen_ms > config::cyclist_timeout_ms)
            {
                cyclists[i].is_active = false;
            }
        }
    }

    // ── RSSI smoothing helper (shared by both packet types) ───────────────────
    void update_rssi(int slot, bool has_rssi, int8_t rssi_dbm) {
        bool  had_rssi  = cyclists[slot].has_rssi;
        float prev_rssi = cyclists[slot].rssi_smoothed_dbm;

        cyclists[slot].has_rssi = has_rssi;
        cyclists[slot].rssi_dbm = rssi_dbm;

        if (has_rssi) {
            cyclists[slot].rssi_smoothed_dbm = had_rssi
                ? (config::rssi_smoothing_alpha * static_cast<float>(rssi_dbm) +
                   (1.0f - config::rssi_smoothing_alpha) * prev_rssi)
                : static_cast<float>(rssi_dbm);
        }
    }

    bool parse_cyclist_packet(
        const uint8_t mac[6],
        const char* json,
        bool has_rssi,
        int8_t rssi_dbm
    ) {
        // Bump buffer slightly for the larger GPS packet (13 fields).
        StaticJsonDocument<384> doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            Serial.print("Bad JSON: ");
            Serial.println(error.c_str());
            return false;
        }

        const char* packet_mode = doc["mode"] | "";

        // ── Find or allocate slot ─────────────────────────────────────────────
        int index = find_cyclist_by_mac(mac);
        if (index < 0) index = find_free_cyclist_slot();
        if (index < 0) index = find_oldest_cyclist_slot();

        cyclists[index].is_active = true;
        memcpy(cyclists[index].mac, mac, 6);
        cyclists[index].last_seen_ms = millis();

        update_rssi(index, has_rssi, rssi_dbm);

        // ── Remote packet: {"mode":"remote","cri":0|1|2|3} ───────────────────
        if (strcmp(packet_mode, "remote") == 0) {
            if (!doc["cri"].is<int>()) {
                Serial.println("Rejected remote packet: missing cri");
                return false;
            }

            cyclists[index].has_cyclist_state = true;
            switch (doc["cri"].as<int>()) {
                case 1: cyclists[index].cyclist_state = State::Alert;   break;
                case 2: cyclists[index].cyclist_state = State::Warning; break;
                case 3: cyclists[index].cyclist_state = State::Danger;  break;
                default: cyclists[index].cyclist_state = State::Safe;   break;
            }
            return true;
        }

        // ── GPS packet: {"mode":"gps","lat":...,"lng":...,...} ────────────────
        // Also accepts packets with no "mode" field for backwards compatibility.
        if (!doc["lat"].is<double>() || !doc["lng"].is<double>()) {
            Serial.println("Rejected packet: missing lat/lng");
            return false;
        }

        double lat        = doc["lat"].as<double>();
        double lng        = doc["lng"].as<double>();
        float  speed_kmph = doc["speed"] | 0.0f;

        if (!helpers::is_valid_coordinate(lat, lng)) {
            Serial.println("Rejected packet: invalid coordinates");
            return false;
        }

        if (speed_kmph < 0.0f ||
            speed_kmph > config::max_reasonable_cyclist_speed_kmph)
        {
            Serial.println("Rejected packet: invalid speed");
            return false;
        }

        cyclists[index].lat               = lat;
        cyclists[index].lng               = lng;
        cyclists[index].speed_kmph        = speed_kmph;
        cyclists[index].has_cyclist_state = false;
        cyclists[index].cyclist_state     = State::Safe;
        return true;
    }

    CyclistsData get_cyclists_data() {
        CyclistsData data;
        data.cyclists   = cyclists;
        data.slot_count = config::max_cyclists;
        return data;
    }

    uint8_t get_active_cyclist_count() {
        uint8_t count = 0;
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (cyclists[i].is_active) count++;
        }
        return count;
    }

    const CyclistData* get_cyclist_at(int index) {
        if (index < 0 || index >= config::max_cyclists) return nullptr;
        return &cyclists[index];
    }
}