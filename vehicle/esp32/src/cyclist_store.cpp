#include "cyclist_store.h"
#include "config.h"
#include "helpers.h"

#include <ArduinoJson.h>
#include <string.h>

namespace cyclist_store {

    static CyclistData cyclists[config::max_cyclists];

    // ── Slot management ──────────────────────────────────────────────────────

    int find_cyclist_by_mac(const uint8_t mac[6]) {
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (cyclists[i].is_active && helpers::is_same_mac(cyclists[i].mac, mac)) {
                return i;
            }
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
        const uint32_t now = millis();
        for (uint8_t i = 0; i < config::max_cyclists; i++) {
            if (cyclists[i].is_active &&
                now - cyclists[i].last_seen_ms > config::cyclist_timeout_ms)
            {
                cyclists[i].is_active = false;
            }
        }
    }

    // ── RSSI smoothing ───────────────────────────────────────────────────────
    // Only updates fields when the packet actually carried RSSI metadata —
    // otherwise the previous reading is preserved.
    static void update_rssi(int slot, bool has_rssi, int8_t rssi_dbm) {
        if (!has_rssi) return;

        const bool  had_rssi  = cyclists[slot].has_rssi;
        const float prev_rssi = cyclists[slot].rssi_smoothed_dbm;

        cyclists[slot].has_rssi = true;
        cyclists[slot].rssi_dbm = rssi_dbm;
        cyclists[slot].rssi_smoothed_dbm = had_rssi
            ? (config::rssi_smoothing_alpha * static_cast<float>(rssi_dbm) +
               (1.0f - config::rssi_smoothing_alpha) * prev_rssi)
            : static_cast<float>(rssi_dbm);
    }

    // ── Packet parsing ───────────────────────────────────────────────────────

    // Fields parsed out of the JSON before any slot is touched.
    struct ParsedPacket {
        bool   is_remote     = false;
        State  cyclist_state = State::Safe;   // remote
        double lat           = 0.0;           // gps
        double lng           = 0.0;           // gps
        float  speed_kmph    = 0.0f;          // gps
    };

    static bool validate_remote(const JsonDocument& doc, ParsedPacket& out) {
        if (!doc["cri"].is<int>()) {
            Serial.println("Rejected remote packet: missing cri");
            return false;
        }
        switch (doc["cri"].as<int>()) {
            case 0: out.cyclist_state = State::Safe;    break;
            case 1: out.cyclist_state = State::Alert;   break;
            case 2: out.cyclist_state = State::Warning; break;
            case 3: out.cyclist_state = State::Danger;  break;
            default:
                Serial.println("Rejected remote packet: invalid cri");
                return false;
        }
        out.is_remote = true;
        return true;
    }

    static bool validate_gps(const JsonDocument& doc, ParsedPacket& out) {
        if (!doc["lat"].is<double>() || !doc["lng"].is<double>()) {
            Serial.println("Rejected packet: missing lat/lng");
            return false;
        }

        out.lat        = doc["lat"].as<double>();
        out.lng        = doc["lng"].as<double>();
        out.speed_kmph = doc["speed"] | 0.0f;

        if (!helpers::is_valid_coordinate(out.lat, out.lng)) {
            Serial.println("Rejected packet: invalid coordinates");
            return false;
        }
        if (out.speed_kmph < 0.0f ||
            out.speed_kmph > config::max_reasonable_cyclist_speed_kmph)
        {
            Serial.println("Rejected packet: invalid speed");
            return false;
        }

        out.is_remote = false;
        return true;
    }

    bool parse_cyclist_packet(
        const uint8_t mac[6],
        const char* json,
        bool has_rssi,
        int8_t rssi_dbm
    ) {
        // 384 bytes is enough headroom for the larger GPS packet (~13 fields).
        StaticJsonDocument<384> doc;
        const DeserializationError error = deserializeJson(doc, json);
        if (error) {
            Serial.print("Bad JSON: ");
            Serial.println(error.c_str());
            return false;
        }

        // ── Validate before touching any slot ────────────────────────────────
        // Critical: a packet that fails validation must NOT mutate the store,
        // otherwise a bad packet can evict a healthy cyclist via the oldest-slot
        // path and then be thrown away.
        ParsedPacket parsed;
        const char* packet_mode = doc["mode"] | "";

        if (strcmp(packet_mode, "remote") == 0) {
            if (!validate_remote(doc, parsed)) return false;
        } else {
            // Backwards-compatible: missing "mode" → GPS packet.
            if (!validate_gps(doc, parsed)) return false;
        }

        // ── Allocate slot ────────────────────────────────────────────────────
        int        index      = find_cyclist_by_mac(mac);
        const bool is_new_mac = (index < 0);

        if (is_new_mac) {
            index = find_free_cyclist_slot();
            if (index < 0) index = find_oldest_cyclist_slot();
            // Full reset when claiming a slot for a different MAC — prevents
            // the previous cyclist's RSSI/GPS/remote state from bleeding into
            // the new one (especially the EMA-smoothed RSSI).
            cyclists[index] = CyclistData{};
        }

        // ── Commit ───────────────────────────────────────────────────────────
        cyclists[index].is_active    = true;
        memcpy(cyclists[index].mac, mac, 6);
        cyclists[index].last_seen_ms = millis();
        update_rssi(index, has_rssi, rssi_dbm);

        if (parsed.is_remote) {
            cyclists[index].has_cyclist_state = true;
            cyclists[index].cyclist_state     = parsed.cyclist_state;
            // The cyclist has switched to broadcasting remote state — any
            // previously cached GPS coordinates are no longer trustworthy.
            cyclists[index].has_gps_data      = false;
        } else {
            cyclists[index].has_gps_data      = true;
            cyclists[index].lat               = parsed.lat;
            cyclists[index].lng               = parsed.lng;
            cyclists[index].speed_kmph        = parsed.speed_kmph;
            cyclists[index].has_cyclist_state = false;
            cyclists[index].cyclist_state     = State::Safe;
        }

        return true;
    }

    // ── Accessors ────────────────────────────────────────────────────────────

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