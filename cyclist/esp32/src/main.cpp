#include <Arduino.h>
#if defined(ESP32)
#include <esp32-hal-ledc.h>
#endif
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

#include "../include/AccGyro.h"
#include "../include/CrashDetector.h"

#define BUTTON_PIN 14

// External RGB LED pins (R,G,B). You said: GPIO 26, 2, 25
#define RGB_R_PIN 26
#define RGB_G_PIN 13
#define RGB_B_PIN 25

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

namespace espNow {
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo;
    uint32_t lastSendMs = 0;
    constexpr uint32_t kSendPeriodMs = 250;
    uint32_t messageCounter = 0;
    bool isInitialised = false;

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

    void sendEspNowBroadcast() {
        if (!isInitialised) return;
        char message[64];
        snprintf(message, sizeof(message), "Hello ESP-NOW %lu", static_cast<unsigned long>(messageCounter++));
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
            // Serial.println("Skipping sendCrash: no clients connected"); // Optional: uncomment if needed
            return;
        }
        char message[256];
        // Dummy GPS coordinates
        float lat = 52.4862;
        float lng = -1.8904;
        snprintf(message, sizeof(message),
                 "{\"type\":\"crash\",\"crashId\":%lu,\"peakDyn\":%.2f,\"pitch\":%.1f,\"roll\":%.1f,\"orient\":\"%s\",\"moving\":%s,\"gps\":{\"lat\":%.4f,\"lng\":%.4f}}",
                 static_cast<unsigned long>(crashId),
                 static_cast<double>(peakDynamicMps2),
                 static_cast<double>(o.pitchDeg),
                 static_cast<double>(o.rollDeg),
                 o.orientationLabel,
                 o.isMoving ? "true" : "false",
                 static_cast<double>(lat), static_cast<double>(lng));

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
    pinMode(BUTTON_PIN, INPUT);

    statusLed::init();

    ble::init("VICAM");

    sys::enterOperating();

    imuPrint::initialised = imuPrint::imu.beginI2C();
}

void loop() {
    const uint32_t now = millis();
    espNow::led::update();
    statusLed::update(now);

    // Broadcast ESP-NOW periodically when not bonding
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

    static uint32_t btnPressTime = 0;
    static bool actionTriggered = false;
    if (digitalRead(BUTTON_PIN) == HIGH) {
        if (btnPressTime == 0) {
            btnPressTime = now;
            actionTriggered = false;
        } else if (now - btnPressTime > 3000 && !actionTriggered) {
            actionTriggered = true;
            if (sys::isCrashed) {
                Serial.println("Button held in crash mode, ignoring bonding request...");
            } else if (!sys::isBonding) {
                Serial.println("Button held for 3 seconds, entering bonding mode...");
                sys::enterBonding();
            } else {
                Serial.println("Button held for 3 seconds, exiting bonding mode...");
                sys::enterOperating();
            }
        }
    } else {
        btnPressTime = 0;
        actionTriggered = false;
    }

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
            const ImuOrientation o = imuPrint::imu.computeOrientation(s);
            static crash::CrashDetector detector;
            const crash::CrashEvent ev = detector.update(now, s, o);

            if (ev.triggered && !sys::isCrashed) {
                sys::enterCrashMode(ev, o);
            }
            imuPrint::printImuLine(s, o);
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
