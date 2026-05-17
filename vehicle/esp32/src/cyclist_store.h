#pragma once

#include <Arduino.h>

#include "types.h"

namespace cyclist_store {
    int find_cyclist_by_mac(const uint8_t mac[6]);
    int find_free_cyclist_slot();
    int find_oldest_cyclist_slot();

    void remove_expired_cyclists();

    bool parse_cyclist_packet(
        const uint8_t mac[6],
        const char* json,
        bool has_rssi,
        int8_t rssi_dbm
    );

    CyclistsData get_cyclists_data();

    uint8_t get_active_cyclist_count();
    const CyclistData* get_cyclist_at(int index);
}