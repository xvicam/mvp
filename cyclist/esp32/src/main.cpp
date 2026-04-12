#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

#include "../include/AccGyro.h"
#include "../include/CrashDetector.h"

#define BUTTON_PIN 27

namespace sys {
  bool isBonding = false;
  bool bondingCompleteFlag = false;
  uint32_t bondingStartTime = 0;

  void enterBonding();
  void enterOperating();
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

    if (esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t*>(message), strlen(message) + 1) == ESP_OK) {
      led::pulse(40);
    } else {
      led::pulse(250);
    }
  }

  void sendCrashAlert(uint32_t crashId, float peakDynamicMps2, const ImuOrientation& o) {
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

    if (esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t*>(message), strlen(message) + 1) == ESP_OK) {
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

  static void printImuLine(const ImuSample& s, const ImuOrientation& o) {
    Serial.print(";pitchDeg="); Serial.print(o.pitchDeg, 2);
    Serial.print(";rollDeg="); Serial.print(o.rollDeg, 2);
    Serial.print(";orient="); Serial.print(o.orientationLabel);
    Serial.print(";moving="); Serial.println(o.isMoving ? 1 : 0);
  }
}

namespace ble {
  static const NimBLEUUID SVC("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e1");
  static const NimBLEUUID CHR("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e2");

  static NimBLEServer* srv;
  static NimBLECharacteristic* chr;
  static bool hasBonds() { return NimBLEDevice::getNumBonds() > 0; }

  struct SrvCb : NimBLEServerCallbacks {
    void onConnect(NimBLEServer* s, NimBLEConnInfo& connInfo) override {
      if (hasBonds() && !NimBLEDevice::isBonded(connInfo.getAddress())) {
        s->disconnect(connInfo.getConnHandle());
      }
    }

    void onDisconnect(NimBLEServer* s, NimBLEConnInfo& connInfo, int reason) override {
      if (sys::isBonding) NimBLEDevice::startAdvertising();
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
      if (connInfo.isBonded()) {
        sys::bondingCompleteFlag = true;
      } else if (!connInfo.isEncrypted()) {
        NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
      }
    }
  };

  static SrvCb cbs;
  static bool started = false;

  void init(const char* name) {
    if (started) return;
    NimBLEDevice::init(name);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    srv = NimBLEDevice::createServer();
    srv->setCallbacks(&cbs);

    auto* service = srv->createService(SVC);
    chr = service->createCharacteristic(CHR, NIMBLE_PROPERTY::NOTIFY);
    static uint32_t v = 0;
    chr->setValue(v);

    started = true;
  }

  void startAdvertising() {
    auto* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SVC);
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
      uint32_t x = nowMs;
      chr->setValue(x);
      chr->notify();
    }
  }
}

namespace sys {
  void enterBonding() {
    isBonding = true;
    bondingStartTime = millis();
    espNow::stopEspNow();
    ble::startAdvertising();
    Serial.println("Entered Bonding Mode.");
  }

  void enterOperating() {
    isBonding = false;
    ble::stopAdvertising();
    espNow::initEspNow();
    Serial.println("Entered Operating Mode. WiFi enabled.");
  }
}

void setup() {
  espNow::initSerialAndLed();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ble::init("Cyclist-ESP32");

  sys::enterOperating();

  imuPrint::initialised = imuPrint::imu.beginI2C();
}

void loop() {
  const uint32_t now = millis();
  espNow::led::update();

  static uint32_t btnPressTime = 0;
  static bool actionTriggered = false;
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (btnPressTime == 0) {
      btnPressTime = now;
      actionTriggered = false;
    } else if (now - btnPressTime > 3000 && !actionTriggered) {
      actionTriggered = true;
      if (!sys::isBonding) {
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

  if (!sys::isBonding && now - espNow::lastSendMs >= espNow::kSendPeriodMs) {
    espNow::lastSendMs = now;
    espNow::sendEspNowBroadcast();
  }

  if (imuPrint::initialised && (now - imuPrint::lastPrintMs >= imuPrint::kPrintPeriodMs)) {
    imuPrint::lastPrintMs = now;
    ImuSample s;

    if (imuPrint::imu.readSample(s)) {
      const ImuOrientation o = imuPrint::imu.computeOrientation(s);
      static crash::CrashDetector detector;
      const crash::CrashEvent ev = detector.update(now, s, o);

      if (ev.triggered) {
        espNow::sendCrashAlert(ev.crashId, ev.peakDynamicMps2, o);
      }
      imuPrint::printImuLine(s, o);
    }
  }

  if (sys::isBonding) ble::tick(now);
}