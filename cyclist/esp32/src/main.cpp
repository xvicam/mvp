#include <Arduino.h>
#include <cmath>
#if defined(ESP32)
#include <esp32-hal-ledc.h>
#endif
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <esp_sleep.h>

#include "../include/AccGyro.h"
#include "../include/CrashDetector.h"
#include "../include/Gnss.h"

#define BUTTON_PIN 14

// IMU interrupt pin (LSM6DSOX INT1/INT2 wired here)
#define IMU_INT_PIN 4

// External RGB LED pins (R,G,B). You said: GPIO 26, 2, 25
#define RGB_R_PIN 26
#define RGB_G_PIN 13
#define RGB_B_PIN 25
#define batteryPin A2

// Set true if your RGB LED is common-anode (active LOW). False for common-cathode (active HIGH).
constexpr bool RGB_COMMON_ANODE = false;

#if defined(ESP32)
  #define STATUSLED_USE_LEDC_ATTACH 1
#else
  #define STATUSLED_USE_LEDC_ATTACH 0
#endif

namespace statusLed {
    enum class Mode : uint8_t {
        Operating,
        Bonding,
        Crash
    };

    static Mode mode = Mode::Operating;

    // LEDC PWM setup (avoid visible flicker)
    static constexpr uint32_t kPwmFreqHz = 5000;
    static constexpr uint8_t kPwmResolutionBits = 8; // 0..255

#if STATUSLED_USE_LEDC_ATTACH

#else
    // v2 and older: fixed channels and explicit setup
    static constexpr uint8_t kChR = 0;
    static constexpr uint8_t kChG = 1;
    static constexpr uint8_t kChB = 2;
#endif

    static inline uint8_t applyPolarity(uint8_t v) {
        return RGB_COMMON_ANODE ? static_cast<uint8_t>(255 - v) : v;
    }

    static void writeRgb(uint8_t r, uint8_t g, uint8_t b) {
#if STATUSLED_USE_LEDC_ATTACH
        ledcWrite(RGB_R_PIN, applyPolarity(r));
        ledcWrite(RGB_G_PIN, applyPolarity(g));
        ledcWrite(RGB_B_PIN, applyPolarity(b));
#else
        ledcWrite(kChR, applyPolarity(r));
        ledcWrite(kChG, applyPolarity(g));
        ledcWrite(kChB, applyPolarity(b));
#endif
    }
    static bool phaseOn(uint32_t nowMs, uint32_t periodMs, uint8_t dutyPct) {
        if (periodMs == 0) return false;
        const uint32_t onMs = (static_cast<uint64_t>(periodMs) * dutyPct) / 100;
        return (nowMs % periodMs) < onMs;
    }

void init() {
#if STATUSLED_USE_LEDC_ATTACH
        ledcAttach(RGB_R_PIN, kPwmFreqHz, kPwmResolutionBits);
        ledcAttach(RGB_G_PIN, kPwmFreqHz, kPwmResolutionBits);
        ledcAttach(RGB_B_PIN, kPwmFreqHz, kPwmResolutionBits);
#else
        ledcSetup(kChR, kPwmFreqHz, kPwmResolutionBits);
        ledcSetup(kChG, kPwmFreqHz, kPwmResolutionBits);
        ledcSetup(kChB, kPwmFreqHz, kPwmResolutionBits);

        ledcAttachPin(RGB_R_PIN, kChR);
        ledcAttachPin(RGB_G_PIN, kChG);
        ledcAttachPin(RGB_B_PIN, kChB);
#endif

        writeRgb(0, 0, 0);
    }
    void setMode(Mode m) {
        mode = m;
    }

    void update(uint32_t nowMs) {
        // Requirements:
        // - Bonding: flash BLUE
        // - Operating: flash GREEN slowly
        // - Crash: flash RED
        switch (mode) {
            case Mode::Bonding: {
                const bool on = phaseOn(nowMs, 250, 50); // 4Hz
                writeRgb(0, 0, on ? 255 : 0);
                break;
            }
            case Mode::Operating: {
                const bool on = phaseOn(nowMs, 2000, 10); // brief pulse every 2s
                writeRgb(0, on ? 255 : 0, 0);
                break;
            }
            case Mode::Crash: {
                const bool on = phaseOn(nowMs, 500, 50); // 1Hz
                writeRgb(on ? 255 : 0, 0, 0);
                break;
            }
        }
    }
}

namespace sys {
    bool isBonding = false;
    bool bondingCompleteFlag = false;
    uint32_t bondingStartTime = 0;

    bool isCrashed = false;
    crash::CrashEvent lastCrash;
    ImuOrientation lastCrashOrientation;
    uint32_t crashModeStartTime = 0;
    bool crashNotificationAcknowledged = false;

    void enterBonding();
    void enterOperating();
    void enterCrashMode(const crash::CrashEvent& ev, const ImuOrientation& o);

}

// Forward declarations (used by power manager before full namespace definitions)
namespace ble {
    void stopAdvertising();
}

namespace espNow {
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo;
    uint32_t lastSendMs = 0;
    // Store the most recent IMU sample for broadcast
    ImuSample lastImuSample{};
    gnss::Fix previousFix{};
    gnss::Fix headingRefFix{};
    uint32_t previousFixAtMs = 0;
    float lastGnssSpeedMps = 0.0f;
    uint32_t lastGnssSpeedAtMs = 0;
    float lastImuSpeedMps = 0.0f;
    float lastHeadingDeg = 0.0f;
    uint32_t lastImuUpdateMs = 0;
    float lastImuAccelMps2 = 0.0f;
    bool lastImuMoving = false;
    constexpr uint32_t kSendPeriodMs = 250;
    uint32_t messageCounter = 0;
    bool isInitialised = false;
    static double haversineMeters(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg) {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kEarthRadiusM = 6371000.0;
        const double lat1 = lat1Deg * kPi / 180.0;
        const double lon1 = lon1Deg * kPi / 180.0;
        const double lat2 = lat2Deg * kPi / 180.0;
        const double lon2 = lon2Deg * kPi / 180.0;
        const double dLat = lat2 - lat1;
        const double dLon = lon2 - lon1;
        const double a = std::sin(dLat * 0.5) * std::sin(dLat * 0.5) +
                         std::cos(lat1) * std::cos(lat2) * std::sin(dLon * 0.5) * std::sin(dLon * 0.5);
        const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        return kEarthRadiusM * c;
    }

    static bool computeGnssSpeedMps(const gnss::Fix &currentFix, float &outSpeedMps, uint32_t &outAgeMs) {
        if (!currentFix.valid || !previousFix.valid) return false;
        if (currentFix.updatedAtMs == 0 || previousFix.updatedAtMs == 0) return false;
        if (currentFix.updatedAtMs <= previousFix.updatedAtMs) return false;

        const uint32_t dtMs = currentFix.updatedAtMs - previousFix.updatedAtMs;
        outAgeMs = millis() - currentFix.updatedAtMs;
        if (dtMs < 200 || dtMs > 5000) return false;

        const double distM = haversineMeters(previousFix.latDeg, previousFix.lngDeg, currentFix.latDeg, currentFix.lngDeg);
        const float speedMps = static_cast<float>(distM / (static_cast<double>(dtMs) / 1000.0));

        if (speedMps < 0.0f) return false;
        const float kMaxSpeedMps = 60.0f; // sanity cap
        outSpeedMps = (speedMps > kMaxSpeedMps) ? kMaxSpeedMps : speedMps;
        return true;
    }

    void updateImuSpeed(uint32_t nowMs, const ImuOrientation &o) {
        constexpr float kNoiseFloorMps2 = 0.15f;
        constexpr float kLeakPerSecMoving = 0.45f;
        constexpr float kLeakPerSecStill = 1.6f;
        constexpr float kMaxSpeedMps = 40.0f;

        if (lastImuUpdateMs == 0) {
            lastImuUpdateMs = nowMs;
            lastImuSpeedMps = 0.0f;
            lastImuAccelMps2 = o.accelMagHp;
            lastImuMoving = o.isMoving;
            return;
        }

        const uint32_t dtMs = nowMs - lastImuUpdateMs;
        if (dtMs == 0 || dtMs > 1000) {
            lastImuUpdateMs = nowMs;
            lastImuAccelMps2 = o.accelMagHp;
            lastImuMoving = o.isMoving;
            return;
        }

        const float dt = static_cast<float>(dtMs) / 1000.0f;
        const float leak = o.isMoving ? kLeakPerSecMoving : kLeakPerSecStill;
        lastImuSpeedMps *= std::exp(-leak * dt);

        float accel = std::fabs(o.accelMagHp);
        if (accel < kNoiseFloorMps2) accel = 0.0f;
        lastImuSpeedMps += accel * dt;

        if (lastImuSpeedMps < 0.0f) lastImuSpeedMps = 0.0f;
        if (lastImuSpeedMps > kMaxSpeedMps) lastImuSpeedMps = kMaxSpeedMps;

        lastImuUpdateMs = nowMs;
        lastImuAccelMps2 = o.accelMagHp;
        lastImuMoving = o.isMoving;
    }

    namespace led {
        uint32_t offAtMs = 0;

        void init() {
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, LOW);
        }

        void pulse(uint32_t durationMs) {
            digitalWrite(LED_BUILTIN, HIGH);
            offAtMs = millis() + durationMs;
        }

        void update() {
            if (offAtMs != 0 && millis() >= offAtMs) {
                digitalWrite(LED_BUILTIN, LOW);
                offAtMs = 0;
            }
        }
    }

    void initSerialAndLed() {
        Serial.begin(115200);
        led::init();
    }

    bool initEspNow() {
        WiFi.mode(WIFI_STA);
        esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
        esp_wifi_set_max_tx_power(78);

        if (esp_now_init() != ESP_OK) return false;

        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;

        isInitialised = true;
        return true;
    }

    void stopEspNow() {
        if (isInitialised) {
            esp_now_deinit();
            WiFi.mode(WIFI_OFF);
            isInitialised = false;
        }
    }

    static float lastGnssHeadingDeg = 0.0f; // Add this cache variable

void sendEspNowBroadcast() {
        if (!isInitialised) return;

        const gnss::Fix currentFix = gnss::lastFix();
        const bool gpsValid = gnss::hasFix(3000); // 3 seconds timeout

        // ONLY recalculate if we have a new GPS coordinate
        if (gpsValid && currentFix.valid && currentFix.updatedAtMs > previousFix.updatedAtMs) {

            const uint32_t dtMs = currentFix.updatedAtMs - previousFix.updatedAtMs;
            const bool coordsChanged = (currentFix.latDeg != previousFix.latDeg || currentFix.lngDeg != previousFix.lngDeg);

            // Process if coordinates changed OR if enough time has passed to conclude we are stationary.
            if (coordsChanged || dtMs >= 800) {
                // Calculate Speed
                float gnssSpeedMps = 0.0f;
                uint32_t gnssSpeedAgeMs = 0;
                if (computeGnssSpeedMps(currentFix, gnssSpeedMps, gnssSpeedAgeMs)) {
                    lastGnssSpeedMps = gnssSpeedMps;
                    lastGnssSpeedAtMs = millis();
                }

                // Calculate Heading
                // We calculate bearing only over a distance of > 2.5 meters to completely eliminate GPS dither/noise.
                if (!headingRefFix.valid) {
                    headingRefFix = currentFix;
                } else if (coordsChanged) {
                    double distM = haversineMeters(headingRefFix.latDeg, headingRefFix.lngDeg, currentFix.latDeg, currentFix.lngDeg);
                    if (distM > 2.5) { 
                        const double kPi = 3.14159265358979323846;
                        double lat1 = headingRefFix.latDeg * kPi / 180.0;
                        double lon1 = headingRefFix.lngDeg * kPi / 180.0;
                        double lat2 = currentFix.latDeg * kPi / 180.0;
                        double lon2 = currentFix.lngDeg * kPi / 180.0;
                        double dLon = lon2 - lon1;
                        double y = std::sin(dLon) * std::cos(lat2);
                        double x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
                        double bearing = std::atan2(y, x);
                        bearing = std::fmod((bearing * 180.0 / kPi) + 360.0, 360.0);
                        lastGnssHeadingDeg = static_cast<float>(bearing);
                        
                        headingRefFix = currentFix; // Update our baseline point
                    }
                }

                // Save the fix ONLY when a new one is processed
                previousFix = currentFix;
                previousFixAtMs = millis();
            }
        }

        // Decide what speed to broadcast
        float speedToBroadcast = 0.0f;
        if (gpsValid && (millis() - lastGnssSpeedAtMs < 3000)) {
            speedToBroadcast = lastGnssSpeedMps;
        } else {
            speedToBroadcast = lastImuSpeedMps;
        }

        float ax = lastImuSample.ax;
        float ay = lastImuSample.ay;
        float az = lastImuSample.az;
        float gx = lastImuSample.gx;
        float gy = lastImuSample.gy;
        float gz = lastImuSample.gz;
        float accelMag = std::sqrt(ax*ax + ay*ay + az*az);

        char message[256];
        snprintf(message, sizeof(message),
                "{\"lat\":%.7f,\"lng\":%.7f,\"alt\":%.2f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,\"speed\":%.2f,\"accel\":%.2f,\"heading\":%.2f}",
                gpsValid ? currentFix.latDeg : 0.0,
                gpsValid ? currentFix.lngDeg : 0.0,
                gpsValid ? currentFix.altMeters : 0.0,
                ax, ay, az, gx, gy, gz, speedToBroadcast, accelMag, lastGnssHeadingDeg);

        esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t *>(message), strlen(message) + 1);
    }

    void sendCrashAlert(uint32_t crashId, float peakDynamicMps2, const ImuOrientation &o) {
        if (!isInitialised) return;
        char message[128];
        snprintf(message, sizeof(message),
                 "CRASH;id=%lu;peakDyn=%.2f;pitch=%.1f;roll=%.1f;orient=%s;moving=%d",
                 static_cast<unsigned long>(crashId),
                 static_cast<double>(peakDynamicMps2),
                 static_cast<double>(o.pitchDeg),
                 static_cast<double>(o.rollDeg),
                 o.orientationLabel,
                 o.isMoving ? 1 : 0);

        if (esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t *>(message), strlen(message) + 1) == ESP_OK) {
            led::pulse(120);
        } else {
            led::pulse(400);
        }
    }
}

namespace imuPrint {
    constexpr uint32_t kPrintPeriodMs = 100;
    uint32_t lastPrintMs = 0;
    bool initialised = false;
    AccGyro imu;

    static void printImuLine(const ImuSample &s, const ImuOrientation &o) {
        Serial.print(";pitchDeg=");
        Serial.print(o.pitchDeg, 2);
        Serial.print(";rollDeg=");
        Serial.print(o.rollDeg, 2);
        Serial.print(";orient=");
        Serial.print(o.orientationLabel);
        Serial.print(";moving=");
        Serial.println(o.isMoving ? 1 : 0);
    }
}

namespace powerMgr {
    // Policy: sleep when still for 2 minutes, wake on IMU interrupt level.
    constexpr uint32_t kStillToSleepMs = 2UL * 60UL * 1000UL; // 2 minutes stillness

    // Wake sources:
    // - IMU interrupt pin
    // - User button

    // Button logic in this project treats a press as HIGH (see loop()).
    constexpr int kButtonWakeLevel = 1; // 1 = HIGH, 0 = LOW

    // IMU interrupt polarity.
    constexpr int kImuWakeLevel = 1; // 1 = HIGH, 0 = LOW
    constexpr bool kImuIntActiveHigh = (kImuWakeLevel == 1);

    // Map existing motion detection (computed in AccGyro) to a still-timer.
    constexpr uint32_t kStillDebounceMs = 3000;

    enum class SleepReason : uint8_t {
        Unknown = 0,
        Button = 1,
        Still = 2
    };

    RTC_DATA_ATTR uint8_t lastSleepReason = static_cast<uint8_t>(SleepReason::Unknown);

    static SleepReason getLastSleepReason() {
        return static_cast<SleepReason>(lastSleepReason);
    }

    static void setLastSleepReason(SleepReason r) {
        lastSleepReason = static_cast<uint8_t>(r);
    }

    uint32_t stillSinceMs = 0;
    uint32_t lastMovingMs = 0;
    bool armed = false;

    static bool eligibleToSleep() {
        // Don’t sleep during bonding or while crashed.
        return (!sys::isBonding) && (!sys::isCrashed) && imuPrint::initialised;
    }

    void initPin() {
        // Keep a defined level while sleeping.
        pinMode(IMU_INT_PIN, (kImuWakeLevel == 0) ? INPUT_PULLUP : INPUT_PULLDOWN);

        pinMode(BUTTON_PIN, (kButtonWakeLevel == 1) ? INPUT_PULLDOWN : INPUT_PULLUP);
    }

    // Forward declaration (used by spurious-wake filter below)
    void enterDeepSleep(const char *reason, SleepReason sleepReason);

    void armImuInterrupt() {
        if (!imuPrint::initialised) return;

        // Route wake-up + inactivity to INT1 by default.
        // 187mg (3 steps) rejects more small vibrations while still waking on real movement.
        const bool ok = imuPrint::imu.configureWakeInactivity(30, 187, true, kImuIntActiveHigh, true);
        Serial.printf("IMU wake/inactivity config: %s (pin=%d level=%d)\n", ok ? "OK" : "FAIL", IMU_INT_PIN, kImuWakeLevel);
    }

    // Ignore EXT0 wake glitches for a short window after boot/wake.
    // This helps when the IMU interrupt line is still settling or latched.
    constexpr uint32_t kIgnoreWakeMs = 800;
    uint32_t ignoreWakeUntilMs = 0;

    // If falsely awake (e.g. IMU line still active but no WU source), go back to sleep.
    static void returnToSleepIfFalseExtWake() {
        const auto cause = esp_sleep_get_wakeup_cause();
        if (cause != ESP_SLEEP_WAKEUP_EXT1) return;
        if (!imuPrint::initialised) return;

        const uint64_t wakeMask = esp_sleep_get_ext1_wakeup_status();
        const bool wokeByButton = (wakeMask & (1ULL << BUTTON_PIN)) != 0;
        if (wokeByButton) return;

        const int pinNow = digitalRead(IMU_INT_PIN);
        const bool pinStillActive = (pinNow == kImuWakeLevel);

        bool wu = false, inact = false;
        (void)imuPrint::imu.readWakeInactivitySources(wu, inact);

        if (pinStillActive && !wu) {
            Serial.printf("Spurious EXT wake (mask=0x%llX imuPinNow=%d active=%d wu=%d inact=%d). Returning to sleep.\n",
                          static_cast<unsigned long long>(wakeMask),
                          pinNow,
                          pinStillActive ? 1 : 0,
                          wu ? 1 : 0,
                          inact ? 1 : 0);
            Serial.flush();
            delay(50);
            enterDeepSleep("spurious EXT", SleepReason::Unknown);
        }
    }

    void enterDeepSleep(const char *reason, SleepReason sleepReason) {
        Serial.printf("Entering deep sleep (%s)\n", reason ? reason : "");

        // Clear any latched IMU interrupt before sleeping.
        if (imuPrint::initialised) {
            bool wu = false, inact = false;
            (void)imuPrint::imu.readWakeInactivitySources(wu, inact);
        }

        const uint32_t waitStartMs = millis();
        bool warned = false;
        while (digitalRead(BUTTON_PIN) == kButtonWakeLevel) {
            if (!warned && (millis() - waitStartMs) > 1000) {
                Serial.println("Waiting for button release before sleep...");
                warned = true;
            }
            delay(10);
        }

        setLastSleepReason(sleepReason);

        Serial.printf("IMU_INT_PIN=%d level now=%d (wakeLevel=%d)\n", IMU_INT_PIN, digitalRead(IMU_INT_PIN), kImuWakeLevel);
        Serial.printf("BUTTON_PIN=%d level now=%d (wakeLevel=%d)\n", BUTTON_PIN, digitalRead(BUTTON_PIN), kButtonWakeLevel);
        Serial.flush();

        // Stop radios cleanly.
        espNow::stopEspNow();
        ble::stopAdvertising();

        const bool buttonOnlyWake = (sleepReason == SleepReason::Button);
        uint64_t wakeMask = (1ULL << BUTTON_PIN);
        if (!buttonOnlyWake) {
            wakeMask |= (1ULL << IMU_INT_PIN);
        }

        if (kImuWakeLevel == 1 || kButtonWakeLevel == 1) {
            esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        } else {
            esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ALL_LOW);
        }

        // Give BLE/WiFi stacks a moment to quiesce and let Serial flush.
        delay(200);
        esp_deep_sleep_start();
    }

    void onBootPrintWakeReason() {
        const auto cause = esp_sleep_get_wakeup_cause();
        Serial.print("Wake cause: ");
        switch (cause) {
            case ESP_SLEEP_WAKEUP_EXT1: {
                const uint64_t mask = esp_sleep_get_ext1_wakeup_status();
                Serial.printf("EXT1 (mask=0x%llX)", static_cast<unsigned long long>(mask));
                if (mask & (1ULL << BUTTON_PIN)) Serial.print(" [BUTTON]");
                if (mask & (1ULL << IMU_INT_PIN)) Serial.print(" [IMU]");
                Serial.println();
                break;
            }
            case ESP_SLEEP_WAKEUP_TIMER: Serial.println("TIMER"); break;
            case ESP_SLEEP_WAKEUP_UNDEFINED: Serial.println("POWERON/RESET"); break;
            default: Serial.printf("%d\n", static_cast<int>(cause)); break;
        }
    }

    void enforceHoldToWakeFromButtonOrStill() {
        const SleepReason lastReason = getLastSleepReason();
        if (lastReason != SleepReason::Button && lastReason != SleepReason::Still) return;
        if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) return;

        const uint64_t mask = esp_sleep_get_ext1_wakeup_status();
        if ((mask & (1ULL << BUTTON_PIN)) == 0) return;

        constexpr uint32_t kWakeHoldMs = 2000;
        constexpr uint32_t kPollDelayMs = 2;

        if (digitalRead(BUTTON_PIN) != kButtonWakeLevel) {
            Serial.println("Wake hold not detected, returning to sleep...");
            enterDeepSleep("wake hold not met", lastReason);
        }

        const uint32_t startMs = millis();
        const uint32_t deadlineMs = startMs + kWakeHoldMs;

        while (digitalRead(BUTTON_PIN) == kButtonWakeLevel) {
            const uint32_t nowMs = millis();
            if (static_cast<int32_t>(nowMs - deadlineMs) >= 0) {
                Serial.println("Wake hold satisfied.");
                setLastSleepReason(SleepReason::Unknown);
                return;
            }
            delay(kPollDelayMs);
        }

        Serial.println("Wake hold released early, returning to sleep...");
        enterDeepSleep("wake hold not met", lastReason);
    }

    void update(uint32_t nowMs, bool isMoving) {
        if (ignoreWakeUntilMs != 0 && nowMs < ignoreWakeUntilMs) {
            return;
        }
        if (!eligibleToSleep()) {
            armed = false;
            stillSinceMs = 0;
            return;
        }

        if (!armed) {
            armImuInterrupt();
            armed = true;
            lastMovingMs = nowMs;
            stillSinceMs = 0;
        }

        if (isMoving) {
            lastMovingMs = nowMs;
            stillSinceMs = 0;
            return;
        }

        // not moving
        if (stillSinceMs == 0) {
            // require a short debounce interval after last detected movement
            if (nowMs - lastMovingMs >= kStillDebounceMs) {
                stillSinceMs = nowMs;
            }
            return;
        }
        if (nowMs - stillSinceMs >= kStillToSleepMs) {
            enterDeepSleep("still threshold reached", SleepReason::Still);
        }
    }
}

namespace ble {
    static const NimBLEUUID SVC("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e1");
    static const NimBLEUUID CHR("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e2");

    static NimBLEServer *srv;
    static NimBLECharacteristic *chr;
    static bool hasBonds() { return NimBLEDevice::getNumBonds() > 0; }

    struct SrvCb : NimBLEServerCallbacks {
        void onConnect(NimBLEServer *s, NimBLEConnInfo &connInfo) override {
            Serial.printf("BLE Client Connected! Address: %s, Bonded: %d\n", connInfo.getAddress().toString().c_str(), connInfo.isBonded());
            if (hasBonds() && !NimBLEDevice::isBonded(connInfo.getAddress())) {
                Serial.println("Rejecting unbonded connection.");
                s->disconnect(connInfo.getConnHandle());
            }
        }

        void onDisconnect(NimBLEServer *s, NimBLEConnInfo &connInfo, int reason) override {
            Serial.printf("BLE Client Disconnected! Reason: %d\n", reason);
            if (sys::isBonding) NimBLEDevice::startAdvertising();
        }

        void onAuthenticationComplete(NimBLEConnInfo &connInfo) override {
            if (connInfo.isBonded()) {
                sys::bondingCompleteFlag = true;
            } else if (!connInfo.isEncrypted()) {
                NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
            }
        }
    };

    static SrvCb cbs;

    struct ChrCb : NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
            std::string val = pCharacteristic->getValue();
            if (val.find("ACK") != std::string::npos) {
                sys::crashNotificationAcknowledged = true;
                Serial.println("Crash notification acknowledged by APP!");
            }
        }
    };
    static ChrCb chrCb;

    static bool started = false;

    void init(const char *name) {
        if (started) return;
        NimBLEDevice::init(name);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        srv = NimBLEDevice::createServer();
        srv->setCallbacks(&cbs);

        auto *service = srv->createService(SVC);
        chr = service->createCharacteristic(CHR, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
        chr->setCallbacks(&chrCb);
        static uint32_t v = 0;
        chr->setValue(v);

        started = true;
    }

    void startAdvertising(bool isBonding) {
        auto *adv = NimBLEDevice::getAdvertising();
        adv->addServiceUUID(SVC);

        // Force the BLE name
        adv->enableScanResponse(true);
        adv->setName("VICAM");

        // 0x02 = General Discoverable (shows in new devices list)
        // 0 = Non-discoverable (hidden from new devices list)
        adv->setDiscoverableMode(isBonding ? 0x02 : 0);

        adv->start();
    }

    void stopAdvertising() {
        NimBLEDevice::getAdvertising()->stop();
    }

    void tick(uint32_t nowMs) {
        static uint32_t last = 0;
        if (nowMs - last < 500) return;
        last = nowMs;
        if (srv && srv->getConnectedCount() > 0) {
        }
    }

    void sendCrash(uint32_t crashId, float peakDynamicMps2, const ImuOrientation &o) {
        if (!srv || srv->getConnectedCount() == 0) {
            return;
        }
        char message[256];

        const gnss::Fix fix = gnss::lastFix();
        const bool gpsValid = gnss::hasFix(3000);
        const uint32_t gpsAgeMs = fix.updatedAtMs == 0 ? 0 : (millis() - fix.updatedAtMs);

        // Keep JSON backwards compatible by always including gps.lat/lng, but add gps.valid/age/sats.
        const double lat = gpsValid ? fix.latDeg : 0.0;
        const double lng = gpsValid ? fix.lngDeg : 0.0;
        snprintf(message, sizeof(message),
                 "{\"type\":\"crash\",\"crashId\":%lu,\"peakDyn\":%.2f,\"pitch\":%.1f,\"roll\":%.1f,\"orient\":\"%s\",\"moving\":%s,\"gps\":{\"lat\":%.7f,\"lng\":%.7f,\"valid\":%s,\"ageMs\":%lu,\"satsUsed\":%u}}",
                 static_cast<unsigned long>(crashId),
                 static_cast<double>(peakDynamicMps2),
                 static_cast<double>(o.pitchDeg),
                 static_cast<double>(o.rollDeg),
                 o.orientationLabel,
                 o.isMoving ? "true" : "false",
                 static_cast<double>(lat),
                 static_cast<double>(lng),
                 gpsValid ? "true" : "false",
                 static_cast<unsigned long>(gpsAgeMs),
                 static_cast<unsigned int>(fix.satsUsed));

        chr->setValue(reinterpret_cast<uint8_t *>(message), strlen(message));
        chr->notify();
    }
}

namespace sys {
    void enterBonding() {
        isBonding = true;
        bondingStartTime = millis();
        espNow::stopEspNow();
        ble::startAdvertising(true);
        statusLed::setMode(statusLed::Mode::Bonding);
        Serial.println("Entered Bonding Mode.");
    }

    void enterOperating() {
        isBonding = false;
        isCrashed = false;
        crashNotificationAcknowledged = false;
        ble::stopAdvertising();
        espNow::initEspNow();
        espNow::lastSendMs = millis();
        statusLed::setMode(statusLed::Mode::Operating);
        Serial.println("Entered Operating Mode. WiFi enabled.");
    }

    void enterCrashMode(const crash::CrashEvent& ev, const ImuOrientation& o) {
        isCrashed = true;
        crashModeStartTime = millis();
        crashNotificationAcknowledged = false;
        lastCrash = ev;
        lastCrashOrientation = o;
        espNow::stopEspNow();
        ble::startAdvertising(false);
        statusLed::setMode(statusLed::Mode::Crash);
        Serial.println("CRASH DETECTED! Switched to BLE Mode.");
    }
}

void setup() {
    espNow::initSerialAndLed();
    powerMgr::initPin();

    statusLed::init();

    ble::init("VICAM");

    const bool gnssOk = gnss::init();
    Serial.println(gnssOk ? "GNSS init OK" : "GNSS init FAILED (check wiring/I2C addr)" );

    powerMgr::onBootPrintWakeReason();
    powerMgr::enforceHoldToWakeFromButtonOrStill();

    sys::enterOperating();

    imuPrint::initialised = imuPrint::imu.beginI2C();

    // Configure IMU interrupts as early as possible (required for wake).
    if (imuPrint::initialised) {
        powerMgr::armImuInterrupt();
        // If the user is pressing the button, prevent immediately re-entering sleep.
        powerMgr::ignoreWakeUntilMs = millis() + powerMgr::kIgnoreWakeMs + 700;
        powerMgr::returnToSleepIfFalseExtWake();
    } else {
        Serial.println("IMU init failed; sleep-on-still disabled.");
    }

    analogReadResolution(12);

}

void loop() {
    const uint32_t now = millis();
    espNow::led::update();
    statusLed::update(now);

    gnss::update(now);

    if (!sys::isBonding && espNow::isInitialised) {
        if (now - espNow::lastSendMs >= espNow::kSendPeriodMs) {
            espNow::lastSendMs = now;
            espNow::sendEspNowBroadcast();
        }
    }

    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'C' || c == 'c') {
            ImuOrientation o{};
            o.orientationLabel = "Simulated";

            if (imuPrint::initialised) {
                ImuSample s;
                if (imuPrint::imu.readSample(s)) {
                    espNow::lastImuSample = s;
                    o = imuPrint::imu.computeOrientation(s);
                }
            }

            static uint32_t simCrashId = 1000;
            crash::CrashEvent mockEv;
            mockEv.triggered = true;
            mockEv.crashId = simCrashId++;
            mockEv.peakDynamicMps2 = o.accelMagnitude;

            if (!sys::isCrashed) {
                sys::enterCrashMode(mockEv, o);
                Serial.println("Simulated crash triggered. Switched to BLE Mode.");
            } else {
                Serial.println("Already in crash mode.");
            }
        }
    }

    static uint32_t lastPulseMs = 0;
    uint32_t pulseInterval = sys::isBonding ? 250 : 2000;
    if (now - lastPulseMs > pulseInterval) {
        lastPulseMs = now;
        espNow::led::pulse(50);
    }

    static constexpr uint32_t kButtonLongPressMs = 2000;
    static constexpr uint32_t kButtonCrashLongPressMs = 5000;
    static constexpr uint32_t kButtonDoublePressGapMs = 400;
    static constexpr uint32_t kButtonDebounceMs = 30;

    static bool btnPrev = false;
    static uint32_t btnDownAt = 0;
    static uint32_t lastReleaseAt = 0;
    static uint8_t pressCount = 0;
    static bool longPressHandled = false;

    const bool btnNow = (digitalRead(BUTTON_PIN) == HIGH);

    if (btnNow && !btnPrev) {
        btnDownAt = now;
        longPressHandled = false;
    }

    if (!btnNow && btnPrev) {
        const uint32_t heldMs = (btnDownAt != 0) ? (now - btnDownAt) : 0;
        if (!longPressHandled && heldMs >= kButtonDebounceMs) {
            pressCount = static_cast<uint8_t>(pressCount + 1);
            lastReleaseAt = now;
        }
        btnDownAt = 0;
    }

    if (btnNow && !longPressHandled && btnDownAt != 0) {
        const uint32_t holdMs = sys::isCrashed ? kButtonCrashLongPressMs : kButtonLongPressMs;
        if ((now - btnDownAt) >= holdMs) {
            longPressHandled = true;
            pressCount = 0;
            Serial.println(sys::isCrashed
                ? "Button held for 5 seconds in crash mode, entering deep sleep..."
                : "Button held for 2 seconds, entering deep sleep...");
            powerMgr::enterDeepSleep("button long press", powerMgr::SleepReason::Button);
        }
    }

    if (pressCount == 1 && (now - lastReleaseAt) > kButtonDoublePressGapMs) {
        pressCount = 0;
    }

    if (pressCount >= 2) {
        pressCount = 0;
        if (sys::isCrashed) {
            Serial.println("Button double-press in crash mode, ignoring bonding request...");
        } else if (!sys::isBonding) {
            Serial.println("Button double-pressed, entering bonding mode...");
            sys::enterBonding();
        } else {
            Serial.println("Button double-pressed, exiting bonding mode...");
            sys::enterOperating();
        }
    }

    btnPrev = btnNow;

    if (sys::bondingCompleteFlag) {
        sys::bondingCompleteFlag = false;
        sys::enterOperating();
    }

    if (sys::isBonding && (now - sys::bondingStartTime > 120000)) {
        Serial.println("Bonding timeout, entering operating mode...");
        sys::enterOperating();
    }

    if (imuPrint::initialised && (now - imuPrint::lastPrintMs >= imuPrint::kPrintPeriodMs)) {
        imuPrint::lastPrintMs = now;
        ImuSample s;

        if (imuPrint::imu.readSample(s)) {
            espNow::lastImuSample = s;
            const ImuOrientation o = imuPrint::imu.computeOrientation(s);
            espNow::updateImuSpeed(now, o);
            static crash::CrashDetector detector;
            const crash::CrashEvent ev = detector.update(now, s, o);

            if (ev.triggered && !sys::isCrashed) {
                sys::enterCrashMode(ev, o);
            }

            const gnss::Fix fix = gnss::lastFix();
            if (gnss::hasFix(3000)) {
                Serial.printf("GPS - Lat: %.7f, Lng: %.7f, Alt: %.2f | ", fix.latDeg, fix.lngDeg, fix.altMeters);
            } else {
                Serial.print("GPS - Waiting for fix... | ");
            }

            int rawValue = analogRead(batteryPin);
            float voltage = (rawValue / 4095.0) * 3.3 * 2.0;
            float percentage = (voltage - 3.3) / (4.2 - 3.3) * 100;

            if (percentage > 100) percentage = 100;
            if (percentage < 0) percentage = 0;

            float currentSpeed = espNow::lastGnssSpeedMps;
            Serial.printf("Voltage: %.2fV | Percentage: %.1f%% | Speed: %.2fm/s | Heading: %.2fdeg ", voltage, percentage, currentSpeed, espNow::lastHeadingDeg);

            imuPrint::printImuLine(s, o);

            powerMgr::update(now, o.isMoving);
        }
    }

    if (sys::isCrashed) {
        if (sys::crashNotificationAcknowledged || (millis() - sys::crashModeStartTime > 120000)) {
            Serial.println("Crash mode ended (ACK received or timeout). Returning to operating mode...");
            sys::enterOperating();
        } else {
            static uint32_t lastCrashSend = 0;
            if (now - lastCrashSend > 1000) {
                lastCrashSend = now;
                ble::sendCrash(sys::lastCrash.crashId, sys::lastCrash.peakDynamicMps2, sys::lastCrashOrientation);
            }
        }
    }

    if (sys::isBonding) ble::tick(now);
}
