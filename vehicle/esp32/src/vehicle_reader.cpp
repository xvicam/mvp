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
            config::gps_serial_rx_pin,
            config::gps_serial_tx_pin
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

    void print_gps_data() {
        Serial.println();
        Serial.println("========== GPS DATA ==========");

        Serial.print("Chars processed: ");
        Serial.println(gps.charsProcessed());

        Serial.print("Passed checksum: ");
        Serial.println(gps.passedChecksum());

        Serial.print("Failed checksum: ");
        Serial.println(gps.failedChecksum());

        Serial.print("GPS valid: ");
        Serial.println(is_vehicle_gps_valid() ? "yes" : "no");

        Serial.print("Location valid: ");
        Serial.println(gps.location.isValid() ? "yes" : "no");

        Serial.print("Location age: ");
        Serial.print(gps.location.age());
        Serial.println(" ms");

        if (gps.location.isValid()) {
            Serial.print("Lat: ");
            Serial.println(gps.location.lat(), 6);

            Serial.print("Lng: ");
            Serial.println(gps.location.lng(), 6);
        } else {
            Serial.println("Lat: unavailable");
            Serial.println("Lng: unavailable");
        }

        Serial.print("Speed: ");
        if (gps.speed.isValid()) {
            Serial.print(gps.speed.kmph());
            Serial.println(" km/h");
        } else {
            Serial.println("unavailable");
        }

        Serial.print("Course: ");
        if (gps.course.isValid()) {
            Serial.print(gps.course.deg());
            Serial.println(" deg");
        } else {
            Serial.println("unavailable");
        }

        Serial.print("Satellites: ");
        if (gps.satellites.isValid()) {
            Serial.println(gps.satellites.value());
        } else {
            Serial.println("unavailable");
        }

        Serial.print("HDOP: ");
        if (gps.hdop.isValid()) {
            Serial.println(gps.hdop.hdop());
        } else {
            Serial.println("unavailable");
        }

        Serial.print("Altitude: ");
        if (gps.altitude.isValid()) {
            Serial.print(gps.altitude.meters());
            Serial.println(" m");
        } else {
            Serial.println("unavailable");
        }

        Serial.print("UTC time: ");
        if (gps.time.isValid()) {
            char time_buffer[12];

            snprintf(
            time_buffer,
            sizeof(time_buffer),
            "%02d:%02d:%02d",
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second()
            );

            Serial.println(time_buffer);
        } else {
            Serial.println("unavailable");
        }

        Serial.println("==============================");
        Serial.println();
        }
}