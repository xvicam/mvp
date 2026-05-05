#include "vehicle_reader.h"
#include "config.h"
#include "helpers.h"

#include <TinyGPS++.h>

namespace vehicle_reader {
    TinyGPSPlus gps;
    HardwareSerial gps_serial(2);

    bool is_vehicle_moving_flag = false;

    void init_gps() {
        gps_serial.begin(
            config::gps_baud,
            SERIAL_8N1,
            config::gps_rx_pin,
            config::gps_tx_pin
        );
    }

    void update_gps() {
        while (gps_serial.available()) {
            gps.encode(gps_serial.read());
        }
    }

    bool is_vehicle_gps_valid() {
        return gps.location.isValid() &&
               gps.location.age() <= config::gps_max_age_ms &&
               helpers::is_valid_coordinate(gps.location.lat(), gps.location.lng());
    }

    float get_vehicle_speed_kmph() {
        if (!gps.speed.isValid()) {
            return 0.0;
        }

        float speed = gps.speed.kmph();

        if (speed < 0.0 || speed > 250.0) {
            return 0.0;
        }

        return speed;
    }

    void update_vehicle_moving(float speed_kmph) {
        if (!is_vehicle_moving_flag && speed_kmph >= config::moving_on_kmph) {
            is_vehicle_moving_flag = true;
        } else if (is_vehicle_moving_flag && speed_kmph <= config::moving_off_kmph) {
            is_vehicle_moving_flag = false;
        }
    }
    
    bool is_vehicle_moving() {
        return is_vehicle_moving_flag;
    }

    VehicleData get_vehicle_data() {
        VehicleData data;

        data.is_gps_valid = is_vehicle_gps_valid();
        data.speed_kmph = get_vehicle_speed_kmph();

        if (data.is_gps_valid) {
            update_vehicle_moving(data.speed_kmph);

            data.lat = gps.location.lat();
            data.lng = gps.location.lng();
        }

        data.is_moving = is_vehicle_moving_flag;

        return data;
    }

    bool get_utc_time(char* buffer, size_t buffer_size) {
        if (!gps.time.isValid()) {
            return false;
        }

        snprintf(
            buffer,
            buffer_size,
            "%02d:%02d:%02d",
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second()
        );

        return true;
    }
}