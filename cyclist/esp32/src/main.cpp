#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#include "AccGyro.h"

// Simple ESP-NOW broadcaster for ESP32
namespace espNow{
  // Broadcast MAC address (all 0xFF -> broadcast)
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // Peer info struct for ESP-NOW operations
  esp_now_peer_info_t peerInfo;

  // Last time we sent a message (ms)
  uint32_t lastSendMs = 0;
  // How often to send messages (milliseconds)
  constexpr uint32_t kSendPeriodMs = 250;
  // Simple message counter to vary payload
  uint32_t messageCounter = 0;

  namespace led {
    // Non-blocking LED pulse so we don't stall the main loop.
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
      if (offAtMs != 0 && static_cast<int32_t>(millis() - offAtMs) >= 0) {
        digitalWrite(LED_BUILTIN, LOW);
        offAtMs = 0;
      }
    }
  }

  // Initialize serial port and built-in LED
  void initSerialAndLed() {
    Serial.begin(115200);
    led::init();
  }

  // Configure WiFi for STA mode and initialise ESP-NOW
  bool initEspNow() {
    WiFi.mode(WIFI_STA);
    // Use long-range protocol and set TX power
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    esp_wifi_set_max_tx_power(78);

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initialising ESP-NOW");
      return false;
    }

    // Prepare a broadcast peer entry
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Register the broadcast peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add broadcast peer");
      return false;
    }

    return true;
  }

  // Build a small message and send it via ESP-NOW to broadcast address
  void sendEspNowBroadcast() {
    char message[64];
    snprintf(message, sizeof(message), "Hello ESP-NOW %lu", static_cast<unsigned long>(messageCounter++));

    // Send including terminating null (hence +1)
    esp_err_t result = esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t*>(message), strlen(message) + 1);
    if (result == ESP_OK) {
      // brief pulse on successful send
      led::pulse(40);
    } else {
      Serial.print("ESP-NOW send failed: ");
      Serial.println(static_cast<int>(result));
      // longer pulse on error
      led::pulse(250);
    }
  }
}

namespace imuPrint {
  // Print rate for accelerometer results
  constexpr uint32_t kPrintPeriodMs = 100;
  uint32_t lastPrintMs = 0;

  bool initialised = false;

  AccGyro imu;

  static void printImuLine(const ImuSample& s, const ImuOrientation& o) {
    // Single-line, parseable output (m/s^2 and g)
    constexpr float kG = 9.80665f;

    Serial.print("ax_mps2="); Serial.print(s.ax, 3);
    Serial.print(";ay_mps2="); Serial.print(s.ay, 3);
    Serial.print(";az_mps2="); Serial.print(s.az, 3);

    Serial.print(";ax_g="); Serial.print(s.ax / kG, 3);
    Serial.print(";ay_g="); Serial.print(s.ay / kG, 3);
    Serial.print(";az_g="); Serial.print(s.az / kG, 3);

    Serial.print(";pitchDeg="); Serial.print(o.pitchDeg, 2);
    Serial.print(";rollDeg="); Serial.print(o.rollDeg, 2);
    Serial.print(";orient="); Serial.print(o.orientationLabel);
    Serial.print(";mag="); Serial.print(o.accelMagnitude, 3);
    Serial.print(";magFilt="); Serial.print(o.filteredMagnitude, 3);
    Serial.print(";moving="); Serial.println(o.isMoving ? 1 : 0);
  }
}

void setup() {
  // Prepare debugging output and LED
  espNow::initSerialAndLed();

  // Initialise ESP-NOW (peer added inside)
  espNow::initEspNow();

  // Initialise accelerometer/IMU
  imuPrint::initialised = imuPrint::imu.beginI2C();
  if (!imuPrint::initialised) {
    Serial.println("IMU init failed; will not print accel results.");
  }
}

void loop() {
  // keep LED pulses non-blocking
  espNow::led::update();

  // Periodically send messages, non-blocking timing
  const uint32_t now = millis();
  if (now - espNow::lastSendMs >= espNow::kSendPeriodMs) {
    espNow::lastSendMs = now;
    espNow::sendEspNowBroadcast();
  }

  // Periodically print accelerometer results
  if (imuPrint::initialised && (now - imuPrint::lastPrintMs >= imuPrint::kPrintPeriodMs)) {
    imuPrint::lastPrintMs = now;

    ImuSample s;
    if (imuPrint::imu.readSample(s)) {
      const ImuOrientation o = imuPrint::imu.computeOrientation(s);
      imuPrint::printImuLine(s, o);
    }
  }
}